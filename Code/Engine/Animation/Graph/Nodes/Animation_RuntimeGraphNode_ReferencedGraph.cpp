#include "Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ReferencedGraphNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ReferencedGraphNode>( context, options );
        pNode->m_pGraphInstance = context.m_referencedGraphInstances[m_referencedGraphIdx];

        context.SetOptionalNodePtrFromIndex( m_fallbackNodeIdx, pNode->m_pFallbackNode );

        // Create parameter mapping
        //-------------------------------------------------------------------------

        if ( pNode->m_pGraphInstance != nullptr )
        {
            size_t const numChildParams = pNode->m_pGraphInstance->GetNumControlParameters();
            for ( int16_t childParamIdx = 0; childParamIdx < numChildParams; childParamIdx++ )
            {
                ValueNode* pFoundParameterNode = nullptr;

                // Try find a parameter with the same ID in the parent graph
                StringID const childParamID = pNode->m_pGraphInstance->GetControlParameterID( childParamIdx );
                auto foundParamIter = context.m_parameterLookupMap.find( childParamID );
                if ( foundParamIter != context.m_parameterLookupMap.end() )
                {
                    int16_t const parentParamIdx = foundParamIter->second;
                    ValueNode* pParentParameterNode = reinterpret_cast<ValueNode*>( context.m_nodePtrs[parentParamIdx] );

                    // Check value types
                    auto const parentParamType = pParentParameterNode->GetValueType();
                    auto const childParamType = pNode->m_pGraphInstance->GetControlParameterType( childParamIdx );
                    if ( parentParamType == childParamType )
                    {
                        pFoundParameterNode = pParentParameterNode;
                    }
                    else
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( "Mismatch parameter type for referenced graph '%s', parent type: '%s', child type: '%s'", pNode->m_pGraphInstance->GetDefinitionResourceID().c_str(), GetNameForValueType( parentParamType ), GetNameForValueType( childParamType ) );
                        #endif 
                    }
                }

                // Add mapping
                pNode->m_parameterMapping.emplace_back( pFoundParameterNode );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool ReferencedGraphNode::IsValid() const
    {
        if ( m_pGraphInstance != nullptr )
        {
            return m_pGraphInstance->IsValid();
        }

        if ( m_pFallbackNode != nullptr )
        {
            return m_pFallbackNode->IsValid();
        }

        return false;
    }

    void ReferencedGraphNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        if ( m_pGraphInstance != nullptr )
        {
            ReflectControlParameters( context );
            m_pGraphInstance->ResetGraphState( initialTime );

            auto pRootNode = const_cast<PoseNode*>( m_pGraphInstance->GetRootNode() );
            EE_ASSERT( pRootNode->IsInitialized() );
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0;

            // Initialize the fallback node if set
            if ( m_pFallbackNode != nullptr )
            {
                m_pFallbackNode->Initialize( context, initialTime );

                if ( m_pFallbackNode->IsValid() )
                {
                    m_duration = m_pFallbackNode->GetDuration();
                    m_previousTime = m_pFallbackNode->GetPreviousTime();
                    m_currentTime = m_pFallbackNode->GetCurrentTime();
                }
            }
        }
    }

    void ReferencedGraphNode::ShutdownInternal( GraphContext& context )
    {
        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = const_cast<PoseNode*>( m_pGraphInstance->GetRootNode() );
            EE_ASSERT( pRootNode->IsInitialized() );
            pRootNode->Shutdown( context );
        }
        else
        {
            if ( m_pFallbackNode != nullptr )
            {
                m_pFallbackNode->Shutdown( context );
            }
        }
    }

    //-------------------------------------------------------------------------

    SyncTrack const& ReferencedGraphNode::GetSyncTrack() const
    {
        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = m_pGraphInstance->GetRootNode();
            if ( pRootNode->IsValid() )
            {
                return pRootNode->GetSyncTrack();
            }
        }
        else if( m_pFallbackNode != nullptr )
        {
            return m_pFallbackNode->GetSyncTrack();
        }

        return SyncTrack::s_defaultTrack;
    }

    void ReferencedGraphNode::ReflectControlParameters( GraphContext& context )
    {
        EE_ASSERT( m_pGraphInstance != nullptr );
        size_t const numChildParams = m_pGraphInstance->GetNumControlParameters();
        EE_ASSERT( m_parameterMapping.size() == numChildParams );

        //-------------------------------------------------------------------------

        for ( int16_t childParamIdx = 0; childParamIdx < numChildParams; childParamIdx++ )
        {
            ValueNode* pParameter = m_parameterMapping[childParamIdx];
            if ( pParameter == nullptr )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            GraphValueType const valueType = pParameter->GetValueType();

            switch ( valueType )
            {
                case GraphValueType::Bool:
                {
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<bool>( context ) );
                }
                break;

                case GraphValueType::ID:
                {
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<StringID>( context ) );
                }
                break;

                case GraphValueType::Float:
                {
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<float>( context ) );
                }
                break;

                case GraphValueType::Vector:
                {
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<Float3>( context ) );
                }
                break;

                case GraphValueType::Target:
                {
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<Target>( context ) );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult ReferencedGraphNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = InvalidIndex;

        //-------------------------------------------------------------------------

        if ( m_pGraphInstance != nullptr )
        {
            ReflectControlParameters( context );

            // Push the current node idx into the event debug path
            #if EE_DEVELOPMENT_TOOLS
            context.PushDebugPath( GetNodeIndex() );
            #endif

            // Evaluate referenced graph
            result = m_pGraphInstance->EvaluateReferencedGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, pUpdateRange, context.m_pLayerContext );

            // Transfer graph state
            auto pRootNode = m_pGraphInstance->GetRootNode();
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();

            // Update sampled event range
            result.m_sampledEventRange.m_endIdx = context.m_pSampledEventsBuffer->GetNumSampledEvents();

            // Pop debug path element
            #if EE_DEVELOPMENT_TOOLS
            context.PopDebugPath();
            #endif
        }
        else if( m_pFallbackNode != nullptr && m_pFallbackNode->IsValid() )
        {
            result = m_pFallbackNode->Update( context, pUpdateRange );
            m_duration = m_pFallbackNode->GetDuration();
            m_previousTime = m_pFallbackNode->GetPreviousTime();
            m_currentTime = m_pFallbackNode->GetCurrentTime();
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ReferencedGraphNode::DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->DrawNodeDebug( graphContext, drawCtx );
        }
    }

    void ReferencedGraphNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        // Record the referenced graph initial state
        if ( m_pGraphInstance != nullptr )
        {
            RecordedGraphState* pGraphState = outState.CreateReferencedGraphStateRecording( GetNodeIndex() );
            m_pGraphInstance->RecordGraphState( *pGraphState );
        }
    }

    void ReferencedGraphNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        // Restore referenced graph state
        if ( m_pGraphInstance != nullptr )
        {
            RecordedGraphState* pGraphState = inState.GetReferencedGraphRecording( GetNodeIndex() );
            m_pGraphInstance->SetToRecordedState( *pGraphState );
        }
    }
    #endif
}
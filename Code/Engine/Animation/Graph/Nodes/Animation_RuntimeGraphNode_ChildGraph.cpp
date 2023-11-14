#include "Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ChildGraphNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ChildGraphNode>( context, options );
        pNode->m_pGraphInstance = context.m_childGraphInstances[m_childGraphIdx];

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
                        context.LogWarning( "Mismatch parameter type for child graph '%s', parent type: '%s', child type: '%s'", pNode->m_pGraphInstance->GetDefinitionResourceID().c_str(), GetNameForValueType( parentParamType ), GetNameForValueType( childParamType ) );
                        #endif 
                    }
                }

                // Add mapping
                pNode->m_parameterMapping.emplace_back( pFoundParameterNode );
            }
        }
    }

    //-------------------------------------------------------------------------

    void ChildGraphNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
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
        }
    }

    void ChildGraphNode::ShutdownInternal( GraphContext& context )
    {
        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = const_cast<PoseNode*>( m_pGraphInstance->GetRootNode() );
            EE_ASSERT( pRootNode->IsInitialized() );
            pRootNode->Shutdown( context );
        }
    }

    //-------------------------------------------------------------------------

    SyncTrack const& ChildGraphNode::GetSyncTrack() const
    {
        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = m_pGraphInstance->GetRootNode();
            return pRootNode->GetSyncTrack();
        }

        return SyncTrack::s_defaultTrack;
    }

    void ChildGraphNode::ReflectControlParameters( GraphContext& context )
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
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<Vector>( context ) );
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

    GraphPoseNodeResult ChildGraphNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
            context.m_pSampledEventsBuffer->PushDebugGraphPathElement( GetNodeIndex() );
            #endif

            // Evaluate child graph
            result = m_pGraphInstance->EvaluateChildGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, pUpdateRange, context.m_pLayerContext );

            // Transfer graph state
            auto pRootNode = m_pGraphInstance->GetRootNode();
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();

            // Update sampled event range
            result.m_sampledEventRange.m_endIdx = context.m_pSampledEventsBuffer->GetNumSampledEvents();
            if ( result.m_sampledEventRange.GetLength() > 0 )
            {
                EE_LOG_INFO( "Anim", "CG", "Start: %d, End: %d", result.m_sampledEventRange.m_startIdx, result.m_sampledEventRange.m_endIdx );
            }

            // Transfer root motion debug
            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordGraphSource( GetNodeIndex(), result.m_rootMotionDelta );
            #endif

            // Pop debug path element
            #if EE_DEVELOPMENT_TOOLS
            context.m_pSampledEventsBuffer->PopDebugGraphPathElement();
            #endif
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ChildGraphNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        // Record the child graph initial state
        RecordedGraphState* pChildGraphState = outState.CreateChildGraphStateRecording( GetNodeIndex() );
        m_pGraphInstance->RecordGraphState( *pChildGraphState );
    }

    void ChildGraphNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        // Restore child graph state
        RecordedGraphState* pChildGraphState = inState.GetChildGraphRecording( GetNodeIndex() );
        m_pGraphInstance->SetToRecordedState( *pChildGraphState );
    }
    #endif
}
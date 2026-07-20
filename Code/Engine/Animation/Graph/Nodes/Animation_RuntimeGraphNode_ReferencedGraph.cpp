#include "Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ReferencedGraphNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ReferencedGraphNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_fallbackNodeIdx, pNode->m_pFallbackNode );

        // External graph
        //-------------------------------------------------------------------------

        // If this is an external graph there's nothing to do
        if ( m_referencedGraphIdx == InvalidIndex )
        {
            return;
        }

        // Internal graph
        //-------------------------------------------------------------------------

        // Take ownership of the referenced graph instance
        pNode->m_pGraphInstance = context.m_referencedGraphInstances[m_referencedGraphIdx];

        // Create parameter mapping
        //-------------------------------------------------------------------------

        if ( pNode->m_pGraphInstance == nullptr )
        {
            return;
        }

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

    //-------------------------------------------------------------------------

    ReferencedGraphNode::~ReferencedGraphNode()
    {
        EE_ASSERT( m_pGraphInstance == nullptr );
    }

    bool ReferencedGraphNode::IsValid() const
    {
        if ( m_pGraphInstance != nullptr )
        {
            return true;
        }

        if ( m_pFallbackNode != nullptr )
        {
            return m_pFallbackNode->IsValid();
        }

        return false;
    }

    GraphDefinition const* ReferencedGraphNode::GetGraphDefinition() const
    {
        EE_ASSERT( HasInstance() );
        return m_pGraphInstance->GetGraphDefinition();
    }

    void ReferencedGraphNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        if ( m_pGraphInstance != nullptr )
        {
            if ( IsInternalGraphSlot() )
            {
                ReflectControlParametersFromParent( context );
            }

            // Check if we have any recorded timing info
            GraphTimeInfo const *pReferencedGraphTimeInfo = nullptr;
            if ( context.m_pInitializationTimeInfo != nullptr )
            {
                for ( auto const &rgs : context.m_pInitializationTimeInfo->m_referencedGraphStates )
                {
                    if ( rgs.m_referencedGraphNodeIdx == GetNodeIndex() )
                    {
                        pReferencedGraphTimeInfo = rgs.m_pTimeInfo;
                        break;
                    }
                }
            }

            context.PushBasePath( GetNodeIndex() );

            // Reset the referenced instance
            if ( pReferencedGraphTimeInfo != nullptr )
            {
                m_pGraphInstance->ResetReferencedGraphState( context, *pReferencedGraphTimeInfo );
            }
            else
            {
                m_pGraphInstance->ResetReferencedGraphState( context, initialTime );
            }

            auto pRootNode = m_pGraphInstance->GetRootNode();
            EE_ASSERT( pRootNode->IsInitialized() );
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();

            context.PopBasePath();
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0;

            // Initialize the fallback node if set
            if ( m_pFallbackNode != nullptr )
            {
                m_pFallbackNode->Initialize( context, initialTime );
                m_duration = m_pFallbackNode->GetDuration();
                m_previousTime = m_pFallbackNode->GetPreviousTime();
                m_currentTime = m_pFallbackNode->GetCurrentTime();
            }
        }
    }

    void ReferencedGraphNode::ShutdownInternal( GraphContext& context )
    {
        if( m_pGraphInstance == nullptr && m_pFallbackNode != nullptr )
        {
            EE_ASSERT( m_pFallbackNode->IsInitialized() );
            m_pFallbackNode->Shutdown( context );
        }

        PoseNode::ShutdownInternal( context );
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
        else if ( m_pFallbackNode != nullptr )
        {
            if ( m_pFallbackNode->IsValid() )
            {
                return m_pFallbackNode->GetSyncTrack();
            }
        }

        return SyncTrack::s_defaultTrack;
    }

    void ReferencedGraphNode::ReflectControlParametersFromParent( GraphContext& context )
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

    void ReferencedGraphNode::ConnectExternalGraphInstance( GraphContext &context, GraphInstance *pInstance )
    {
        EE_ASSERT( IsExternalGraphSlot() );
        EE_ASSERT( pInstance != nullptr );
        EE_ASSERT( m_pGraphInstance == nullptr );

        if ( m_pFallbackNode != nullptr && m_pFallbackNode->IsInitialized() )
        {
            m_pFallbackNode->Shutdown( context );
        }

        m_pGraphInstance = pInstance;
    }

    void ReferencedGraphNode::DisconnectExternalGraphInstance( GraphContext &context )
    {
        EE_ASSERT( IsExternalGraphSlot() );
        EE_ASSERT( m_pGraphInstance != nullptr );

        m_pGraphInstance = nullptr;

        if ( m_pFallbackNode != nullptr )
        {
            EE_ASSERT( !m_pFallbackNode->IsInitialized() );
            m_pFallbackNode->Initialize( context, SyncTrackTime() );
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
            if ( IsInternalGraphSlot() )
            {
                ReflectControlParametersFromParent( context );
            }

            if ( !m_pGraphInstance->IsInitialized() )
            {
                m_pGraphInstance->ResetGraphState();
            }

            if ( !m_pGraphInstance->IsValid() )
            {
                return result;
            }

            // Push the current node idx into the base path
            context.PushBasePath( GetNodeIndex() );

            // Evaluate referenced graph
            result = m_pGraphInstance->EvaluateReferencedGraph( context, pUpdateRange );

            // Transfer graph state
            auto pRootNode = m_pGraphInstance->GetRootNode();
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();

            // Update sampled event range
            result.m_sampledEventRange.m_endIdx = context.GetSampledEventsBuffer()->GetNumSampledEvents();

            // Pop debug path element
            context.PopBasePath();
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

    void ReferencedGraphNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        // Record the referenced graph initial state
        if ( IsInternalGraphSlot() && m_pGraphInstance != nullptr )
        {
            RecordedGraphState* pGraphState = outState.CreateReferencedGraphStateRecording( GetNodeIndex() );
            m_pGraphInstance->RecordReferencedGraphState( *pGraphState );
        }
    }

    bool ReferencedGraphNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        // Restore referenced graph state
        if ( IsInternalGraphSlot() && m_pGraphInstance != nullptr )
        {
            RecordedGraphState* pGraphState = inState.GetReferencedGraphRecording( GetNodeIndex() );
            pGraphState->PrepareForReading();
            if ( !m_pGraphInstance->SetToRecordedReferencedGraphState( *pGraphState ) )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ReferencedGraphNode::DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->SetGraphDebugMode( GraphDebugMode::On );
            m_pGraphInstance->DrawDebug( drawCtx, graphContext.m_pNodesToDebug );
            m_pGraphInstance->SetGraphDebugMode( GraphDebugMode::Off );
        }
    }
    #endif

    //-------------------------------------------------------------------------
    // I Slot Filled
    //-------------------------------------------------------------------------

    void IsExternalGraphSlotFilledNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IsExternalGraphSlotFilledNode>( context, options );
        context.SetNodePtrFromIndex( m_externalGraphNodeIdx, pNode->m_pExternalGraphNode );
    }

    void IsExternalGraphSlotFilledNode::InitializeInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        BoolValueNode::InitializeInternal( context );
        m_result = false;
    }

    void IsExternalGraphSlotFilledNode::ShutdownInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        BoolValueNode::ShutdownInternal( context );
    }

    void IsExternalGraphSlotFilledNode::GetValueInternal( GraphContext &context, void *pOutValue )
    {
        EE_ASSERT( context.IsValid() );

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            m_result = m_pExternalGraphNode->HasInstance();
            MarkNodeActive( context );
        }

        // Set Result
        *( (bool *) pOutValue ) = m_result;
    }
}
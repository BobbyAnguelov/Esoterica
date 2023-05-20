#include "Animation_RuntimeGraphNode_ChildGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "System/Log.h"

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
            auto pRootNode = const_cast<PoseNode*>( m_pGraphInstance->GetRootNode() );
            EE_ASSERT( !pRootNode->IsInitialized() );
            pRootNode->Initialize( context, initialTime );

            ReflectControlParameters( context );

            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = s_oneFrameDuration;
        }

        m_isFirstUpdate = true;
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

                case GraphValueType::Int:
                {
                    m_pGraphInstance->SetControlParameterValue( childParamIdx, pParameter->GetValue<int32_t>( context ) );
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

    void ChildGraphNode::TransferGraphInstanceData( GraphContext& context, GraphPoseNodeResult& result )
    {
        auto& localEventBuffer = context.m_sampledEventsBuffer;
        auto const& externalEventBuffer = m_pGraphInstance->GetSampledEvents();
        result.m_sampledEventRange = localEventBuffer.AppendBuffer( externalEventBuffer );

        auto pRootNode = m_pGraphInstance->GetRootNode();
        m_previousTime = pRootNode->GetCurrentTime();
        m_currentTime = pRootNode->GetCurrentTime();
        m_duration = pRootNode->GetDuration();

        #if EE_DEVELOPMENT_TOOLS
        context.GetRootMotionDebugger()->RecordGraphSource( GetNodeIndex(), result.m_rootMotionDelta );
        #endif
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult ChildGraphNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        if ( m_pGraphInstance == nullptr )
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }
        else
        {
            ReflectControlParameters( context );
            result = m_pGraphInstance->EvaluateGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, m_isFirstUpdate );
            m_isFirstUpdate = false;
            TransferGraphInstanceData( context, result );
        }
        return result;
    }

    GraphPoseNodeResult ChildGraphNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        if ( m_pGraphInstance == nullptr )
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }
        else
        {
            ReflectControlParameters( context );
            result = m_pGraphInstance->EvaluateGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, updateRange, m_isFirstUpdate );
            m_isFirstUpdate = false;
            TransferGraphInstanceData( context, result );
        }
        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ChildGraphNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        // Record the child graph initial state
        RecordedGraphState* pChildRecorder = outState.CreateChildGraphStateRecording( GetNodeIndex() );
        m_pGraphInstance->StartRecording( *pChildRecorder );
    }

    void ChildGraphNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        // Restore child graph state
        RecordedGraphState* pChildRecording = inState.GetChildGraphRecording( GetNodeIndex() );
        m_pGraphInstance->SetToRecordedState( *pChildRecording );
    }
    #endif
}
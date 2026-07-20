#include "Animation_RuntimeGraphNode_TwoBoneIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_TwoBoneIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void TwoBoneIKNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TwoBoneIKNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_effectorTargetNodeIdx, pNode->m_pEffectorTargetNode );
        context.SetOptionalNodePtrFromIndex( m_enableNodeIdx, pNode->m_pEnableNode );

        // Lookup effector IDs
        //-------------------------------------------------------------------------

        EE_ASSERT( m_effectorBoneID.IsValid() );
        pNode->m_effectorBoneIdx = context.m_pSkeleton->GetBoneIndex( m_effectorBoneID );

        // Validate setup
        //-------------------------------------------------------------------------

        pNode->m_isValidSetup = true;
        if ( pNode->m_effectorBoneIdx == InvalidIndex )
        {
            pNode->m_isValidSetup = false;
        }
        else
        {
            int32_t nParentCount = 0;
            int32_t nParentIdx = context.m_pSkeleton->GetParentBoneIndex( pNode->m_effectorBoneIdx );
            while ( nParentIdx != InvalidIndex && nParentCount < 2 )
            {
                nParentCount++;
                nParentIdx = context.m_pSkeleton->GetParentBoneIndex( nParentIdx );
            }

            if ( nParentCount != 2 )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( "Invalid effector bone ID ('%s') specified, there are not enough bones in the chain to solve the IK request", m_effectorBoneID.c_str() );
                #endif

                pNode->m_effectorBoneIdx = InvalidIndex;
                pNode->m_isValidSetup = false;
            }
        }
    }

    void TwoBoneIKNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pEffectorTargetNode->Initialize( context );

        auto pDefinition = GetDefinition<TwoBoneIKNode>();

        if ( m_pEnableNode != nullptr )
        {
            m_pEnableNode->Initialize( context );
            m_IKWeight.Reset( pDefinition->m_blendTime, m_pEnableNode->GetValue<bool>( context ) );
        }
        else
        {
            m_IKWeight.Reset( 0.0f, true );
        }
    }

    void TwoBoneIKNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pEnableNode != nullptr )
        {
            m_pEnableNode->Shutdown( context );
        }

        m_pEffectorTargetNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult TwoBoneIKNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        result = PassthroughNode::Update( context, pUpdateRange );

        if ( result.HasRegisteredTasks() && m_isValidSetup )
        {
            // Update IK Weight
            //-------------------------------------------------------------------------

            bool isIKEnabled = true;
            if ( m_pEnableNode != nullptr )
            {
                isIKEnabled = m_pEnableNode->GetValue<bool>( context );
            }

            float const weight = m_IKWeight.Update( context.m_deltaTime, isIKEnabled );

            // Register Task
            //-------------------------------------------------------------------------

            if ( weight > 0.0f )
            {
                Target const effectorTarget = m_pEffectorTargetNode->GetValue<Target>( context );
                if ( effectorTarget.IsTargetSet() )
                {
                    bool shouldRegisterTask = true;

                    // Validate input target
                    if ( effectorTarget.IsBoneTarget() )
                    {
                        StringID const effectorTargetBoneID = effectorTarget.GetBoneID();
                        if ( !effectorTargetBoneID.IsValid() )
                        {
                            #if EE_DEVELOPMENT_TOOLS
                            context.LogError( GetNodeIndex(), "No input target bone ID specified" );
                            #endif
                            shouldRegisterTask = false;
                        }

                        if( context.m_pSkeleton->GetBoneIndex( effectorTargetBoneID ) == InvalidIndex )
                        {
                            #if EE_DEVELOPMENT_TOOLS
                            context.LogError( GetNodeIndex(), "Invalid input target bone ID ('%s') specified", effectorTarget.GetBoneID().c_str() );
                            #endif
                            shouldRegisterTask = false;
                        }
                    }

                    if ( shouldRegisterTask )
                    {
                        auto pDefinition = GetDefinition<TwoBoneIKNode>();
                        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<TwoBoneIKTask>( GetNodePath( context ), result.m_taskIdx, m_effectorBoneIdx, pDefinition->m_isTargetInWorldSpace, effectorTarget, pDefinition->m_blendMode, weight, pDefinition->m_referencePoseTwistWeight );
                    }
                }
            }
        }

        return result;
    }

    void TwoBoneIKNode::RecordGraphState( RecordedGraphState &outState )
    {
        PassthroughNode::RecordGraphState( outState );

        outState.WriteValue( m_IKWeight.m_state );
        outState.WriteValue( m_IKWeight.m_blendTime );
    }

    bool TwoBoneIKNode::RestoreGraphState( RecordedGraphState const &inState )
    {
        if ( !PassthroughNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_IKWeight.m_blendTime );
        inState.ReadValue( m_IKWeight.m_state );
        return true;
    }
}
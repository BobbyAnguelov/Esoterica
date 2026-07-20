#include "Animation_RuntimeGraphNode_FootIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_FootIK.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_TwoBoneIK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void FootIKNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FootIKNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_leftTargetNodeIdx, pNode->m_pLeftTargetNode );
        context.SetNodePtrFromIndex( m_rightTargetNodeIdx, pNode->m_pRightTargetNode );
        context.SetOptionalNodePtrFromIndex( m_enableNodeIdx, pNode->m_pEnableNode );

        // Lookup effector IDs
        //-------------------------------------------------------------------------

        EE_ASSERT( m_leftFootBoneID.IsValid() );
        pNode->m_leftFootBoneIdx = context.m_pSkeleton->GetBoneIndex( m_leftFootBoneID );

        EE_ASSERT( m_rightFootBoneID.IsValid() );
        pNode->m_rightFootBoneIdx = context.m_pSkeleton->GetBoneIndex( m_rightFootBoneID );

        // Validate setup
        //-------------------------------------------------------------------------

        pNode->m_isValidSetup = true;
        if ( pNode->m_leftFootBoneIdx == InvalidIndex || pNode->m_rightFootBoneIdx == InvalidIndex )
        {
            pNode->m_isValidSetup = false;
        }
        else
        {
            int32_t parentCount = 0;
            int32_t parentIdx = context.m_pSkeleton->GetParentBoneIndex( pNode->m_leftFootBoneIdx );
            while ( parentIdx != InvalidIndex && parentCount < 2 )
            {
                parentCount++;
                parentIdx = context.m_pSkeleton->GetParentBoneIndex( parentIdx );
            }

            if ( parentCount != 2 )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( "Invalid left effector bone ID ('%s') specified, there are not enough bones in the chain to solve the IK request", m_leftFootBoneID.c_str() );
                #endif

                pNode->m_leftFootBoneIdx = InvalidIndex;
                pNode->m_isValidSetup = false;
            }

            //-------------------------------------------------------------------------

            parentCount = 0;
            parentIdx = context.m_pSkeleton->GetParentBoneIndex( pNode->m_rightFootBoneIdx );
            while ( parentIdx != InvalidIndex && parentCount < 2 )
            {
                parentCount++;
                parentIdx = context.m_pSkeleton->GetParentBoneIndex( parentIdx );
            }

            if ( parentCount != 2 )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( "Invalid right effector bone ID ('%s') specified, there are not enough bones in the chain to solve the IK request", m_rightFootBoneID.c_str() );
                #endif

                pNode->m_rightFootBoneIdx = InvalidIndex;
                pNode->m_isValidSetup = false;
            }
        }
    }

    void FootIKNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pLeftTargetNode->Initialize( context );
        m_pRightTargetNode->Initialize( context );

        auto pDefinition = GetDefinition<FootIKNode>();

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

    void FootIKNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pEnableNode != nullptr )
        {
            m_pEnableNode->Shutdown( context );
        }

        m_pLeftTargetNode->Shutdown( context );
        m_pRightTargetNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult FootIKNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        result = PassthroughNode::Update( context, pUpdateRange );

        if ( result.HasRegisteredTasks() && m_isValidSetup )
        {
            auto pDefinition = GetDefinition<FootIKNode>();

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

            auto ValidateTarget = [&] ( GraphContext &context, Target const &target )
            {
                if ( target.IsBoneTarget() )
                {
                    StringID const effectorTargetBoneID = target.GetBoneID();
                    if ( !effectorTargetBoneID.IsValid() )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogError( GetNodeIndex(), "No effector bone ID specified in input target" );
                        #endif

                        return false;
                    }

                    if ( context.m_pSkeleton->GetBoneIndex( effectorTargetBoneID ) == InvalidIndex )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogError( GetNodeIndex(), "Invalid effector bone ID ('%s') specified in input target", target.GetBoneID().c_str() );
                        #endif

                        return false;
                    }
                }

                return true;
            };

            if ( weight > 0.0f )
            {
                Target const leftTarget = m_pLeftTargetNode->GetValue<Target>( context );
                Target const rightTarget = m_pRightTargetNode->GetValue<Target>( context );

                if ( leftTarget.IsTargetSet() && rightTarget.IsTargetSet() )
                {
                    if ( ValidateTarget( context, leftTarget ) && ValidateTarget( context, rightTarget ) )
                    {
                        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<FootIKTask>( GetNodePath( context ), result.m_taskIdx, m_leftFootBoneIdx, m_rightFootBoneIdx, pDefinition->m_isTargetInWorldSpace, leftTarget, rightTarget, pDefinition->m_blendMode, weight );
                    }
                }
                else if ( leftTarget.IsTargetSet() )
                {
                    if ( ValidateTarget( context, leftTarget ) )
                    {
                        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<TwoBoneIKTask>( GetNodePath( context ), result.m_taskIdx, m_leftFootBoneIdx, pDefinition->m_isTargetInWorldSpace, leftTarget, pDefinition->m_blendMode, weight, 0.0f );
                    }
                }
                else if ( rightTarget.IsTargetSet() )
                {
                    if ( ValidateTarget( context, rightTarget ) )
                    {
                        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<TwoBoneIKTask>( GetNodePath( context ), result.m_taskIdx, m_rightFootBoneIdx, pDefinition->m_isTargetInWorldSpace, rightTarget, pDefinition->m_blendMode, weight, 0.0f );
                    }
                }
            }
        }

        return result;
    }

    void FootIKNode::RecordGraphState( RecordedGraphState &outState )
    {
        PassthroughNode::RecordGraphState( outState );

        outState.WriteValue( m_IKWeight.m_state );
        outState.WriteValue( m_IKWeight.m_blendTime );
    }

    bool FootIKNode::RestoreGraphState( RecordedGraphState const &inState )
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
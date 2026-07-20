#include "Animation_RuntimeGraphNode_Scale.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Scale.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ScaleNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ScaleNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_maskNodeIdx, pNode->m_pMaskNode );
        context.SetOptionalNodePtrFromIndex( m_enableNodeIdx, pNode->m_pEnableNode );
    }

    void ScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pMaskNode->Initialize( context );

        if ( m_pEnableNode != nullptr )
        {
            m_pEnableNode->Initialize( context );
        }
    }

    void ScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pEnableNode != nullptr )
        {
            m_pEnableNode->Shutdown( context );
        }

        m_pMaskNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult ScaleNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        result = PassthroughNode::Update( context, pUpdateRange );

        if ( result.HasRegisteredTasks() && m_pMaskNode->IsValid() )
        {
            bool const bIsEnabled = ( m_pEnableNode != nullptr ) ? m_pEnableNode->GetValue<bool>( context ) : true;
            if ( bIsEnabled )
            {
                BoneMaskTaskList const* pBoneMaskTaskList = m_pMaskNode->GetValue<BoneMaskTaskList const*>( context );
                EE_ASSERT( pBoneMaskTaskList != nullptr );
                if ( pBoneMaskTaskList->HasTasks() )
                {
                    result.m_taskIdx = context.GetTaskSystem()->RegisterTask<ScaleTask>( GetNodePath( context ), result.m_taskIdx, *pBoneMaskTaskList );
                }
            }
        }

        return result;
    }
}

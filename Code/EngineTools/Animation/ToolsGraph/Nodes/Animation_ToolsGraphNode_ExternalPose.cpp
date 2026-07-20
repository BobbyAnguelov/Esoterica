#include "Animation_ToolsGraphNode_ExternalPose.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ExternalPose.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    ExternalPoseToolsNode::ExternalPoseToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        Rename( "External Pose" );
    }

    int16_t ExternalPoseToolsNode::Compile( GraphCompilationContext &context ) const
    {
        ExternalPoseNode::Definition *pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ExternalPoseNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            context.RegisterExternalPoseSlotNode( pDefinition->m_nodeIdx, StringID( GetName() ) );
        }

        return pDefinition->m_nodeIdx;
    }

    String ExternalPoseToolsNode::CreateUniqueNodeName( String const &desiredName ) const
    {
        String uniqueName = desiredName;

        if ( HasParentGraph() )
        {
            // Ensure that the slot name is unique within the same graph
            int32_t cnt = 0;
            auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalPoseToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Exact );
            for ( int32_t i = 0; i < externalSlotNodes.size(); i++ )
            {
                if ( externalSlotNodes[i] == this )
                {
                    continue;
                }

                if ( externalSlotNodes[i]->GetName() == uniqueName )
                {
                    uniqueName = String( String::CtorSprintf(), "%s_%d", desiredName.c_str(), cnt );
                    cnt++;
                    i = -1;
                }
            }
        }

        return uniqueName;
    }

    //-------------------------------------------------------------------------

    IsExternalPoseSetToolsNode::IsExternalPoseSetToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
    }

    int16_t IsExternalPoseSetToolsNode::Compile( GraphCompilationContext &context ) const
    {
        IsExternalPoseSetNode::Definition *pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IsExternalPoseSetNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !m_slotID.IsValid() )
            {
                context.LogError( "Invalid external slot ID" );
                return false;
            }

            bool bExternalSlotNodeFound = false;

            auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalPoseToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Exact );
            for ( int32_t i = 0; i < externalSlotNodes.size(); i++ )
            {
                if ( StringID( externalSlotNodes[i]->GetName() ) == m_slotID )
                {
                    int16_t const compiledNodeIdx = externalSlotNodes[i]->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_externalPoseNodeIdx = compiledNodeIdx;
                    }
                    else
                    {
                        return InvalidIndex;
                    }

                    bExternalSlotNodeFound = true;
                    break;
                }
            }

            if ( !bExternalSlotNodeFound )
            {
                context.LogError( "Invalid external slot ID: %s", m_slotID.c_str() );
                return InvalidIndex;
            }
        }

        return pDefinition->m_nodeIdx;
    }
}
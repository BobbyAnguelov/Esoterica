#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationClipToolsNode::AnimationClipToolsNode()
        :VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Play In Reverse", GraphValueType::Bool );
        CreateInputPin( "Reset Time", GraphValueType::Bool );
    }

    int16_t AnimationClipToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationClipNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<AnimationClipNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pShouldPlayInReverseNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pShouldPlayInReverseNode != nullptr )
            {
                auto compiledNodeIdx = pShouldPlayInReverseNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_playInReverseValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            auto pResetTimeNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pResetTimeNode != nullptr )
            {
                auto compiledNodeIdx = pResetTimeNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_resetTimeValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            pDefinition->m_dataSlotIdx = context.RegisterResource( pData->m_animClip.GetResourceID() );
            pDefinition->m_speedMultiplier = pData->m_speedMultiplier;
            pDefinition->m_startSyncEventOffset = pData->m_startSyncEventOffset;
            pDefinition->m_sampleRootMotion = m_sampleRootMotion;
            pDefinition->m_allowLooping = m_allowLooping;

            for ( auto const &ID : m_graphEvents )
            {
                if ( ID.IsValid() )
                {
                    pDefinition->m_graphEvents.emplace_back( ID );
                }
            }
        }
        return pDefinition->m_nodeIdx;
    }

    void AnimationClipToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        FlowToolsNode::DrawExtraControls( ctx, pUserContext );

        //-------------------------------------------------------------------------

        bool hasValidEvents = false;

        for ( auto const &ID : m_graphEvents )
        {
            if ( ID.IsValid() )
            {
                hasValidEvents = true;
                break;
            }
        }

        if ( !hasValidEvents )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Graph Events" );

        for ( auto const &ID : m_graphEvents )
        {
            if ( ID.IsValid() )
            {
                ImGui::BulletText( ID.c_str() );
            }
        }
    }
}
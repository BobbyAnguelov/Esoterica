#include "Animation_ToolsGraphNode_TimeControlledAnimationClip.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TimeControlledAnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TimeControlledAnimationClipToolsNode::TimeControlledAnimationClipToolsNode()
        :VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Time", GraphValueType::Float );
        CreateInputPin( "Play In Reverse", GraphValueType::Bool );
    }

    int16_t TimeControlledAnimationClipToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TimeControlledAnimationClipNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TimeControlledAnimationClipNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            FlowToolsNode const* pTimeNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pTimeNode != nullptr )
            {
                int16_t compiledNodeIdx = pTimeNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_timeValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( "Time value must be connected for time controlled clip node" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pShouldPlayInReverseNode = GetConnectedInputNode<FlowToolsNode>( 1 );
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

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            pDefinition->m_dataSlotIdx = context.RegisterResource( pData->m_animClip.GetResourceID() );
            pDefinition->m_sampleRootMotion = m_sampleRootMotion;

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

    void TimeControlledAnimationClipToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
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
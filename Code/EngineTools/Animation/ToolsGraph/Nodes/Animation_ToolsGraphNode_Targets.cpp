#include "Animation_ToolsGraphNode_Targets.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    IsTargetSetToolsNode::IsTargetSetToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Target", GraphValueType::Target );
    }

    int16_t IsTargetSetToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IsTargetSetNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IsTargetSetNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    TargetInfoToolsNode::TargetInfoToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Target", GraphValueType::Target );
    }

    int16_t TargetInfoToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TargetInfoNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TargetInfoNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_isWorldSpaceTarget = m_isWorldSpaceTarget;
            pDefinition->m_infoType = m_infoType;
        }
        return pDefinition->m_nodeIdx;
    }

    void TargetInfoToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        InlineString infoText;
        
        if ( m_isWorldSpaceTarget )
        {
            infoText += "World Space - ";
        }
        else
        {
            infoText += "Character Space - ";
        }

        //-------------------------------------------------------------------------

        switch ( m_infoType )
        {
            case TargetInfoNode::Info::AngleHorizontal:
            {
                infoText += "Horizontal Angle";
            }
            break;

            case TargetInfoNode::Info::AngleVertical:
            {
                infoText += "Vertical Angle";
            }
            break;

            case TargetInfoNode::Info::Distance:
            {
                infoText += "Distance";
            }
            break;

            case TargetInfoNode::Info::DistanceHorizontalOnly:
            {
                infoText += "Horizontal Distance";
            }
            break;

            case TargetInfoNode::Info::DistanceVerticalOnly:
            {
                infoText += "Vertical Distance";
            }
            break;

            case TargetInfoNode::Info::DeltaOrientationX:
            {
                infoText += "Rotation Angle X";
            }
            break;

            case TargetInfoNode::Info::DeltaOrientationY:
            {
                infoText += "Rotation Angle Y";
            }
            break;

            case TargetInfoNode::Info::DeltaOrientationZ:
            {
                infoText += "Rotation Angle Z";
            }
            break;
        }

        ImGui::Text( infoText.c_str() );
    }

    //-------------------------------------------------------------------------

     TargetOffsetToolsNode::TargetOffsetToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Target, true );
        CreateInputPin( "Target", GraphValueType::Target );
    }

    int16_t TargetOffsetToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TargetOffsetNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TargetOffsetNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_isBoneSpaceOffset = m_isBoneSpaceOffset;
            pDefinition->m_rotationOffset = m_rotationOffset;
            pDefinition->m_translationOffset = m_translationOffset;
        }
        return pDefinition->m_nodeIdx;
    }
}
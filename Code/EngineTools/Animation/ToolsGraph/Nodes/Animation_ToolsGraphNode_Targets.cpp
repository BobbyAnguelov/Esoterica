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
        IsTargetSetNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<IsTargetSetNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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
        return pSettings->m_nodeIdx;
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
        TargetInfoNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<TargetInfoNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_isWorldSpaceTarget = m_isWorldSpaceTarget;
            pSettings->m_infoType = m_infoType;
        }
        return pSettings->m_nodeIdx;
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
        TargetOffsetNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<TargetOffsetNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
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

            pSettings->m_isBoneSpaceOffset = m_isBoneSpaceOffset;
            pSettings->m_rotationOffset = m_rotationOffset;
            pSettings->m_translationOffset = m_translationOffset;
        }
        return pSettings->m_nodeIdx;
    }
}
#include "Animation_ToolsGraphNode_Vectors.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    VectorInfoToolsNode::VectorInfoToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Vector", GraphValueType::Vector );
    }

    int16_t VectorInfoToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VectorInfoNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VectorInfoNode>( this, pSettings );
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

            pSettings->m_desiredInfo = m_desiredInfo;
        }
        return pSettings->m_nodeIdx;
    }

    void VectorInfoToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        switch ( m_desiredInfo )
        {
            case VectorInfoNode::Info::X:
            ImGui::Text( "X" );
            break;

            case VectorInfoNode::Info::Y:
            ImGui::Text( "Y" );
            break;

            case VectorInfoNode::Info::Z:
            ImGui::Text( "Z" );
            break;

            case VectorInfoNode::Info::W:
            ImGui::Text( "W" );
            break;

            case VectorInfoNode::Info::Length:
            ImGui::Text( "Length" );
            break;

            case VectorInfoNode::Info::AngleHorizontal:
            ImGui::Text( "Angle Horizontal" );
            break;

            case VectorInfoNode::Info::AngleVertical:
            ImGui::Text( "Angle Vertical" );
            break;

        }
    }

    //-------------------------------------------------------------------------

    VectorCreateToolsNode::VectorCreateToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Vector, true );
        CreateInputPin( "Vector", GraphValueType::Vector );
        CreateInputPin( "X", GraphValueType::Float );
        CreateInputPin( "Y", GraphValueType::Float );
        CreateInputPin( "Z", GraphValueType::Float );
    }

    int16_t VectorCreateToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VectorCreateNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VectorCreateNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputVectorValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueXNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueYNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 3 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueZNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
        }
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    VectorNegateToolsNode::VectorNegateToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Vector, true );
        CreateInputPin( "Vector", GraphValueType::Vector );
    }

    int16_t VectorNegateToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VectorNegateNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VectorNegateNode>( this, pSettings );
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
}
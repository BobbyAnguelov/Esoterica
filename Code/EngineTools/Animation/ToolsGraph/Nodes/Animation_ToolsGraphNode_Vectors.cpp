#include "Animation_ToolsGraphNode_Vectors.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    VectorInfoToolsNode::VectorInfoToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Vector", GraphValueType::Vector );
    }

    int16_t VectorInfoToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VectorInfoNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<VectorInfoNode>( this, pDefinition );
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

            pDefinition->m_desiredInfo = m_desiredInfo;
        }
        return pDefinition->m_nodeIdx;
    }

    void VectorInfoToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
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
        VectorCreateNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<VectorCreateNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputVectorValueNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_inputValueXNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_inputValueYNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_inputValueZNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
        }
        return pDefinition->m_nodeIdx;
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
        VectorNegateNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<VectorNegateNode>( this, pDefinition );
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
}
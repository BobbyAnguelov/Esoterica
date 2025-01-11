#include "Animation_ToolsGraphNode_CachedValues.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    CachedBoolToolsNode::CachedBoolToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Value", GraphValueType::Bool );
    }

    int16_t CachedBoolToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CachedBoolNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<CachedBoolNode>( this, pDefinition );
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

            pDefinition->m_mode = m_mode;
        }

        return pDefinition->m_nodeIdx;
    }

    void CachedBoolToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    CachedIDToolsNode::CachedIDToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::ID, true );
        CreateInputPin( "Value", GraphValueType::ID );
    }

    int16_t CachedIDToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CachedIDNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<CachedIDNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
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

            pDefinition->m_mode = m_mode;
        }
        return pDefinition->m_nodeIdx;
    }

    void CachedIDToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    CachedFloatToolsNode::CachedFloatToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Value", GraphValueType::Float );
    }

    int16_t CachedFloatToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CachedFloatNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<CachedFloatNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
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

            pDefinition->m_mode = m_mode;
        }
        return pDefinition->m_nodeIdx;
    }

    void CachedFloatToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    CachedVectorToolsNode::CachedVectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Vector, true );
        CreateInputPin( "Value", GraphValueType::Vector );
    }

    int16_t CachedVectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CachedVectorNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<CachedVectorNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
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

            pDefinition->m_mode = m_mode;
        }
        return pDefinition->m_nodeIdx;
    }

    void CachedVectorToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    CachedTargetToolsNode::CachedTargetToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Target, true );
        CreateInputPin( "Value", GraphValueType::Target );
    }

    int16_t CachedTargetToolsNode::Compile( GraphCompilationContext& context ) const
    {
        CachedTargetNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<CachedTargetNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
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

            pDefinition->m_mode = m_mode;
        }
        return pDefinition->m_nodeIdx;
    }

    void CachedTargetToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }
}
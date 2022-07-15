#include "Animation_EditorGraphNode_CachedValues.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void CachedBoolEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "Value", GraphValueType::Bool );
    }

    int16_t CachedBoolEditorNode::Compile( GraphCompilationContext& context ) const
    {
        CachedBoolNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<CachedBoolNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_mode = m_mode;
        }

        return pSettings->m_nodeIdx;
    }

    void CachedBoolEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    void CachedIDEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::ID, true );
        CreateInputPin( "Value", GraphValueType::ID );
    }

    int16_t CachedIDEditorNode::Compile( GraphCompilationContext& context ) const
    {
        CachedIDNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<CachedIDNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_mode = m_mode;
        }
        return pSettings->m_nodeIdx;
    }

    void CachedIDEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    void CachedIntEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Int, true );
        CreateInputPin( "Value", GraphValueType::Int );
    }

    int16_t CachedIntEditorNode::Compile( GraphCompilationContext& context ) const
    {
        CachedIntNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<CachedIntNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_mode = m_mode;
        }
        return pSettings->m_nodeIdx;
    }

    void CachedIntEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    void CachedFloatEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "Value", GraphValueType::Float );
    }

    int16_t CachedFloatEditorNode::Compile( GraphCompilationContext& context ) const
    {
        CachedFloatNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<CachedFloatNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_mode = m_mode;
        }
        return pSettings->m_nodeIdx;
    }

    void CachedFloatEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    void CachedVectorEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Vector, true );
        CreateInputPin( "Value", GraphValueType::Vector );
    }

    int16_t CachedVectorEditorNode::Compile( GraphCompilationContext& context ) const
    {
        CachedVectorNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<CachedVectorNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_mode = m_mode;
        }
        return pSettings->m_nodeIdx;
    }

    void CachedVectorEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }

    //-------------------------------------------------------------------------

    void CachedTargetEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Target, true );
        CreateInputPin( "Value", GraphValueType::Target );
    }

    int16_t CachedTargetEditorNode::Compile( GraphCompilationContext& context ) const
    {
        CachedTargetNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<CachedTargetNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pSettings->m_mode = m_mode;
        }
        return pSettings->m_nodeIdx;
    }

    void CachedTargetEditorNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_mode == CachedValueMode::OnEntry ? "Cache On Entry" : "Cache On Exit" );
    }
}
#include "Animation_EditorGraphNode_ConstValues.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ConstValues.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ConstBoolEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    int16_t ConstBoolEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ConstBoolNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ConstBoolNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstBoolEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_value ? "True" : "False" );
    }

    //-------------------------------------------------------------------------

    void ConstIDEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    int16_t ConstIDEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ConstIDNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstIDNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstIDEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( m_value.c_str() );
    }

    //-------------------------------------------------------------------------

    void ConstIntEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Int, true );
    }

    int16_t ConstIntEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ConstIntNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstIntNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstIntEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( "%d", m_value );
    }

    //-------------------------------------------------------------------------

    void ConstFloatEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    int16_t ConstFloatEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ConstFloatNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstFloatNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstFloatEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( "%.3f", m_value );
    }

    //-------------------------------------------------------------------------

    void ConstVectorEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    int16_t ConstVectorEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ConstVectorNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstVectorNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstVectorEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( "X: %.2f, Y: %.2f, Z: %.2f, W: %.2f", m_value.m_x, m_value.m_y, m_value.m_z, m_value.m_w );
    }

    //-------------------------------------------------------------------------

    void ConstTargetEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    int16_t ConstTargetEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ConstTargetNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstTargetNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = Target( m_value );
        }
        return pSettings->m_nodeIdx;
    }

    void ConstTargetEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        ImGui::Text( "Transform - TODO" );
    }
}
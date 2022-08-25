#include "Animation_ToolsGraphNode_ConstValues.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ConstValues.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ConstBoolToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    int16_t ConstBoolToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstBoolNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ConstBoolNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstBoolToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_value ? "True" : "False" );
    }

    //-------------------------------------------------------------------------

    void ConstIDToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    int16_t ConstIDToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstIDNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstIDNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstIDToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        ImGui::Text( m_value.c_str() );
    }

    //-------------------------------------------------------------------------

    void ConstIntToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Int, true );
    }

    int16_t ConstIntToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstIntNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstIntNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstIntToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        ImGui::Text( "%d", m_value );
    }

    //-------------------------------------------------------------------------

    void ConstFloatToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    int16_t ConstFloatToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstFloatNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstFloatNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstFloatToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        ImGui::Text( "%.3f", m_value );
    }

    //-------------------------------------------------------------------------

    void ConstVectorToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    int16_t ConstVectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstVectorNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstVectorNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = m_value;
        }
        return pSettings->m_nodeIdx;
    }

    void ConstVectorToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        ImGui::Text( "X: %.2f, Y: %.2f, Z: %.2f, W: %.2f", m_value.m_x, m_value.m_y, m_value.m_z, m_value.m_w );
    }

    //-------------------------------------------------------------------------

    void ConstTargetToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    int16_t ConstTargetToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstTargetNode::Settings* pSettings = nullptr;
        if ( context.GetSettings<ConstTargetNode>( this, pSettings ) == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_value = Target( m_value );
        }
        return pSettings->m_nodeIdx;
    }

    void ConstTargetToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        ImGui::Text( "Transform - TODO" );
    }
}
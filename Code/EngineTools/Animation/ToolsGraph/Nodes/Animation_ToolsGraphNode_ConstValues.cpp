#include "Animation_ToolsGraphNode_ConstValues.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ConstValues.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ConstBoolToolsNode::ConstBoolToolsNode()
        : FlowToolsNode()
    {
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
        BeginDrawInternalRegion( ctx, Color( 40, 40, 40 ) );
        ImGui::Text( m_value ? "True" : "False" );
        EndDrawInternalRegion( ctx );
    }

    //-------------------------------------------------------------------------

    ConstIDToolsNode::ConstIDToolsNode()
        : FlowToolsNode()
    {
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
        BeginDrawInternalRegion( ctx, Color( 40, 40, 40 ) );
        ImGui::Text( m_value.c_str() );
        EndDrawInternalRegion( ctx );
    }

    //-------------------------------------------------------------------------

    ConstFloatToolsNode::ConstFloatToolsNode()
        : FlowToolsNode()
    {
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
        BeginDrawInternalRegion( ctx, Color( 40, 40, 40 ) );
        ImGui::Text( "%.3f", m_value );
        EndDrawInternalRegion( ctx );
    }

    //-------------------------------------------------------------------------

    ConstVectorToolsNode::ConstVectorToolsNode()
        : FlowToolsNode()
    {
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
        BeginDrawInternalRegion( ctx, Color( 40, 40, 40 ) );
        DrawVectorInfoText( ctx, m_value );
        EndDrawInternalRegion( ctx );
    }

    //-------------------------------------------------------------------------

    ConstTargetToolsNode::ConstTargetToolsNode()
        : FlowToolsNode()
    {
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
        BeginDrawInternalRegion( ctx, Color( 40, 40, 40 ) );
        DrawTargetInfoText( ctx, Target( m_value ) );
        EndDrawInternalRegion( ctx );
    }
}
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
        ConstBoolNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ConstBoolNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_value = m_value;
        }
        return pDefinition->m_nodeIdx;
    }

    void ConstBoolToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );
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
        ConstIDNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<ConstIDNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_value = m_value;
        }
        return pDefinition->m_nodeIdx;
    }

    void ConstIDToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );
        ImGui::Text( m_value.IsValid() ? m_value.c_str() : "" );
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
        ConstFloatNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<ConstFloatNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_value = m_value;
        }
        return pDefinition->m_nodeIdx;
    }

    void ConstFloatToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );
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
        ConstVectorNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<ConstVectorNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_value = m_value;
        }
        return pDefinition->m_nodeIdx;
    }

    void ConstVectorToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );
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
        ConstTargetNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<ConstTargetNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_value = Target( m_value );
        }
        return pDefinition->m_nodeIdx;
    }

    void ConstTargetToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );
        DrawTargetInfoText( ctx, Target( m_value ) );
        EndDrawInternalRegion( ctx );
    }

    //-------------------------------------------------------------------------

    ConstBoneTargetToolsNode::ConstBoneTargetToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    int16_t ConstBoneTargetToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ConstTargetNode::Definition* pDefinition = nullptr;
        if ( context.GetDefinition<ConstTargetNode>( this, pDefinition ) == NodeCompilationState::NeedCompilation )
        {
            pDefinition->m_value = Target( m_boneName );
        }
        return pDefinition->m_nodeIdx;
    }

    void ConstBoneTargetToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );
        DrawTargetInfoText( ctx, Target( m_boneName ) );
        EndDrawInternalRegion( ctx );
    }
}
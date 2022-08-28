#include "Animation_ToolsGraphNode_Transition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void TransitionToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

        CreateInputPin( "Condition", GraphValueType::Bool );
        CreateInputPin( "Duration Override", GraphValueType::Float );
        CreateInputPin( "Sync Event Override", GraphValueType::Float );
    }

    void TransitionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        float const separatorWidth = GetWidth() == 0 ? 20 : GetWidth();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
        ImVec2 const cursorScreenPos = ctx.WindowToScreenPosition( ImGui::GetCursorPos() );
        ctx.m_pDrawList->AddLine( cursorScreenPos, cursorScreenPos + ImVec2( separatorWidth, 0 ), ImGuiX::Style::s_colorText );
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );

        //-------------------------------------------------------------------------

        switch ( m_rootMotionBlend )
        {
            case RootMotionBlendMode::Blend:
            ImGui::Text( "Blend Root Motion" );
            break;

            case RootMotionBlendMode::IgnoreSource:
            ImGui::Text( "Ignore Source Root Motion" );
            break;

            case RootMotionBlendMode::IgnoreTarget:
            ImGui::Text( "Ignore Target Root Motion" );
            break;
        }

        //-------------------------------------------------------------------------

        ImGui::Text( "Duration: %.2fs", m_duration.ToFloat() );

        if ( m_clampDurationToSource )
        {
            ImGui::Text( "Clamped To Source" );
        }

        //-------------------------------------------------------------------------

        if ( m_isSynchronized )
        {
            ImGui::Text( "Synchronized" );
        }
        else
        {
            if ( m_keepSourceSyncEventIdx && m_keepSourceSyncEventPercentageThrough )
            {
                ImGui::Text( "Match Sync Event and Percentage" );
            }
            else if ( m_keepSourceSyncEventIdx )
            {
                ImGui::Text( "Match Sync Event" );
            }
            else if ( m_keepSourceSyncEventPercentageThrough )
            {
                ImGui::Text( "Match Sync Percentage" );
            }
        }

        ImGui::Text( "Sync Offset: %.2f", m_syncEventOffset );

        //-------------------------------------------------------------------------

        if ( m_canBeForced )
        {
            ImGui::Text( "Forced" );
        }
    }

    //-------------------------------------------------------------------------

    void TransitionConduitToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::TransitionConduit::Initialize( pParent );
        SetSecondaryGraph( EE::New<FlowGraph>( GraphType::TransitionTree ) );
    }

    bool TransitionConduitToolsNode::HasTransitions() const
    {
        return !GetSecondaryGraph()->FindAllNodesOfType<TransitionToolsNode>().empty();
    }

    ImColor TransitionConduitToolsNode::GetNodeBorderColor( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::NodeVisualState visualState ) const
    {
        // Is this an blocked transition
        if ( visualState == VisualGraph::NodeVisualState::None && !HasTransitions() )
        {
            return VisualGraph::VisualSettings::s_connectionColorInvalid;
        }

        // Is this transition active?
        auto pGraphNodeContext = reinterpret_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            bool isActive = false;
            auto childTransitions = GetSecondaryGraph()->FindAllNodesOfType<TransitionToolsNode>();

            for ( auto pTransition : childTransitions )
            {
                auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( pTransition->GetID() );
                if ( runtimeNodeIdx != InvalidIndex && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
                {
                    isActive = true;
                    break;
                }
            }

            if ( isActive )
            {
                return VisualGraph::VisualSettings::s_connectionColorValid;
            }
        }

        //-------------------------------------------------------------------------

        return VisualGraph::SM::TransitionConduit::GetNodeBorderColor( ctx, pUserContext, visualState );
    }
}
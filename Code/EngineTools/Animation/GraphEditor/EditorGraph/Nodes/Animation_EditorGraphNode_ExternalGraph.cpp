#include "Animation_EditorGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ExternalGraphEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
        m_name = GetUniqueSlotName( "External Graph" );
    }

    int16_t ExternalGraphEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ExternalGraphNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ExternalGraphNode>( this, pSettings );
        context.RegisterExternalGraphSlotNode( pSettings->m_nodeIdx, StringID( GetName() ) );
        return pSettings->m_nodeIdx;
    }

    void ExternalGraphEditorNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenamable() );
        m_name = GetUniqueSlotName( newName );
    }

    void ExternalGraphEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        // Draw separator
        //-------------------------------------------------------------------------

        float const spacerWidth = Math::Max( GetSize().x, 40.0f );
        ImVec2 originalCursorPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton( "S1", ImVec2( spacerWidth, 10 ) );
        ctx.m_pDrawList->AddLine( originalCursorPos + ImVec2( 0, 4 ), originalCursorPos + ImVec2( GetSize().x, 4 ), ImColor( ImGuiX::Style::s_colorTextDisabled ) );

        //-------------------------------------------------------------------------

        auto pGraphNodeContext = reinterpret_cast<EditorGraphNodeContext*>( ctx.m_pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                StringID const SlotID( GetName() );
                GraphInstance const* pConnectedGraphInstance = pGraphNodeContext->m_pGraphInstance->GetExternalGraphDebugInstance( SlotID );
                if ( pConnectedGraphInstance != nullptr )
                {
                    ImGui::Text( EE_ICON_NEEDLE" %s", pConnectedGraphInstance->GetGraphDefinitionID().c_str() + 7 );
                }
                else
                {
                    ImGui::Text( EE_ICON_NEEDLE_OFF " Empty" );
                }
            }
        }
        else // Simple label when editing
        {
            ImGui::Text( EE_ICON_NEEDLE" External Graph" );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );

        //-------------------------------------------------------------------------

        originalCursorPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton( "S2", ImVec2( spacerWidth, 10 ) );
        ctx.m_pDrawList->AddLine( originalCursorPos + ImVec2( 0, 4 ), originalCursorPos + ImVec2( GetSize().x, 4 ), ImColor( ImGuiX::Style::s_colorTextDisabled ) );

        //-------------------------------------------------------------------------

        EditorGraphNode::DrawExtraControls( ctx );
    }

    String ExternalGraphEditorNode::GetUniqueSlotName( String const& desiredName )
    {
        // Ensure that the slot name is unique within the same graph
        int32_t cnt = 0;
        String uniqueName = desiredName;
        auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalGraphEditorNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( int32_t i = 0; i < (int32_t) externalSlotNodes.size(); i++ )
        {
            if ( externalSlotNodes[i] == this )
            {
                continue;
            }

            if ( externalSlotNodes[i]->GetName() == uniqueName )
            {
                uniqueName = String( String::CtorSprintf(), "%s_%d", desiredName.c_str(), cnt );
                cnt++;
                i = 0;
            }
        }

        return uniqueName;
    }

    void ExternalGraphEditorNode::PostPaste()
    {
        EditorGraphNode::PostPaste();
        m_name = GetUniqueSlotName( m_name );
    }

    #if EE_DEVELOPMENT_TOOLS
    void ExternalGraphEditorNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        EditorGraphNode::PostPropertyEdit( pPropertyEdited );
        m_name = GetUniqueSlotName( m_name );
    }
    #endif

}
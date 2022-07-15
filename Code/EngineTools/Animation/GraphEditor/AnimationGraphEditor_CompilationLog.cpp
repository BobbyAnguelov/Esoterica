#include "AnimationGraphEditor_CompilationLog.h"
#include "AnimationGraphEditor_Context.h"
#include "EditorGraph/Animation_EditorGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphCompilationLog::GraphCompilationLog( GraphEditorContext& editorContext )
        : m_editorContext( editorContext )
    {}

    void GraphCompilationLog::UpdateAndDraw( UpdateContext const& context, GraphNodeContext* pGraphNodeContext, ImGuiWindowClass* pWindowClass, char const* pWindowName )
    {
        int32_t windowFlags = 0;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( pWindowName, nullptr, windowFlags ) )
        {
            if ( m_compilationLog.empty() )
            {
                ImGui::TextColored( ImGuiX::ConvertColor( Colors::LimeGreen ), EE_ICON_CHECK );
                ImGui::SameLine();
                ImGui::Text( "Graph Compiled Successfully" );
            }
            else
            {
                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 6 ) );
                if ( ImGui::BeginTable( "Compilation Log Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( 0, 0 ) ) )
                {
                    ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
                    ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupScrollFreeze( 0, 1 );

                    //-------------------------------------------------------------------------

                    ImGui::TableHeadersRow();

                    //-------------------------------------------------------------------------

                    ImGuiListClipper clipper;
                    clipper.Begin( (int32_t) m_compilationLog.size() );
                    while ( clipper.Step() )
                    {
                        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                        {
                            auto const& entry = m_compilationLog[i];

                            ImGui::TableNextRow();

                            //-------------------------------------------------------------------------

                            ImGui::TableSetColumnIndex( 0 );
                            switch ( entry.m_severity )
                            {
                                case Log::Severity::Warning:
                                ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_ALERT_OCTAGON );
                                break;

                                case Log::Severity::Error:
                                ImGui::TextColored( Colors::Red.ToFloat4(), EE_ICON_ALERT );
                                break;

                                case Log::Severity::Message:
                                ImGui::Text( "Message" );
                                break;
                            }

                            //-------------------------------------------------------------------------

                            ImGui::TableSetColumnIndex( 1 );
                            if ( ImGui::Selectable( entry.m_message.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick ) )
                            {
                                if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                                {
                                    m_editorContext.NavigateTo( entry.m_nodeID );
                                }
                            }
                        }
                    }

                    // Auto scroll the table
                    if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                    {
                        ImGui::SetScrollHereY( 1.0f );
                    }

                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar( 1 );
    }
}
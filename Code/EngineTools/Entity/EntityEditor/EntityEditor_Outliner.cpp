#include "EntityEditor_Outliner.h"
#include "EntityEditor_Context.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/Entity.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathRandom.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityEditorOutliner::EntityEditorOutliner( EntityEditorContext& ctx )
        : m_context( ctx )
    {}

    void EntityEditorOutliner::Draw( UpdateContext const& context )
    {
        EntityMap const* pMap = m_context.GetMap();
        if ( pMap == nullptr )
        {
            return;
        }

        // Create New Entity
        //-------------------------------------------------------------------------

        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
            if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "CREATE NEW ENTITY", ImVec2( -1, 0 ) ) )
            {
                Entity* pEntity = m_context.CreateEntity();
                m_context.SelectEntity( pEntity );
            }
        }

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        if ( ImGui::BeginChild( "EntityList", ImVec2( -1, 0 ) ) )
        {
            int32_t const numEntities = (int32_t) pMap->GetEntities().size();
            for ( int32_t i = 0; i < numEntities; i++ )
            {
                Entity* pEntity = pMap->GetEntities()[i];

                bool const isSelected = m_context.IsSelected( pEntity );
                if ( isSelected )
                {
                    ImGuiX::PushFontAndColor( ImGuiX::Font::SmallBold, ImGuiX::Style::s_colorAccent1 );
                }

                String const buttonLabel( String::CtorSprintf(), "%s##%u", pEntity->GetName().c_str(), pEntity->GetID().ToUint64() );
                ImGui::Selectable( buttonLabel.c_str(), isSelected, 0 );

                if ( isSelected )
                {
                    ImGui::PopFont();
                    ImGui::PopStyleColor();
                }

                if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
                {
                    if ( pEntity->IsSpatialEntity() )
                    {
                        auto drawingCtx = m_context.GetDrawingContext();
                        drawingCtx.DrawWireBox( pEntity->GetCombinedWorldBounds(), (ImVec4) ImGuiX::Style::s_colorAccent1, 2.0f );
                    }

                    // Left click follows regular selection logic
                    if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
                    {
                        if ( ImGui::GetIO().KeyShift )
                        {
                            // TODO
                            // Find selection bounds and bulk select everything between them
                        }
                        else if ( ImGui::GetIO().KeyCtrl )
                        {
                            if ( isSelected )
                            {
                                m_context.RemoveFromSelectedEntities( pEntity );
                            }
                            else
                            {
                                m_context.AddToSelectedEntities( pEntity );
                            }
                        }
                        else
                        {
                            m_context.SelectEntity( pEntity );
                        }
                    }
                    // Right click never deselects! Nor does it support multi-selection
                    else if ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
                    {
                        if ( !isSelected )
                        {
                            m_context.SelectEntity( pEntity );
                        }
                    }
                    else if ( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
                    {
                        m_context.SelectEntity( pEntity );
                    }
                }

                // Handle context menu
                if ( ImGui::BeginPopupContextItem( buttonLabel.c_str() ) )
                {
                    if ( ImGui::MenuItem( "Delete" ) )
                    {
                        m_context.DestroySelectedEntities();
                    }

                    ImGui::EndPopup();
                }
            }
        }
        ImGui::EndChild();
    }
}
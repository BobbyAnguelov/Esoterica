#include "EntityWorldOutliner.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/Entity.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathRandom.h"
#include "System/Imgui/ImguiStyle.h"

#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityWorldOutlinerTreeItem final : public TreeListViewItem
    {
    public:

        constexpr static char const* const s_dragAndDropID = "EntityItem";

    public:

        EntityWorldOutlinerTreeItem( Entity* pEntity )
            : m_pEntity( pEntity )
        {
            EE_ASSERT( m_pEntity != nullptr );
        }

        virtual uint64_t GetUniqueID() const override
        {
            return m_pEntity->GetID().m_ID;
        }

        virtual StringID GetNameID() const override
        {
            return m_pEntity->GetName();
        }

        virtual bool IsDragAndDropTarget() { return m_pEntity->IsSpatialEntity(); }

        virtual bool IsDragAndDropSource() { return m_pEntity->IsSpatialEntity(); }

        virtual void SetDragAndDropPayloadData() const override
        {
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    private:

        Entity* m_pEntity = nullptr;
    };

    //-------------------------------------------------------------------------

    EntityWorldOutliner::EntityWorldOutliner( ToolsContext const* pToolsContext, EntityEditorContext& ctx )
        : m_context( ctx )
    {
        m_multiSelectionAllowed = true;
    }

    void EntityWorldOutliner::Initialize( UpdateContext const& context, uint32_t widgetUniqueID )
    {
        m_windowName.sprintf( "Outliner##%u", widgetUniqueID );
    }

    void EntityWorldOutliner::Shutdown( UpdateContext const& context )
    {
        m_pWorld = nullptr;
        m_pMap = nullptr;
    }

    void EntityWorldOutliner::SetWorldToOutline( EntityWorld* pWorld, EntityMapID const& mapID )
    {
        m_pWorld = pWorld;

        if ( m_pWorld != nullptr )
        {
            if ( mapID.IsValid() )
            {
                m_pMap = m_pWorld->GetMap( mapID );
            }
            else
            {
                m_pWorld->GetFirstNonPersistentMap();
            }
        }

        //-------------------------------------------------------------------------

        RebuildTree( false );
    }

    //-------------------------------------------------------------------------

    void EntityWorldOutliner::UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_windowName.c_str() ) )
        {
            EntityMap const* pMap = m_context.GetMap();
            if ( m_pWorld == nullptr )
            {
                ImGui::Text( "No World To Outline." );
            }
            else if ( m_pMap == nullptr )
            {
                ImGui::Text( "No Map To Outline." );
            }
            else // Valid world and map
            {
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
                    if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "CREATE NEW ENTITY", ImVec2( -1, 0 ) ) )
                    {
                        Entity* pEntity = m_context.CreateEntity();
                        m_context.SelectEntity( pEntity );
                    }
                }

                TreeListView::Draw();
            }

            //EntityMap const* pMap = m_context.GetMap();
            //if ( pMap == nullptr )
            //{
            //    ImGui::Text( "Nothing to show." );
            //}
            //else
            //{
            //    // Create New Entity
            //    //-------------------------------------------------------------------------

            //    {
            //        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
            //        if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "CREATE NEW ENTITY", ImVec2( -1, 0 ) ) )
            //        {
            //            Entity* pEntity = m_context.CreateEntity();
            //            m_context.SelectEntity( pEntity );
            //        }
            //    }

            //    //-------------------------------------------------------------------------

            //    ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

            //    if ( ImGui::BeginChild( "EntityList", ImVec2( -1, 0 ) ) )
            //    {
            //        int32_t const numEntities = (int32_t) pMap->GetEntities().size();
            //        for ( int32_t i = 0; i < numEntities; i++ )
            //        {
            //            Entity* pEntity = pMap->GetEntities()[i];

            //            bool const isSelected = m_context.IsSelected( pEntity );
            //            if ( isSelected )
            //            {
            //                ImGuiX::PushFontAndColor( ImGuiX::Font::SmallBold, ImGuiX::Style::s_colorAccent1 );
            //            }

            //            String const buttonLabel( String::CtorSprintf(), "%s##%u", pEntity->GetName().c_str(), pEntity->GetID().ToUint64() );
            //            ImGui::Selectable( buttonLabel.c_str(), isSelected, 0 );

            //            if ( isSelected )
            //            {
            //                ImGui::PopFont();
            //                ImGui::PopStyleColor();
            //            }

            //            if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
            //            {
            //                if ( pEntity->IsSpatialEntity() )
            //                {
            //                    auto drawingCtx = m_context.GetDrawingContext();
            //                    drawingCtx.DrawWireBox( pEntity->GetCombinedWorldBounds(), (ImVec4) ImGuiX::Style::s_colorAccent1, 2.0f );
            //                }

            //                // Left click follows regular selection logic
            //                if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            //                {
            //                    if ( ImGui::GetIO().KeyShift )
            //                    {
            //                        // TODO
            //                        // Find selection bounds and bulk select everything between them
            //                    }
            //                    else if ( ImGui::GetIO().KeyCtrl )
            //                    {
            //                        if ( isSelected )
            //                        {
            //                            m_context.RemoveFromSelectedEntities( pEntity );
            //                        }
            //                        else
            //                        {
            //                            m_context.AddToSelectedEntities( pEntity );
            //                        }
            //                    }
            //                    else
            //                    {
            //                        m_context.SelectEntity( pEntity );
            //                    }
            //                }
            //                // Right click never deselects! Nor does it support multi-selection
            //                else if ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            //                {
            //                    if ( !isSelected )
            //                    {
            //                        m_context.SelectEntity( pEntity );
            //                    }
            //                }
            //                else if ( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
            //                {
            //                    m_context.SelectEntity( pEntity );
            //                }
            //            }

            //            // Handle context menu
            //            if ( ImGui::BeginPopupContextItem( buttonLabel.c_str() ) )
            //            {
            //                if ( ImGui::MenuItem( "Delete" ) )
            //                {
            //                    m_context.DestroySelectedEntities();
            //                }

            //                ImGui::EndPopup();
            //            }
            //        }
            //    }
            //    ImGui::EndChild();
            //}
        }
        ImGui::End();
    }

    void EntityWorldOutliner::RebuildTreeInternal()
    {
        if ( m_pMap == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pEntity : m_pMap->GetEntities() )
        {
            m_rootItem.CreateChild<EntityWorldOutlinerTreeItem>( pEntity );
        }
    }

    void EntityWorldOutliner::HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem )
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( EntityWorldOutlinerTreeItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                auto pRawData = (uintptr_t*) payload->Data;
                auto pDraggedSourceItem = (EntityWorldOutlinerTreeItem*) *pRawData;
                // TODO: support parenting entities
            }
        }
    }
}
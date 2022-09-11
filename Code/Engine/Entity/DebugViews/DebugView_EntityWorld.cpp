#include "DebugView_EntityWorld.h"
#include "System/Imgui/ImguiX.h"
#include "Engine/Camera/Components/Component_Camera.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    EntityDebugView::EntityDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Engine/World", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void EntityDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( m_pWorld != nullptr );

        if ( m_isWorldBrowserOpen )
        {
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            DrawWorldBrowser( context );
        }

        if ( m_isMapLoaderOpen )
        {
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            DrawMapLoader( context );
        }
    }

    void EntityDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Show World Browser" ) )
        {
            m_isWorldBrowserOpen = true;
        }

        if ( ImGui::MenuItem( "Show Map Loader" ) )
        {
            m_isMapLoaderOpen = true;
        }
    }

    //-------------------------------------------------------------------------
    // Map Loader
    //-------------------------------------------------------------------------

    void EntityDebugView::DrawMapLoader( EntityWorldUpdateContext const& context )
    {
        ImGui::SetNextWindowBgAlpha( 0.75f );
        if ( ImGui::Begin( "Map Loader", &m_isMapLoaderOpen ) )
        {
            TInlineString<100> tempStr;

            TPair<String, ResourceID> maps[] =
            {
                TPair<String, ResourceID>( "BR Minimal", ResourceID( "data://maps/BR_Minimal.map" ) ),
                TPair<String, ResourceID>( "BR Full", ResourceID( "data://maps/BR_Full.map" ) ),
                TPair<String, ResourceID>( "Anim Minimal", ResourceID( "data://maps/AnimTest_Minimal.map" ) ),
                TPair<String, ResourceID>( "Anim Full", ResourceID( "data://maps/AnimTest_Full.map" ) ),
            };

            //-------------------------------------------------------------------------

            for ( auto i = 0; i < 4; i++ )
            {
                if ( m_pWorld->HasMap( maps[i].second ) )
                {
                    tempStr.sprintf( "Unload Map: %s", maps[i].first.c_str() );
                    if ( ImGui::Button( tempStr.c_str() ) )
                    {
                        const_cast<EntityWorld*>( m_pWorld )->UnloadMap( maps[i].second );
                    }
                }
                else
                {
                    tempStr.sprintf( "Load Map: %s", maps[i].first.c_str() );
                    if ( ImGui::Button( tempStr.c_str() ) )
                    {
                        const_cast<EntityWorld*>( m_pWorld )->LoadMap( maps[i].second );
                    }
                }
            }
        }
        ImGui::End();
    }

    //-------------------------------------------------------------------------
    // World Browser
    //-------------------------------------------------------------------------

    void EntityDebugView::DrawComponentEntry( EntityComponent const* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );

        ImGui::Text( pComponent->GetNameID().c_str() );
        ImGui::SameLine();
        ImGui::Text( " - %u - ", pComponent->GetID() );
        ImGui::SameLine();

        EntityComponent::Status const componentStatus = pComponent->GetStatus();
        
        switch ( componentStatus )
        {
            case EntityComponent::Status::LoadingFailed:
            {
                ImGui::PushStyleColor( ImGuiCol_Text, 0xFF0000FF );
                ImGui::Text( "Load Failed" );
                ImGui::PopStyleColor( 1 );
            }
            break;

            case EntityComponent::Status::Unloaded:
            {
                ImGui::PushStyleColor( ImGuiCol_Text, 0xFF666666 );
                ImGui::Text( "Unloaded" );
                ImGui::PopStyleColor( 1 );
            }
            break;

            case EntityComponent::Status::Loading:
            {
                ImGui::PushStyleColor( ImGuiCol_Text, 0xFFAAAAAA );
                ImGui::Text( "Loading" );
                ImGui::PopStyleColor( 1 );
            }
            break;

            case EntityComponent::Status::Loaded:
            {
                ImGui::PushStyleColor( ImGuiCol_Text, 0xFF00FFFF );
                ImGui::Text( "Loaded" );
                ImGui::PopStyleColor( 1 );
            }
            break;

            case EntityComponent::Status::Initialized:
            {
                ImGui::PushStyleColor( ImGuiCol_Text, 0xFF00FFFF );
                ImGui::Text( "Initialized" );
                ImGui::PopStyleColor( 1 );
            }
            break;
        }
    }

    void EntityDebugView::DrawSpatialComponentTree( SpatialEntityComponent const* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );

        bool const IsNodeExpanded = ImGui::TreeNodeEx( pComponent, ImGuiTreeNodeFlags_DefaultOpen, "" );
        ImGui::SameLine();
        DrawComponentEntry( pComponent );

        if( IsNodeExpanded )
        {
            for ( auto pChildComponent : pComponent->m_spatialChildren )
            {
                DrawSpatialComponentTree( pChildComponent );
            }

            ImGui::TreePop();
        }
    }

    void EntityDebugView::DrawWorldBrowser( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_isWorldBrowserOpen );

        auto drawingCtx = context.GetDrawingContext();

        //-------------------------------------------------------------------------

        m_entities.clear();

        int32_t numEntities = 0 ;
        int32_t numSpatialEntities = 0;
        int32_t numComponents = 0;

        bool foundSelectedEntity = false;

        for ( auto& pLoadedMap : m_pWorld->m_maps )
        {
            if ( pLoadedMap->IsLoaded() || pLoadedMap->IsActivated() )
            {
                numEntities += (int32_t) pLoadedMap->GetEntities().size();
                m_entities.insert( m_entities.end(), pLoadedMap->GetEntities().begin(), pLoadedMap->GetEntities().end() );

                for ( Entity* pEntity : pLoadedMap->GetEntities() )
                {
                    if ( pEntity == m_pSelectedEntity )
                    {
                        foundSelectedEntity = true;
                    }

                    numComponents += pEntity->GetNumComponents();

                    if ( pEntity->IsSpatialEntity() )
                    {
                        numSpatialEntities++;
                    }
                }
            }
        }

        if ( !foundSelectedEntity )
        {
            m_pSelectedEntity = nullptr;
        }

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowBgAlpha( 0.5f );
        if ( ImGui::Begin( "World", &m_isWorldBrowserOpen ) )
        {
            // Draw World Info
            //-------------------------------------------------------------------------

            ImGui::Text( "Total number of entities: %d", numEntities );
            ImGui::Text( "Total number of components: %d", numComponents );
            ImGui::Text( "Number of spatial entities: %d", numSpatialEntities );

            ImGui::Separator();

            // Draw Entity List
            //-------------------------------------------------------------------------

            ImGui::BeginGroup();

            static ImGuiTextFilter filter;
            filter.Draw( "##EntityFilter", 300 );

            ImGui::SetNextWindowBgAlpha( 0.5f );
            if ( ImGui::BeginChild( ImGui::GetID( (void*) (intptr_t) 0 ), ImVec2( 300, -1 ), true, 0 ) )
            {
                for ( auto i = 0u; i < m_entities.size(); i++ )
                {
                    if ( !filter.PassFilter( m_entities[i]->GetNameID().c_str() ) )
                    {
                        continue;
                    }

                    if ( m_pSelectedEntity == m_entities[i] )
                    {
                        ImGui::PushStyleColor( ImGuiCol_Text, 0xFF00FFFF );
                        ImGui::Button( m_entities[i]->GetNameID().c_str() );
                        ImGui::PopStyleColor( 1 );
                    }
                    else
                    {
                        String const buttonLabel = String( String::CtorSprintf(), "%s##%d", m_entities[i]->GetNameID().c_str(), i );
                        if ( ImGui::Button( buttonLabel.c_str() ) )
                        {
                            m_pSelectedEntity = m_entities[i];
                        }
                    }

                    //-------------------------------------------------------------------------

                    if ( ImGui::IsItemHovered() )
                    {
                        if ( m_entities[i]->IsSpatialEntity() )
                        {
                            drawingCtx.DrawPoint( m_entities[i]->GetWorldTransform().GetTranslation(), Colors::Yellow, 10.0f );
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::EndGroup();

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            // Draw Entity Details
            //-------------------------------------------------------------------------

            ImGui::SetNextWindowBgAlpha( 0.5f );
            if ( ImGui::BeginChild( ImGui::GetID( (void*) (intptr_t) 1 ), ImVec2( -1, -1 ), true, 0 ) )
            {
                if ( m_pSelectedEntity != nullptr )
                {
                    ImGui::Text( "Entity Name: %s", m_pSelectedEntity->GetNameID().c_str() );
                    ImGui::Text( "Entity ID: %u", m_pSelectedEntity->GetID() );

                    ImGui::Separator();

                    //-------------------------------------------------------------------------

                    if ( m_pSelectedEntity->IsSpatialEntity() )
                    {
                        if ( ImGui::CollapsingHeader( "Spatial Info", ImGuiTreeNodeFlags_DefaultOpen ) )
                        {
                            auto const& transform = m_pSelectedEntity->GetWorldTransform();
                            auto const eulerAngles = transform.GetRotation().ToEulerAngles();
                            ImGui::Text( "Rotation: %.2f %.2f %.2f", eulerAngles.m_x, eulerAngles.m_y, eulerAngles.m_z );
                            ImGui::Text( "Translation: %.2f %.2f %.2f", transform.GetTranslation().m_x, transform.GetTranslation().m_y, transform.GetTranslation().m_z );
                            ImGui::Text( "Scale: %.2f %.2f %.2f", transform.GetScale().m_x, transform.GetScale().m_y, transform.GetScale().m_z );
                        }
                    }

                    //-------------------------------------------------------------------------

                    if ( !m_pSelectedEntity->GetSystems().empty() )
                    {
                        if ( ImGui::CollapsingHeader( "Systems", ImGuiTreeNodeFlags_DefaultOpen ) )
                        {
                            for ( auto pSystem : m_pSelectedEntity->GetSystems() )
                            {
                                ImGui::Text( pSystem->GetName() );
                            }
                        }
                    }

                    //-------------------------------------------------------------------------

                    TInlineVector<EntityComponent*, 10> components;
                    for ( auto pComponent : m_pSelectedEntity->GetComponents() )
                    {
                        if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
                        {
                            continue;
                        }

                        components.emplace_back( pComponent );
                    }

                    if ( !components.empty() )
                    {
                        if ( ImGui::CollapsingHeader( "Components", ImGuiTreeNodeFlags_DefaultOpen ) )
                        {
                            for ( auto pComponent : components )
                            {
                                DrawComponentEntry( pComponent );
                            }
                        }
                    }

                    //-------------------------------------------------------------------------

                    auto pRootComponent = m_pSelectedEntity->GetRootSpatialComponent();
                    if ( pRootComponent != nullptr )
                    {
                        if ( ImGui::CollapsingHeader( "Spatial Components", ImGuiTreeNodeFlags_DefaultOpen ) )
                        {
                            DrawSpatialComponentTree( pRootComponent );
                        }
                    }
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
}
#endif
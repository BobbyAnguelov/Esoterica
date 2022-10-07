#include "Workspace_StaticMesh.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Render/Components/Component_StaticMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_WORKSPACE_FACTORY( StaticMeshWorkspaceFactory, StaticMesh, StaticMeshWorkspace );

    //-------------------------------------------------------------------------

    void StaticMeshWorkspace::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TWorkspace<StaticMesh>::Initialize( context );

        m_showDescriptorEditor = true;

        //-------------------------------------------------------------------------

        m_infoWindowName.sprintf( "Info##%u", GetID() );

        //-------------------------------------------------------------------------

        // We dont own the entity as soon as we add it to the map
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );

        auto pStaticMeshComponent = EE::New<StaticMeshComponent>( StringID( "Static Mesh Component" ) );
        pStaticMeshComponent->SetMesh( m_workspaceResource.GetResourceID() );
        m_pPreviewEntity->AddComponent( pStaticMeshComponent );

        AddEntityToWorld( m_pPreviewEntity );
    }

    void StaticMeshWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), topDockID );
        ImGui::DockBuilderDockWindow( m_infoWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomDockID );
    }

    void StaticMeshWorkspace::DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        ImGui::SetNextItemWidth( 46 );
        if ( ImGui::BeginCombo( "##AnimOptions", EE_ICON_COG, ImGuiComboFlags_HeightLarge ) )
        {
            ImGui::MenuItem( "Show Origin", nullptr, &m_showOrigin );
            ImGui::MenuItem( "Show Bounds", nullptr, &m_showBounds );
            ImGui::MenuItem( "Show Normals", nullptr, &m_showNormals );
            ImGui::MenuItem( "Show Vertices", nullptr, &m_showVertices );

            ImGui::EndCombo();
        }
    }

    void StaticMeshWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        TWorkspace::Update( context, pWindowClass, isFocused );

        //-------------------------------------------------------------------------

        auto DrawWindowContents = [this] ()
        {
            if ( IsWaitingForResource() )
            {
                ImGui::Text( "Loading:" );
                ImGui::SameLine();
                ImGuiX::DrawSpinner( "Loading" );
                return;
            }

            if ( HasLoadingFailed() )
            {
                ImGui::Text( "Loading Failed: %s", m_workspaceResource.GetResourceID().c_str() );
                return;
            }

            //-------------------------------------------------------------------------

            auto pMesh = m_workspaceResource.GetPtr();
            EE_ASSERT( m_workspaceResource.IsLoaded() );
            EE_ASSERT( pMesh != nullptr );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 2 ) );
            if ( ImGui::BeginTable( "MeshInfoTable", 2, ImGuiTableFlags_Borders ) )
            {
                ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 100 );
                ImGui::TableSetupColumn( "Data", ImGuiTableColumnFlags_NoHide );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Data Path" );

                ImGui::TableNextColumn();
                ImGui::Text( pMesh->GetResourceID().c_str() );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Num Vertices" );

                ImGui::TableNextColumn();
                ImGui::Text( "%d", pMesh->GetNumVertices() );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Num Indices" );

                ImGui::TableNextColumn();
                ImGui::Text( "%d", pMesh->GetNumIndices() );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Mesh Sections" );

                ImGui::TableNextColumn();
                for ( auto const& section : pMesh->GetSections() )
                {
                    ImGui::Text( section.m_ID.c_str() );
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        };

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_infoWindowName.c_str() ) )
        {
            DrawWindowContents();
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        if ( m_pPreviewEntity != nullptr && ImGui::IsKeyPressed( ImGuiKey_Backspace ) )
        {
            FocusCameraView( m_pPreviewEntity );
        }

        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            auto drawingContext = GetDrawingContext();

            if ( m_showOrigin )
            {
                drawingContext.DrawAxis( Transform::Identity, 0.25f, 3.0f );
            }

            if ( m_showBounds )
            {
                drawingContext.DrawWireBox( m_workspaceResource->GetBounds(), Colors::Cyan );
            }

            if ( ( m_showVertices || m_showNormals ) )
            {
                auto pVertex = reinterpret_cast<StaticMeshVertex const*>( m_workspaceResource->GetVertexData().data() );
                for ( auto i = 0; i < m_workspaceResource->GetNumVertices(); i++ )
                {
                    if ( m_showVertices )
                    {
                        drawingContext.DrawPoint( pVertex->m_position, Colors::Cyan );
                    }

                    if ( m_showNormals )
                    {
                        drawingContext.DrawLine( pVertex->m_position, pVertex->m_position + ( pVertex->m_normal * 0.15f ), Colors::Yellow );
                    }

                    pVertex++;
                }
            }
        }
    }
}
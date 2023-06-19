#include "Workspace_Material.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_WORKSPACE_FACTORY( MaterialWorkspaceFactory, Material, MaterialWorkspace );

    //-------------------------------------------------------------------------

    void MaterialWorkspace::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewComponent == nullptr );

        TWorkspace<Material>::Initialize( context );

        //-------------------------------------------------------------------------

        m_pPreviewComponent = EE::New<StaticMeshComponent>( StringID( "Static Mesh Component" ) );
        m_pPreviewComponent->SetMesh( "data://Editor/MaterialBall/MaterialBall.msh" );
        m_pPreviewComponent->SetWorldTransform( Transform( Quaternion::Identity, Vector( 0, 0, 1 ) ) );
        m_pPreviewComponent->SetMaterialOverride( 0, m_workspaceResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        auto pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        pPreviewEntity->AddComponent( m_pPreviewComponent );
        AddEntityToWorld( pPreviewEntity );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
    }

    void MaterialWorkspace::Shutdown( UpdateContext const& context )
    {
        m_pPreviewComponent = nullptr;
        TWorkspace<Material>::Shutdown( context );
    }

    void MaterialWorkspace::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomDockID );
    }

    void MaterialWorkspace::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
        }
        else if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_workspaceResource.GetResourceID().c_str() );
            return;
        }
        else
        {
            if ( m_pDescriptor != nullptr )
            {
                m_pDescriptorPropertyGrid->DrawGrid();
            }
        }
    }
}
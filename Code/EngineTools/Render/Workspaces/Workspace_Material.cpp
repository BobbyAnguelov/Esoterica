#include "Workspace_Material.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
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

        m_materialDetailsWindowName.sprintf( "Material Properties##%u", GetID() );

        //-------------------------------------------------------------------------

        m_pPreviewComponent = EE::New<StaticMeshComponent>( StringID( "Static Mesh Component" ) );
        m_pPreviewComponent->SetMesh( "data://Editor/MaterialBall/MaterialBall.msh" );
        m_pPreviewComponent->SetWorldTransform( Transform( Quaternion::Identity, Vector( 0, 0, 1 ) ) );
        m_pPreviewComponent->SetMaterialOverride( 0, m_pResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        auto pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        pPreviewEntity->AddComponent( m_pPreviewComponent );
        AddEntityToWorld( pPreviewEntity );
    }

    void MaterialWorkspace::Shutdown( UpdateContext const& context )
    {
        m_pPreviewComponent = nullptr;
        TWorkspace<Material>::Shutdown( context );
    }

    void MaterialWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), topDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_materialDetailsWindowName.c_str(), bottomDockID );
    }

    void MaterialWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_materialDetailsWindowName.c_str() ) )
        {
            if ( IsWaitingForResource() )
            {
                ImGui::Text( "Loading:" );
                ImGui::SameLine();
                ImGuiX::DrawSpinner( "Loading" );
            }
            else if ( HasLoadingFailed() )
            {
                ImGui::Text( "Loading Failed: %s", m_pResource.GetResourceID().c_str() );
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
        ImGui::End();
    }
}
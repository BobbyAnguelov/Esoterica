#include "ResourceEditor_Material.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY( MaterialEditorFactory, Material, MaterialEditor );

    //-------------------------------------------------------------------------

    void MaterialEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewComponent == nullptr );

        TResourceEditor<Material>::Initialize( context );

        //-------------------------------------------------------------------------

        m_pPreviewComponent = EE::New<StaticMeshComponent>( StringID( "Static Mesh Component" ) );
        m_pPreviewComponent->SetMesh( "data://Editor/MaterialBall/MaterialBall.msh" );
        m_pPreviewComponent->SetWorldTransform( Transform( Quaternion::Identity, Vector( 0, 0, 1 ) ) );
        m_pPreviewComponent->SetMaterialOverride( 0, m_editedResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        auto pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        pPreviewEntity->AddComponent( m_pPreviewComponent );
        AddEntityToWorld( pPreviewEntity );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
    }

    void MaterialEditor::Shutdown( UpdateContext const& context )
    {
        m_pPreviewComponent = nullptr;
        TResourceEditor<Material>::Shutdown( context );
    }

    void MaterialEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomDockID );
    }

    void MaterialEditor::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
        }
        else if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_editedResource.GetResourceID().c_str() );
            return;
        }
        else
        {
            if ( IsDescriptorLoaded() )
            {
                m_pDescriptorPropertyGrid->DrawGrid();
            }
        }
    }
}
#include "ResourceEditor_Material.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY( MaterialEditorFactory, Material, MaterialEditor );

    //-------------------------------------------------------------------------

    void MaterialEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0;
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Right, 0.35f, nullptr, &leftDockID );

        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), rightDockID );
    }

    void MaterialEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        TResourceEditor<Material>::OnResourceLoadCompleted( pResourcePtr );

        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            CreatePreviewEntity();
        }
    }

    void MaterialEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            DestroyPreviewEntity();
        }

        TResourceEditor<Material>::OnResourceUnload( pResourcePtr );
    }

    void MaterialEditor::CreatePreviewEntity()
    {
        EE_ASSERT( IsDataFileLoaded() );

        m_pPreviewComponent = EE::New<StaticMeshComponent>( StringID( "Static Mesh Component" ) );
        m_pPreviewComponent->SetMesh( "data://Editor/MaterialBall/MaterialBall.mesh" );
        m_pPreviewComponent->SetWorldTransform( Transform( Quaternion::Identity, Vector( 0, 0, 1 ) ) );
        m_pPreviewComponent->SetMaterialOverride( 0, m_editedResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        auto pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        pPreviewEntity->AddComponent( m_pPreviewComponent );
        AddEntityToWorld( pPreviewEntity );
    }

    void MaterialEditor::DestroyPreviewEntity()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pPreviewComponent = nullptr;
        }
    }
}
#include "ResourceEditor_CollisionMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    EE_RESOURCE_EDITOR_FACTORY( CollisionMeshEditorFactory, CollisionMesh, CollisionMeshEditor );

    //-------------------------------------------------------------------------

    void CollisionMeshEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor<CollisionMesh>::Initialize( context );
    }

    void CollisionMeshEditor::Shutdown( UpdateContext const& context )
    {
        TResourceEditor<CollisionMesh>::Shutdown( context );
    }

    void CollisionMeshEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_viewportWindowName ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), bottomDockID );
    }
}
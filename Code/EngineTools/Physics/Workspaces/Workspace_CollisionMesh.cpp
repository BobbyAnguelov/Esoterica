#include "Workspace_CollisionMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    EE_RESOURCE_WORKSPACE_FACTORY( CollisionMeshWorkspaceFactory, CollisionMesh, CollisionMeshWorkspace );

    //-------------------------------------------------------------------------

    void CollisionMeshWorkspace::Initialize( UpdateContext const& context )
    {
        TWorkspace<CollisionMesh>::Initialize( context );
    }

    void CollisionMeshWorkspace::Shutdown( UpdateContext const& context )
    {
        TWorkspace<CollisionMesh>::Shutdown( context );
    }

    void CollisionMeshWorkspace::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomDockID );
    }
}
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

    void CollisionMeshWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), topDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomDockID );
    }

    void CollisionMeshWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        TWorkspace<CollisionMesh>::Update( context, pWindowClass, isFocused );
    }
}
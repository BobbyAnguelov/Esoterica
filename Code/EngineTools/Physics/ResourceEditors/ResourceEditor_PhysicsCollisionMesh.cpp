#include "ResourceEditor_PhysicsCollisionMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Engine/UpdateContext.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    EE_RESOURCE_EDITOR_FACTORY( CollisionMeshEditorFactory, CollisionMesh, CollisionMeshEditor );

    //-------------------------------------------------------------------------

    void CollisionMeshEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor<CollisionMesh>::Initialize( context );
        m_pDebugMeshRegistry = context.GetSystem<Render::DebugMeshRegistry>();
    }

    void CollisionMeshEditor::Shutdown( UpdateContext const& context )
    {
        if ( m_debugMeshID != 0 )
        {
            m_pDebugMeshRegistry->UnregisterMesh( m_debugMeshID );
            m_debugMeshID = 0;
        }

        m_pDebugMeshRegistry = nullptr;
        TResourceEditor<CollisionMesh>::Shutdown( context );
    }

    void CollisionMeshEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Right, 0.33f, &rightDockID, nullptr  );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), rightDockID );
    }

    void CollisionMeshEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pDebugMeshRegistry != nullptr );

        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            // Unregister previous mesh
            if ( m_debugMeshID != 0 )
            {
                m_pDebugMeshRegistry->UnregisterMesh( m_debugMeshID );
            }

            // Create new mesh
            m_debugMesh.Clear();
            m_debugMeshID = 0;

            CollisionMesh const* pCollisionMesh = m_editedResource.GetPtr();
            EE_ASSERT( pCollisionMesh != nullptr );
            if ( pCollisionMesh->IsConvexHull() )
            {
                CreateDebugMeshFromConvexHull( pCollisionMesh->GetHullData(), m_debugMesh );
            }
            else
            {
                CreateDebugMeshFromTriangleMesh( pCollisionMesh->GetMeshData(), m_debugMesh );
            }

            // Register Mesh
            m_debugMeshID = m_pDebugMeshRegistry->RegisterMesh( "P", m_debugMesh );
        }
    }

    void CollisionMeshEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        EE_ASSERT( m_pDebugMeshRegistry != nullptr );

        if ( IsWaitingForResource() || HasLoadingFailed() || m_debugMeshID == 0 )
        {
            return;
        }

        CollisionMesh const* pCollisionMesh = m_editedResource.GetPtr();
        EE_ASSERT( pCollisionMesh != nullptr );

        auto drawingContext = GetDebugDrawContext();
        drawingContext.DrawMesh( m_debugMeshID, Transform::Identity, Colors::Yellow.GetAlphaVersion( 0.75f ) );
        drawingContext.DrawWireMesh( m_debugMeshID, Transform::Identity, Colors::Black );
    }

    bool CollisionMeshEditor::ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        ImGui::Checkbox( "Draw Normals", &m_drawNormals );

        return true;
    }
}
#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Base/Imgui/ImguiX.h"
#include "Engine/Render/DebugMesh/DebugMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render { class DebugMeshRegistry; }

namespace EE::Physics
{
    class CollisionMeshEditor final : public TResourceEditor<CollisionMesh>
    {
        EE_EDITOR_TOOL( CollisionMeshEditor );

    public:

        using TResourceEditor::TResourceEditor;

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PALETTE_SWATCH; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool HasViewportToolbar() const { return true; }
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;
        virtual bool ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport ) override;

    private:

        Render::DebugMeshRegistry*  m_pDebugMeshRegistry = nullptr;
        Render::DebugMesh           m_debugMesh;
        uint64_t                    m_debugMeshID = 0;
        bool                        m_drawNormals = false;
    };
}
#pragma once

#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshWorkspace final : public TResourceWorkspace<StaticMesh>
    {

    public:

        using TResourceWorkspace::TResourceWorkspace;

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) override;

    private:

        Entity*         m_pPreviewEntity = nullptr;
        String          m_infoWindowName;
        bool            m_showVertices = false;
        bool            m_showNormals = false;
    };
}
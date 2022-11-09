#pragma once

#include "EngineTools/Core/Workspace.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshWorkspace final : public TWorkspace<StaticMesh>
    {

    public:

        using TWorkspace::TWorkspace;

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PINE_TREE; }

    private:

        Entity*         m_pPreviewEntity = nullptr;
        String          m_infoWindowName;
        bool            m_showOrigin = true;
        bool            m_showBounds = true;
        bool            m_showVertices = false;
        bool            m_showNormals = false;
    };
}
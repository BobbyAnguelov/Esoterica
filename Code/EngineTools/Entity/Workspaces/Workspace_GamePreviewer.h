#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/Workspace.h"
#include "Engine/ToolsUI/EngineToolsUI.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API GamePreviewer final : public Workspace
    {
    public:

        GamePreviewer( ToolsContext const* pToolsContext, EntityWorld* pWorld );

        void LoadMapToPreview( ResourceID mapResourceID );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    private:

        virtual bool HasViewportWindow() const override { return true; }
        virtual bool HasWorkspaceToolbar() const override;
        virtual bool HasViewportToolbar() const override { return false; }
        virtual bool HasViewportOrientationGuide() const override { return false; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;

        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual bool HasWorkspaceToolbarDefaultItems() const override { return false; }

    private:

        ResourceID                      m_loadedMap;
        EngineToolsUI                   m_engineToolsUI;
    };
}
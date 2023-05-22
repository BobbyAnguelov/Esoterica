#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/Workspace.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EngineToolsUI;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GamePreviewer final : public Workspace
    {
    public:

        GamePreviewer( ToolsContext const* pToolsContext, EntityWorld* pWorld );

        void LoadMapToPreview( ResourceID mapResourceID );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    private:

        virtual bool HasViewportWindow() const override { return true; }
        virtual bool HasViewportOrientationGuide() const override { return false; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;

        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual void DrawWorkspaceToolbar( UpdateContext const& context ) override;
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override {}
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;

        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;

    private:

        ResourceID                      m_loadedMap;
        EngineToolsUI*                  m_pEngineToolsUI = nullptr;
    };
}
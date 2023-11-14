#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/EditorTool.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EngineDebugUI;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GamePreviewer final : public EditorTool
    {
        friend class EditorUI;
        EE_EDITOR_TOOL( GamePreviewer );

    public:

        GamePreviewer( ToolsContext const* pToolsContext, EntityWorld* pWorld );

        void LoadMapToPreview( ResourceID mapResourceID );
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        void DrawEngineDebugUI( UpdateContext const& context );

    private:

        
        virtual bool SupportsViewport() const override { return true; }
        virtual bool HasViewportOrientationGuide() const override { return false; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override {}
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;

    private:

        ResourceID                      m_loadedMap;
        EngineDebugUI*                  m_pDebugUI = nullptr;
    };
}
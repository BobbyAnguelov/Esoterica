#pragma once

#include "EngineTools/Core/EditorTool.h"

//-------------------------------------------------------------------------

namespace EE
{
    class GameDebugUI;

    //-------------------------------------------------------------------------

    class GamePreviewer final : public EditorTool
    {
        friend class EditorUI;
        EE_EDITOR_TOOL( GamePreviewer );

    public:

        GamePreviewer( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld );
        GamePreviewer( ToolsContext const* pToolsContext, EntityWorld* pWorld ) : GamePreviewer( pToolsContext, String( "Game Preview" ), pWorld ) {}

        inline Int2 GetDesiredViewportSize() const { return m_desiredViewportSize; }
        inline Int2 GetRequiredWindowSize() const { return m_requiredWindowSize; }
        inline void SetRequiredWindowSize( Int2 windowSize ) { m_requiredWindowSize = windowSize; }

        void LoadMapToPreview( ResourceID mapResourceID );
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        void DrawEngineDebugUI( UpdateContext const& context );

    private:

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool SupportsViewport() const override { return true; }
        virtual bool SupportsUndoRedo() const override { return false; }
        virtual bool HasViewportOrientationGuide() const override { return false; }
        virtual bool HasViewportToolbarTimeControls() const { return true; }
        virtual void DrawMenu( UpdateContext const& context ) override;

        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override;

    private:

        ResourceID                      m_loadedMap;
        GameDebugUI*                    m_pDebugUI = nullptr;
        Int2                            m_desiredViewportSize = Int2( 1920, 1080 );
        Int2                            m_requiredWindowSize = m_desiredViewportSize;
    };
}
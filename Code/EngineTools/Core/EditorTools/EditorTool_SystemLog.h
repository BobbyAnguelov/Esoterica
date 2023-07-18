#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "Engine/DebugViews/DebugView_System.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API SystemLogEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( SystemLogEditorTool );

    public:

        SystemLogEditorTool( ToolsContext const* pToolsContext );

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual char const* GetDockingUniqueTypeName() const override { return "System Log"; }
        virtual void Initialize( UpdateContext const& context ) override;

    private:

        void DrawLogWindow( UpdateContext const& context, bool isFocused );

    private:

        SystemLogView m_logViewWidget;
    };
}
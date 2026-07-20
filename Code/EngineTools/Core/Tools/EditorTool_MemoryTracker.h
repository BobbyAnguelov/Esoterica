#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "Engine/Debug/Widgets/MemoryTrackerWidget.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API MemoryTrackerEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( MemoryTrackerEditorTool );

    public:

        MemoryTrackerEditorTool( ToolsContext const* pToolsContext );

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual bool SupportsMainMenu() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;

    private:

        void DrawWindow( UpdateContext const& context, bool isFocused );

    private:

        MemoryTrackerWidget m_memoryTracker;
    };
}
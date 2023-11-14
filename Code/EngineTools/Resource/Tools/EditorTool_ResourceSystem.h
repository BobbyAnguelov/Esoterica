#pragma once
#include "EngineTools/Core/EditorTool.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API ResourceSystemEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( ResourceSystemEditorTool );

    public:

        ResourceSystemEditorTool( ToolsContext const* pToolsContext );

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

    private:

        void DrawOverviewWindow( UpdateContext const& context, bool isFocused );
        void DrawRequestHistoryWindow( UpdateContext const& context, bool isFocused );
    };
}
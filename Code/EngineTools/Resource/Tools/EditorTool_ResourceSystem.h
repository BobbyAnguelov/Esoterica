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
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

    private:

        void DrawOverviewWindow( UpdateContext const& context, bool isFocused );
        void DrawDetailsWindow( UpdateContext const& context, bool isFocused );
        void DrawRequestHistoryWindow( UpdateContext const& context, bool isFocused );
        void DrawResourceProviderWindow( UpdateContext const& context, bool isFocused );

        void GenerateSortedAndFilteredRecordList();

    private:

        ImGuiX::FilterWidget                m_filter;
        ResourceID                          m_selectedResourceID;
        TVector<ResourceRecord const*>      m_cachedRecords;
        int32_t                             m_sortedColumnIdx = InvalidIndex;
        bool                                m_sortAscending = false;
    };
}
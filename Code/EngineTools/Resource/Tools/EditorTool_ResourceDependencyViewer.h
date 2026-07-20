#pragma once
#include "EngineTools/Core/EditorTool.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API ResourceDependencyViewerEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( ResourceDependencyViewerEditorTool );

    private:

        struct DependencyTreeNode
        {
            void Clear();
            void AddInstallDependency( ToolsContext const& toolsContext, ResourceID const& installDependencyID );
            void AddCompileDependency( ToolsContext const& toolsContext, Resource::CompileDependency const& compileDependency );

        public:

            DataPath                            m_path;
            bool                                m_isResource = false;
            bool                                m_isMissingOrInvalidFile = false;
            TVector<DependencyTreeNode*>        m_installDependencies;
            TVector<DependencyTreeNode*>        m_compileDependencies;
        };

        struct DependencyView : public DependencyTreeNode
        {
            DependencyView( ResourceID const& ID );

            void ClearDependencyTree() { Clear(); }

            bool operator==( ResourceID const& ID ) const { return m_ID == ID; }
            bool operator!=( ResourceID const& ID ) const { return m_ID != ID; }

        public:

            ResourceID                          m_ID;
            String                              m_tabName;
            FileRegistry::FileInfo const*       m_pFileInfo = nullptr;
            TVector<DataPath>                   m_dependentResources;
        };

    public:

        ResourceDependencyViewerEditorTool( ToolsContext const* pToolsContext );
        ~ResourceDependencyViewerEditorTool();

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual bool SupportsMainMenu() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;

        void ShowDependenciesForResourceID( ResourceID const& resourceID );

    private:

        void OnFileRegistryUpdated();

        inline int32_t FindViewIndex( ResourceID const& ID ) const
        {
           return VectorFindIndex( m_dependencyViews, ID, [] ( DependencyView const& view, ResourceID const& resourceID ) { return view == resourceID; } );
        }

        void DrawWindow( UpdateContext const& context, bool isFocused );

        void RefreshView( DependencyView &view );

        void DrawView( UpdateContext const& context, DependencyView &view );

        void DrawInstallDependencyNode( DependencyTreeNode* pNode );

        void DrawCompileDependencyNode( DependencyTreeNode* pNode );

        void DrawDependentResource( DataPath const& path );

    private:

        TVector<DependencyView>     m_dependencyViews;
        ResourceID                  m_viewFocusRequest;
        TVector<ResourceID>         m_viewCloseRequests;
        ResourcePicker              m_resourcePicker;
        EventBindingID              m_fileRegistryUpdateEventBindingID;
    };
}
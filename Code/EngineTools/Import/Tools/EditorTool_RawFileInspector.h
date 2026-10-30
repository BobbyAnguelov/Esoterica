#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Import/Formats/FormatFile.h"
#include "Engine/Render/Imgui/ImguiImageCache.h"
#include "Base/Imgui/MaterialDesignIcons.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/FileSystem/DataPath.h"
#include "Base/Types/Color.h"
#include "Base/Types/Function.h"
#include "Base/Types/StringID.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    struct EE_ENGINETOOLS_API RawFileInspector
    {
        static bool IsExtensionInspectable( FileSystem::Extension const& ext );
        static bool RequiresWorld( FileSystem::Extension const& ext );
        static EditorTool* CreateInspector( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld );
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API RawImageInspectorEditorTool : public EditorTool
    {
        static const TArray<FileSystem::Extension, 12> supportedExtensions;

    public:

        EE_EDITOR_TOOL( RawImageInspectorEditorTool );

        static bool SupportsExtension( FileSystem::Extension const& ext );

    public:

        RawImageInspectorEditorTool( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld );
        virtual ~RawImageInspectorEditorTool();

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_MAGNIFY; }
        virtual bool SupportsMainMenu() const override { return false; }
        virtual bool IsSingleWindowTool() const override { return true; }
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool IsEditingFile( DataPath const& dataPath ) const override { return m_dataPath == dataPath; }

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        void DrawDetailsWindow( UpdateContext const& context, bool isFocused );

    private:

        DataPath                    m_dataPath;
        FileSystem::Path            m_filePath;
        TUniquePtr<ImageFile>       m_imageFile;
        TUniquePtr<Image>           m_image;
        ImGuiX::ImageInfo           m_previewImage;
        Log                         m_log;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API RawSceneInspectorEditorTool : public EditorTool
    {
        static const TArray<FileSystem::Extension, 12> supportedExtensions;

    public:

        EE_EDITOR_TOOL( RawSceneInspectorEditorTool );

        static bool SupportsExtension( FileSystem::Extension const& ext );

    public:

        RawSceneInspectorEditorTool( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld );
        virtual ~RawSceneInspectorEditorTool();

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_MAGNIFY; }
        virtual bool SupportsMainMenu() const override { return true; }
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool IsEditingFile( DataPath const& dataPath ) const override { return m_dataPath == dataPath; }

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        void DrawSceneFileOutlineWindow( UpdateContext const& context, bool isFocused );
        void DrawSceneFileItemDetailsWindow( UpdateContext const& context, bool isFocused );

    private:

        template<SceneFile::ItemType T>
        void DrawItems( char const* pLabel, TVector<SceneFile::TItem<T>> const& items )
        {
            if ( items.empty() )
            {
                return;
            }

            ImGui::SetNextItemOpen( true, ImGuiCond_Once );
            if ( ImGui::TreeNode( pLabel ) )
            {
                for ( auto const& item : items )
                {
                    EE_ASSERT( item.IsValid() );

                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

                    if ( item == m_selectedItem )
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }

                    bool const isOpen = ImGui::TreeNodeEx( item.m_ID.c_str(), flags );
                    if ( ImGui::IsItemClicked() )
                    {
                        SetSelectedItem( item );
                    }

                    if ( isOpen )
                    {
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }
        }

        void DrawSelectedMaterialDetails();

        void SetSelectedItem( SceneFile::Item const& item );
        inline void ClearSelection() { SetSelectedItem( SceneFile::Item() ); }

    private:

        DataPath                    m_dataPath;
        FileSystem::Path            m_filePath;
        TUniquePtr<SceneFile>       m_sceneFile;
        Log                         m_log;
        SceneFile::Contents         m_fileContents;
        SceneFile::Item             m_selectedItem;

        TUniquePtr<Materials>       m_materials; // Materials are always loaded as they are small and quick
    };
}
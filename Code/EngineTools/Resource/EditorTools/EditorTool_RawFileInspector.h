#pragma once
#include "EngineTools/Core/EditorTool.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceDescriptor;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API RawFileInspectorEditorTool : public EditorTool
    {
    public:

        EE_EDITOR_TOOL( RawFileInspectorEditorTool );

    public:

        RawFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath );

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FILE; }
        virtual char const* GetDockingUniqueTypeName() const override { return "Raw File Inspector"; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool IsSingleWindowTool() const { return true; }

        FileSystem::Path const& GetInspectedPath() const { return m_filePath; }

    protected:

        virtual void UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused ) { EE_ASSERT( IsSingleWindowTool() ); }

        bool TryGetDestinationFilenameForDescriptor( Resource::ResourceDescriptor const* pDescriptor, FileSystem::Path& outPath );
        bool TrySaveDescriptorToDisk( Resource::ResourceDescriptor const* pDescriptor, FileSystem::Path const filePath, bool openAfterSave = true ) const;

    protected:

        FileSystem::Path                    m_filePath;
    };

    //-------------------------------------------------------------------------
    // File Inspector Factory
    //-------------------------------------------------------------------------
    // Used to spawn the appropriate factory

    class EE_ENGINETOOLS_API RawFileInspectorFactory : public TGlobalRegistryBase<RawFileInspectorFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( RawFileInspectorFactory );

    public:

        virtual ~RawFileInspectorFactory() = default;
        static bool CanCreateInspector( FileSystem::Path const& filePath );
        static RawFileInspectorEditorTool* TryCreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath );

    protected:

        // Get the resource type this factory supports
        virtual bool IsSupportedFile( FileSystem::Path const& filePath ) const = 0;

        // Virtual method that will create a workspace if the resource ID matches the appropriate types
        virtual RawFileInspectorEditorTool* CreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath ) const = 0;
    };
}

//-------------------------------------------------------------------------
//  Macro to create a raw file inspector factory
//-------------------------------------------------------------------------
// Use in a CPP to define a factory e.g., EE_RESOURCE_RAW_FILE_INSPECTOR_FACTORY( FBXInspector, fbx, FBXResourceInspector );
// Note: extension needs to be lowercase

#define EE_RAW_FILE_INSPECTOR_FACTORY( FactoryName, RawFileExtension, EditorClass )\
class FactoryName final : public RawFileInspectorFactory\
{\
    virtual bool IsSupportedFile( FileSystem::Path const& filePath) const override { return filePath.GetLowercaseExtensionAsString() == RawFileExtension; }\
    virtual RawFileInspectorEditorTool* CreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath ) const override\
    {\
        EE_ASSERT( IsSupportedFile( filePath ) );\
        return EE::New<EditorClass>( pToolsContext, filePath );\
    }\
};\
static FactoryName g_##FactoryName;

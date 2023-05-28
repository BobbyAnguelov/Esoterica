#pragma once

#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Resource/ResourceTypeID.h"
#include "System/Utils/GlobalRegistryBase.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceDescriptor;
    class ResourceDatabase;

    //-------------------------------------------------------------------------

    class RawFileInspector
    {
    public:

        struct Model
        {
            FileSystem::Path                    m_sourceResourceDirectory;
            FileSystem::Path                    m_compiledResourceDirectory;
            TypeSystem::TypeRegistry const*     m_pTypeRegistry = nullptr;
        };

    public:

        RawFileInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath );
        virtual ~RawFileInspector();

        // Get the actual file-path for the file
        inline FileSystem::Path const& GetFilePath() const { return m_filePath; }

        // Draw the inspector UI
        bool DrawDialog();

    protected:

        bool CreateNewDescriptor( ResourceTypeID resourceTypeID, Resource::ResourceDescriptor const* pDescriptor ) const;

        virtual char const* GetInspectorTitle() const = 0;

        // Override this to draw the contents of the file
        virtual void DrawFileInfo() = 0;

        // Override this to draw the contents of the file
        virtual void DrawFileContents() = 0;

        // Override this to draw the asset creator window, this is where you can create new descriptors
        virtual void DrawResourceDescriptorCreator() = 0;

        RawFileInspector& operator=( RawFileInspector const& ) = delete;
        RawFileInspector( RawFileInspector const& ) = delete;

    protected:

        ToolsContext const*                 m_pToolsContext = nullptr;
        FileSystem::Path                    m_rawResourceDirectory;
        FileSystem::Path                    m_filePath;
        ResourceDescriptor*                 m_pDescriptor = nullptr;
        PropertyGrid                        m_propertyGrid;
    };

    //-------------------------------------------------------------------------
    // Resource Workspace Factory
    //-------------------------------------------------------------------------
    // Used to spawn the appropriate factory

    class RawFileInspectorFactory : public TGlobalRegistryBase<RawFileInspectorFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( RawFileInspectorFactory );

    public:

        virtual ~RawFileInspectorFactory() = default;
        static bool CanCreateInspector( FileSystem::Path const& filePath );
        static RawFileInspector* TryCreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath );

    protected:

        // Get the resource type this factory supports
        virtual bool IsSupportedFile( FileSystem::Path const& filePath ) const = 0;

        // Virtual method that will create a workspace if the resource ID matches the appropriate types
        virtual RawFileInspector* CreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath ) const = 0;
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
    virtual RawFileInspector* CreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath ) const override\
    {\
        EE_ASSERT( IsSupportedFile( filePath ) );\
        return EE::New<EditorClass>( pToolsContext, filePath );\
    }\
};\
static FactoryName g_##FactoryName;

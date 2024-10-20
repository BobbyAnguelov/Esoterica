#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Utils/GlobalRegistryBase.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;

    namespace Resource
    {
        struct ResourceDescriptor;
    }

    //-------------------------------------------------------------------------
    // Data Path Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DataPathPicker final
    {
    public:

        DataPathPicker( FileSystem::Path const& sourceDataDirectoryPath, DataPath const& path = DataPath() )
            : m_sourceDataDirectoryPath( sourceDataDirectoryPath )
            , m_dataPath( path )
        {
            EE_ASSERT( m_sourceDataDirectoryPath.IsValid() && m_sourceDataDirectoryPath.Exists() );
        }

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the path
        void SetPath( DataPath const& path ) { m_dataPath = path; }

        // Get the resource path that was set
        inline DataPath const& GetPath() const { return m_dataPath; }

        // Try to update the path from a drag and drop operation - returns true if the value was updated
        bool TrySetPathFromDragAndDrop();

    private:

        FileSystem::Path const      m_sourceDataDirectoryPath;
        DataPath                    m_dataPath;
    };

    //-------------------------------------------------------------------------
    // Data File Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DataFilePathPicker final
    {
    public:

        DataFilePathPicker( ToolsContext const& toolsContext, TypeSystem::TypeID dataFileTypeID = TypeSystem::TypeID(), DataPath const& datafilePath = DataPath() );

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the type of resource we wish to select
        void SetRequiredDataFileType( TypeSystem::TypeID typeID );

        // Set the path
        void SetPath( DataPath const& path );

        // Get the resource path that was set
        inline DataPath const& GetPath() const { return m_path; }

    private:

        // Generate the set of valid resource options
        void GenerateResourceOptionsList();

        // Generate the set of filtered options
        void GenerateFilteredOptionList();

        // Try to update the resourceID from a paste operation - returns true if the value was updated
        bool TryUpdatePathFromClipboard();

        // Try to update the resourceID from a drag and drop operation - returns true if the value was updated
        bool TryUpdatePathFromDragAndDrop();

        // Validate the supplied extension against what we expect
        inline bool IsValidExtension( FileSystem::Extension const& ext ) const
        {
            return m_requiredExtension.comparei( ext ) == 0;
        }

    private:

        ToolsContext const&                                     m_toolsContext;
        TypeSystem::TypeID                                      m_fileTypeID; // The type of file we should pick from
        FileSystem::Extension                                   m_requiredExtension;
        DataPath                                                m_path;
        ImGuiX::FilterWidget                                    m_filterWidget;
        TVector<DataPath>                                       m_generatedOptions;
        TVector<DataPath>                                       m_filteredOptions;
        bool                                                    m_isComboOpen = false;
    };

    //-------------------------------------------------------------------------
    // Resource ID Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourcePicker final
    {
    public:

        // Implement this if you want to generate a custom list of options for a given resource type
        class EE_ENGINETOOLS_API OptionProvider : public TGlobalRegistryBase<OptionProvider>
        {
            EE_GLOBAL_REGISTRY( OptionProvider );

            friend class ResourcePicker;

        public:

            virtual ~OptionProvider() = default;
            virtual ResourceTypeID GetApplicableResourceTypeID() const = 0;
            virtual void GenerateOptions( ToolsContext const& m_toolsContext, TVector<ResourceID>& outOptions ) const = 0;
            virtual bool ValidatePath( ToolsContext const& m_toolsContext, ResourceID const& resourceID ) const = 0;
        };

    public:

        ResourcePicker( ToolsContext const& toolsContext, ResourceTypeID resourceTypeID = ResourceTypeID(), ResourceID const& resourceID = ResourceID() );

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the type of resource we wish to select
        void SetRequiredResourceType( ResourceTypeID resourceTypeID );

        // Set a custom filter for the generated options
        void SetCustomResourceFilter( TFunction<bool( Resource::ResourceDescriptor const* )>&& filter ) { m_customResourceFilter = filter; }

        // Clear any set custom filter
        void ClearCustomResourceFilter() { m_customResourceFilter = nullptr; }

        // Set the path
        void SetResourceID( ResourceID const& resourceID );

        // Get the resource path that was set
        inline ResourceID const& GetResourceID() const { return m_resourceID; }

    private:

        // Generate the set of valid resource options
        void GenerateResourceOptionsList();

        // Generate the set of filtered options
        void GenerateFilteredOptionList();

        // Try to update the resourceID from a paste operation - returns true if the value was updated
        bool TryUpdateResourceFromClipboard();

        // Try to update the resourceID from a drag and drop operation - returns true if the value was updated
        bool TryUpdateResourceFromDragAndDrop();

    private:

        ToolsContext const&                                         m_toolsContext;
        ResourceTypeID                                              m_resourceTypeID; // The type of resource we should pick from
        ResourceID                                                  m_resourceID;
        ImGuiX::FilterWidget                                        m_filterWidget;
        TVector<ResourceID>                                         m_generatedOptions;
        TVector<ResourceID>                                         m_filteredOptions;
        TFunction<bool( Resource::ResourceDescriptor const* )>      m_customResourceFilter;
        OptionProvider*                                             m_pCustomOptionProvider = nullptr;
        bool                                                        m_isComboOpen = false;
    };

    //-------------------------------------------------------------------------
    // Type Info Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TypeInfoPicker final
    {
        struct Option
        {
            TypeSystem::TypeInfo const*     m_pTypeInfo = nullptr;
            String                          m_friendlyName;
            String                          m_filterableData;
        };

    public:

        TypeInfoPicker( ToolsContext const& toolsContext, TypeSystem::TypeID const& baseClassTypeID = TypeSystem::TypeID(), TypeSystem::TypeInfo const*  pSelectedTypeInfo = nullptr );

        // Update the widget and draws it, returns true if the type was updated
        bool UpdateAndDraw();

        // Disable the picker
        void SetDisabled( bool isDisabled ) { m_isPickerDisabled = isDisabled; }

        // Set the type of resource we wish to select
        void SetRequiredBaseClass( TypeSystem::TypeID baseClassTypeID );

        // Set the selected type
        void SetSelectedType( TypeSystem::TypeInfo const* pSelectedType );

        // Set the selected type
        void SetSelectedType( TypeSystem::TypeID typeID );

        // Get the currently selected type
        inline TypeSystem::TypeInfo const* GetSelectedType() const { return m_pSelectedTypeInfo; }

    private:

        // Generate the set of valid resource options
        void GenerateOptionsList();

        // Generate the set of filtered options
        void GenerateFilteredOptionList();

        // Create a human readable typeinfo name
        String ConstructFriendlyTypeInfoName( TypeSystem::TypeInfo const* pTypeInfo );

    private:

        ToolsContext const&                                         m_toolsContext;
        TypeSystem::TypeID                                          m_baseClassTypeID;
        TypeSystem::TypeInfo const*                                 m_pSelectedTypeInfo = nullptr;
        ImGuiX::FilterWidget                                        m_filterWidget;
        TVector<Option>                                             m_generatedOptions;
        TVector<Option>                                             m_filteredOptions;
        bool                                                        m_isComboOpen = false;
        bool                                                        m_isPickerDisabled = false;
    };
}
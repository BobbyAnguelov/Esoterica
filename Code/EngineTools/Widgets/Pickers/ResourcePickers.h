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
    // Data File Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DataPickerBase
    {
        constexpr static float const s_controlsRowGapY = 2;

    public:

        DataPickerBase( ToolsContext const& toolsContext );
        virtual ~DataPickerBase() = default;

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the path
        virtual void SetDataPath( DataPath const& path ) = 0;

        // Get the resource path that was set
        virtual DataPath const& GetDataPath() const = 0;

        // Clear the set path
        virtual void Clear() { SetDataPath( DataPath() ); }

        // Get the height of the widget
        inline float GetHeight() const { return m_height; }

    protected:

        // Get the required extension/type to display in the preview
        virtual TInlineString<7> GetPreviewLabel() const;

        // Get the color for the preview label
        virtual Color GetPreviewColor() const { return ImGuiX::Style::s_colorText; }

        // Check if the supplied data path is a valid option for the current picker
        virtual bool ValidateDataPath( DataPath const& path ) = 0;

        // Check if the currently set data path is valid, and the file pointed to exists
        virtual bool ValidateCurrentlySetPath() { return ValidateDataPath( GetDataPath() ); }

        // Generate the set of valid resource options
        virtual void GenerateOptionsList() = 0;

        // Generate the set of filtered options
        virtual void GenerateFilteredOptionList();

        // Try to update the resourceID from a paste operation - returns true if the value was updated
        bool TryUpdatePathFromClipboard();

        // Try to update the resourceID from a drag and drop operation - returns true if the value was updated
        bool TryUpdatePathFromDragAndDrop();

    protected:

        ToolsContext const&                                     m_toolsContext;
        ImGuiX::FilterWidget                                    m_filterWidget;
        TVector<DataPath>                                       m_generatedOptions;
        TVector<DataPath>                                       m_filteredOptions;
        TVector<char>                                           m_tempBuffer;
        float                                                   m_height = 0;
        bool                                                    m_isPopupOpen = false;
        bool                                                    m_shouldOpenDropDown = false;
        bool                                                    m_showDependenciesButton = false;
    };

    //-------------------------------------------------------------------------
    // Data File Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DataFilePathPicker final : public DataPickerBase
    {
    public:

        DataFilePathPicker( ToolsContext const& toolsContext, TypeSystem::TypeID dataFileTypeID = TypeSystem::TypeID(), DataPath const& datafilePath = DataPath() );

        // Set the type of resource we wish to select
        void SetRequiredDataFileType( TypeSystem::TypeID typeID );

        virtual void SetDataPath( DataPath const& path ) override;
        virtual DataPath const& GetDataPath() const override { return m_path; }

    private:

        virtual bool ValidateDataPath( DataPath const& path ) override;
        virtual void GenerateOptionsList() override;

    private:

        TypeSystem::TypeID                                      m_fileTypeID; // The type of file we should pick from
        TypeSystem::DataFileInfo const*                         m_pDataFileInfo = nullptr; // Only set when we have a valid resource type ID
        FileSystem::Extension                                   m_requiredExtension;
        DataPath                                                m_path;
    };

    //-------------------------------------------------------------------------
    // Resource ID Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourcePicker final : public DataPickerBase
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
            virtual void GenerateOptions( ToolsContext const& m_toolsContext, TVector<DataPath>& outOptions ) const = 0;
            virtual bool ValidatePath( ToolsContext const& m_toolsContext, ResourceID const& resourceID ) const = 0;
        };

    public:

        ResourcePicker( ToolsContext const& toolsContext, ResourceTypeID resourceTypeID = ResourceTypeID(), ResourceID const& resourceID = ResourceID() );

        // Set the type of resource we wish to select
        void SetRequiredResourceType( ResourceTypeID resourceTypeID );

        // Set a custom filter for the generated options
        void SetCustomResourceFilter( TFunction<bool( Resource::ResourceDescriptor const* )>&& filter );

        // Clear any set custom filter
        void ClearCustomResourceFilter();

        // Set the path
        void SetResourceID( ResourceID const& resourceID );

        // Get the resource path that was set
        inline ResourceID const& GetResourceID() const { return m_resourceID; }

        virtual void SetDataPath( DataPath const& path ) override;
        virtual DataPath const& GetDataPath() const override { return m_resourceID.GetDataPath(); }

    private:

        virtual TInlineString<7> GetPreviewLabel() const override;
        virtual Color GetPreviewColor() const override;
        virtual bool ValidateDataPath( DataPath const& path ) override;
        virtual void GenerateOptionsList() override;

    private:

        ResourceTypeID                                              m_resourceTypeID; // The type of resource we should pick from
        TypeSystem::ResourceInfo const*                             m_pResourceTypeInfo = nullptr; // Only set when we have a valid resource type ID
        ResourceID                                                  m_resourceID;
        TFunction<bool( Resource::ResourceDescriptor const* )>      m_customResourceFilter;
        OptionProvider*                                             m_pCustomOptionProvider = nullptr;
    };
}
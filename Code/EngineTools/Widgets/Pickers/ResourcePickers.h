#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiFilter.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "Base/Imgui/ImguiTextBuffer.h"
#include "Base/Imgui/ImguiInputs.h"

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

        // Set whether this picker should draw in a compact mode or in the full version
        void SetCompactMode( bool isCompact ) { m_isCompact = isCompact; }

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the path
        void SetDataPath( DataPath const& path );

        // Get the resource path that was set
        DataPath const& GetDataPath() const { return m_path; }

        // Clear the set path
        virtual void Clear();

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
        bool ValidateCurrentlySetPath();

        // Try to update the resourceID from a drag and drop operation - returns true if the value was updated
        bool TryUpdatePathFromDragAndDrop();

    private:

        DataPickerBase( DataPickerBase const& ) = delete;
        DataPickerBase( DataPickerBase&& ) = delete;
        DataPickerBase& operator=( DataPickerBase const& ) = delete;
        DataPickerBase& operator=( DataPickerBase&& ) = delete;

        virtual void GenerateDataPathOptions() = 0;

        // Generate the set of options
        void GenerateOptionsList( TVector<ImGuiX::OptionData::Option>& outOptions );

        // Return true if it actually modifies the path
        bool UpdateDataPathFromString( String const& str );

    protected:

        ToolsContext const&                                     m_toolsContext;
        DataPath                                                m_path;

        ImGuiX::OptionData                                      m_optionData;
        TVector<DataPath>                                       m_dataPathOptions;

        ImGuiX::TextBuffer                                      m_buffer;
        float                                                   m_height = 0;
        bool                                                    m_showDependenciesButton = false;
        bool                                                    m_isCompact = false;
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

    private:

        virtual bool ValidateDataPath( DataPath const& path ) override;
        virtual void GenerateDataPathOptions() override;

    private:

        TypeSystem::TypeID                                      m_fileTypeID; // The type of file we should pick from
        TypeSystem::DataFileInfo const*                         m_pDataFileInfo = nullptr; // Only set when we have a valid resource type ID
        FileSystem::Extension                                   m_requiredExtension;
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
        void SetCustomResourceFilter( TFunction<bool( Resource::ResourceDescriptor const* )>&& filter ) { m_customResourceFilter = eastl::move( filter ); }

        // Clear any set custom filter
        void ClearCustomResourceFilter() { m_customResourceFilter = nullptr; }

        // Set the path
        void SetResourceID( ResourceID const& resourceID );

        // Get the resource path that was set
        inline ResourceID GetResourceID() const { return m_path.IsValid() ? ResourceID( m_path ) : ResourceID(); }

    private:

        virtual TInlineString<7> GetPreviewLabel() const override;
        virtual Color GetPreviewColor() const override;
        virtual bool ValidateDataPath( DataPath const& path ) override;
        virtual void GenerateDataPathOptions() override;

    private:

        ResourceTypeID                                              m_resourceTypeID; // The type of resource we should pick from
        TypeSystem::ResourceInfo const*                             m_pResourceTypeInfo = nullptr; // Only set when we have a valid resource type ID
        TFunction<bool( Resource::ResourceDescriptor const* )>      m_customResourceFilter;
        OptionProvider*                                             m_pCustomOptionProvider = nullptr;
    };
}
#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Utils/GlobalRegistryBase.h"

//-------------------------------------------------------------------------

namespace EE { class ToolsContext; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceDatabase;

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

        ToolsContext const&         m_toolsContext;
        ResourceTypeID              m_resourceTypeID; // The type of resource we should pick from
        ResourceID                  m_resourceID;
        ImGuiX::FilterWidget        m_filterWidget;
        TVector<ResourceID>         m_generatedOptions;
        TVector<ResourceID>         m_filteredOptions;
        OptionProvider*             m_pCustomOptionProvider = nullptr;
        bool                        m_isComboOpen = false;
    };

    //-------------------------------------------------------------------------
    // Resource Path Picker
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourcePathPicker final
    {
    public:

        ResourcePathPicker( FileSystem::Path const& rawResourceDirectoryPath, ResourcePath const& path = ResourcePath() )
            : m_rawResourceDirectoryPath( rawResourceDirectoryPath )
            , m_resourcePath( path )
        { 
            EE_ASSERT( m_rawResourceDirectoryPath.IsValid() && m_rawResourceDirectoryPath.Exists() );
        }

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the path
        void SetPath( ResourcePath const& path ) { m_resourcePath = path; }

        // Get the resource path that was set
        inline ResourcePath const& GetPath() const { return m_resourcePath; }

        // Try to update the path from a drag and drop operation - returns true if the value was updated
        bool TrySetPathFromDragAndDrop();

    private:

        FileSystem::Path const      m_rawResourceDirectoryPath;
        ResourcePath                m_resourcePath;
    };
}
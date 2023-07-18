#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE { class ToolsContext; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceDatabase;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourcePicker final
    {

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

    private:

        ToolsContext const&         m_toolsContext;
        ResourceTypeID              m_resourceTypeID; // The type of resource we should pick from
        ResourceID                  m_resourceID;
        ImGuiX::FilterWidget        m_filterWidget;
        TVector<ResourceID>         m_allResourceIDs;
        TVector<ResourceID>         m_filteredResourceIDs;
        bool                        m_isComboOpen = false;
    };

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

    private:

        FileSystem::Path const      m_rawResourceDirectoryPath;
        ResourcePath                m_resourcePath;
    };
}
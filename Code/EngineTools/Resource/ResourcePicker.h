#pragma once
#include "EngineTools/_Module/API.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------
// Resource Picker
//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceDatabase;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourcePicker final
    {
        struct PickerOption
        {
            PickerOption( ResourceID const& resourceID );

            inline bool operator==( ResourceID const& resourceID ) const { return m_resourceID == resourceID; }
            inline bool operator==( ResourceID& resourceID ) const { return m_resourceID == resourceID; }
            inline bool operator!=( ResourceID const& resourceID ) const { return m_resourceID != resourceID; }
            inline bool operator!=( ResourceID& resourceID ) const { return m_resourceID != resourceID; }

            ResourceID                  m_resourceID;
            String                      m_filename;
            String                      m_parentFolder;
        };

        struct Result
        {
            ResourcePath m_newPath;
            bool m_wasPathUpdated;
        };

    public:

        ResourcePicker( ToolsContext const& toolsContext );

        // Draw a picker for a specified resource type - return true if the resource path is updated
        template<typename T>
        bool DrawPicker( TResourcePtr<T> const& ptr, ResourcePath& outPath )
        {
            static_assert( std::is_base_of<IResource, T>::value, "T has to derive from IResource" );
            return DrawPicker( ptr.GetResourcePath(), outPath, true, T::GetStaticResourceTypeID() );
        }

        // Draw a picker restricted to only registered resources - return true if the resource path is updated
        bool DrawPicker( ResourcePtr const& ptr, ResourcePath& outPath )
        {
            return DrawPicker( ptr.GetResourcePath(), outPath, true, ResourceTypeID() );
        }

        // Draw a picker restricted to only registered resources - return true if the resource path is updated
        bool DrawPicker( ResourceID const& resourceID, ResourcePath& outPath, ResourceTypeID restrictedType = ResourceTypeID() )
        {
            return DrawPicker( resourceID.GetResourcePath(), outPath, true, restrictedType );
        }

        // Draw a picker for any valid resource path - return true if the resource path is updated
        bool DrawPicker( ResourcePath const& resourcePath, ResourcePath& outPath )
        {
            return DrawPicker( resourcePath, outPath, false, ResourceTypeID() );
        }

    private:

        bool DrawPicker( ResourcePath const& resourcePath, ResourcePath& outPath, bool restrictToResources, ResourceTypeID restrictedResourceTypeID );

        // Draw the selection dialog, returns true if the dialog is closed
        bool DrawDialog();

        // Generate the set of valid resource options
        void GenerateResourceOptionsList( ResourceTypeID resourceTypeID );

        // Generate the set of filtered options
        void GeneratedFilteredOptionList();

    private:

        ToolsContext const&             m_toolsContext;

        char                            m_filterBuffer[256];
        TVector<PickerOption>           m_allResourceIDs;
        TVector<PickerOption>           m_filteredResourceIDs;
        ResourceID                      m_currentlySelectedID;
        bool                            m_initializeFocus = false;
        bool                            m_shouldOpenPickerDialog = false;
    };
}
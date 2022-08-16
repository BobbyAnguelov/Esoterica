#pragma once
#include "EngineTools/_Module/API.h"
#include "System/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceDatabase;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceFilePicker final
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

    public:

        ResourceFilePicker( ToolsContext const& toolsContext );

        // Get the raw resource data path
        FileSystem::Path const& GetRawResourceDirectoryPath() const;

        // Draws the resource field - returns true if the value is changed
        // The force flag is to be used when we reused the same class to draw multiple pickers
        // TODO: clean this up, this is not a elegant approach.
        bool DrawResourcePicker( ResourceTypeID resourceTypeID, ResourceID const* pResourceID, bool forceRefreshValidity = false );

        // Draws a generic path picker
        bool DrawPathPicker( ResourcePath* pResourcePath );

        // Get the selected ID - Only use if DrawPicker returns true
        inline ResourceID const GetSelectedResourceID() const { return m_selectedID; }

    private:

        // Draw the selection dialog, returns true if the dialog is closed
        bool DrawDialog( ResourceTypeID resourceTypeID, ResourceID const* pResourceID );

        void RefreshResourceList( ResourceTypeID resourceTypeID );

    private:

        ToolsContext const&             m_toolsContext;
        char                            m_filterBuffer[256];
        TVector<PickerOption>           m_knownResourceIDs;
        TVector<PickerOption>           m_filteredResourceIDs;
        ResourceID                      m_selectedID;
        bool                            m_initializeFocus = false;
    };
}
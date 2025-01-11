#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/DataPath.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
  
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
}
#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/DataPath.h"
#include "Base/Imgui/ImguiTextBuffer.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
  
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DataPathPicker final
    {

    public:

        enum class Mode
        {
            PickFiles,
            PickDirectories,
            PickFilesAndDirectories
        };

    public:

        DataPathPicker( FileSystem::Path const& sourceDataDirectoryPath, DataPath const& path = DataPath() );

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw( float width = -1 );

        // Set the path
        void SetPath( DataPath const& path );

        // Get the resource path that was set
        inline DataPath const& GetPath() const { return m_dataPath; }

        // Are we allow to select directories or just files?
        void SetPickingMode( Mode mode ) { m_mode = mode; }

    private:

        bool TryUpdateDataPath( DataPath const& newPath );

        void PickDirectory( bool& outPathUpdated );
        void PickFile( bool& outPathUpdated );

    private:

        FileSystem::Path const      m_sourceDataDirectoryPath;
        DataPath                    m_dataPath;
        ImGuiX::TextBuffer          m_dataPathBuffer;
        Mode                        m_mode = Mode::PickFilesAndDirectories;
    };
}
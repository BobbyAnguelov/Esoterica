#include "EditorTool_MemoryTracker.h"

//-------------------------------------------------------------------------

namespace EE
{
    MemoryTrackerEditorTool::MemoryTrackerEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Memory Tracker" )
    {}

    void MemoryTrackerEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Memory Tracker", [this] ( UpdateContext const& context, bool isFocused ) { DrawWindow( context, isFocused ); } );
    }

    void MemoryTrackerEditorTool::DrawWindow( UpdateContext const& context, bool isFocused )
    {
        m_memoryTracker.UpdateAndDraw( context );
    }
}
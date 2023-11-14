#include "EditorTool_SystemLog.h"

//-------------------------------------------------------------------------

namespace EE
{
    SystemLogEditorTool::SystemLogEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "System Log" )
    {}

    void SystemLogEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Log", [this] ( UpdateContext const& context, bool isFocused ) { DrawLogWindow( context, isFocused ); } );
    }

    void SystemLogEditorTool::DrawLogWindow( UpdateContext const& context, bool isFocused )
    {
        m_logViewWidget.Draw( context );
    }
}
#include "EditorTool_ResourceSystem.h"
#include "Engine/DebugViews/DebugView_Resource.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceSystemEditorTool::ResourceSystemEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource System" )
    {}

    void ResourceSystemEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Overview", [this] ( UpdateContext const& context, bool isFocused ) { DrawOverviewWindow( context, isFocused ); } );
        CreateToolWindow( "Request History", [this] ( UpdateContext const& context, bool isFocused ) { DrawRequestHistoryWindow( context, isFocused ); } );
    }

    void ResourceSystemEditorTool::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, rightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.5f, &leftDockID, &rightDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Overview" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Request History" ).c_str(), rightDockID );
    }

    void ResourceSystemEditorTool::DrawOverviewWindow( UpdateContext const& context, bool isFocused )
    {
        ResourceDebugView::DrawResourceSystemOverview( m_pResourceSystem );
    }

    void ResourceSystemEditorTool::DrawRequestHistoryWindow( UpdateContext const& context, bool isFocused )
    {
        ResourceDebugView::DrawRequestHistory( m_pResourceSystem );
    }
}
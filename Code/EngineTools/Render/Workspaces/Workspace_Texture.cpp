#include "Workspace_Texture.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_WORKSPACE_FACTORY( TextureWorkspaceFactory, Texture, TextureWorkspace );

    //-------------------------------------------------------------------------

    void TextureWorkspace::Initialize( UpdateContext const& context )
    {
        TWorkspace<Texture>::Initialize( context );
        CreateToolWindow( "Preview", [this] ( UpdateContext const& context, bool isFocused ) { DrawPreviewWindow( context, isFocused ); } );
        CreateToolWindow( "Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawInfoWindow( context, isFocused ); } );
    }

    void TextureWorkspace::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Preview" ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomDockID);
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Info" ).c_str(), bottomDockID );
    }

    void TextureWorkspace::DrawInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
        }
        else if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_workspaceResource.GetResourceID().c_str() );
        }
        else
        {
            ImGui::Text( "Dimensions: %d x %d", m_workspaceResource->GetDimensions().m_x, m_workspaceResource->GetDimensions().m_y );
        }
    }

    void TextureWorkspace::DrawPreviewWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsResourceLoaded() )
        {
            ImGui::Image( ImGuiX::ToIm( m_workspaceResource.GetPtr() ), Float2( m_workspaceResource->GetDimensions() ) );
        }
    }
}
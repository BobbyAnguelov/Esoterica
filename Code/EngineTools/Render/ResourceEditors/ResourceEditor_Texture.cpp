#include "ResourceEditor_Texture.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY( TextureEditorFactory, Texture, TextureEditor );

    //-------------------------------------------------------------------------

    void TextureEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor<Texture>::Initialize( context );
        CreateToolWindow( "Preview", [this] ( UpdateContext const& context, bool isFocused ) { DrawPreviewWindow( context, isFocused ); } );
        CreateToolWindow( "Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawInfoWindow( context, isFocused ); } );
    }

    void TextureEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Preview" ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomDockID);
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Info" ).c_str(), bottomDockID );
    }

    void TextureEditor::DrawInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
        }
        else if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_editedResource.GetResourceID().c_str() );
        }
        else
        {
            ImGui::Text( "Dimensions: %d x %d", m_editedResource->GetDimensions().m_x, m_editedResource->GetDimensions().m_y );
        }
    }

    void TextureEditor::DrawPreviewWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsResourceLoaded() )
        {
            ImGui::Image( ImGuiX::ToIm( m_editedResource.GetPtr() ), Float2( m_editedResource->GetDimensions() ) );
        }
    }
}
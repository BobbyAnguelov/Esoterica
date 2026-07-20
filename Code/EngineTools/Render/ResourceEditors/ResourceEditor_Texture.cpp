#include "ResourceEditor_Texture.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Imgui/ImguiImageCache.h"
#include "Engine/UpdateContext.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY_NO_WORLD( TextureEditorFactory, TextureResource, TextureEditor );

    //-------------------------------------------------------------------------

    void TextureEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor<TextureResource>::Initialize( context );
        CreateToolWindow( "Preview", [this] ( UpdateContext const& context, bool isFocused ) { DrawPreviewWindow( context, isFocused ); } );

        RenderSystem* pRenderSystem = context.GetSystem<RenderSystem>();
        m_pPointWrapSampler = pRenderSystem->GetPointWrapSampler();
    }

    void TextureEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0;
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Right, 0.35f, nullptr, &leftDockID );

        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Preview" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), rightDockID );
    }

    void TextureEditor::DrawPreviewWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        ImVec2 const availableSpace = ImGui::GetContentRegionAvail();
        float const imageWidth = float( m_editedResource->GetWidth() );
        float const imageHeight = float( m_editedResource->GetHeight() );

        float widthRatio = availableSpace.x / imageWidth;
        float heightRatio = availableSpace.y / imageHeight;

        ImVec2 imageSize;
        if ( widthRatio < heightRatio )
        {
            imageSize.x = availableSpace.x;
            imageSize.y = availableSpace.x / ( imageHeight / imageWidth );
        }
        else
        {
            imageSize.y = availableSpace.y;
            imageSize.x = availableSpace.y / ( imageWidth/ imageHeight );
        }

        ImTextureID const textureID = ImGuiX::GetImTextureID( m_pPointWrapSampler, m_editedResource->GetTexture() );
        ImGui::Image( textureID, imageSize );
    }

    void TextureEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        ImGui::NewLine();
        ImGuiX::DrawShadowedText( Colors::Yellow, "Dimensions: %d x %d", m_editedResource->GetWidth(), m_editedResource->GetHeight() );
    }
}
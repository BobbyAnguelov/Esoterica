#include "EditorTool_RawFileInspector_Image.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryPNG, "png", ImageFileInspectorEditorTool );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryBMP, "bmp", ImageFileInspectorEditorTool );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryTGA, "tga", ImageFileInspectorEditorTool );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryJPG, "jpg", ImageFileInspectorEditorTool );

    //-------------------------------------------------------------------------

    ImageFileInspectorEditorTool::ImageFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath )
        : RawFileInspectorEditorTool( pToolsContext, rawFilePath )
    {}

    void ImageFileInspectorEditorTool::UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused )
    {
        if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Create Texture Descriptor", ImVec2( 250, 0 ) ) )
        {
            Render::TextureResourceDescriptor resourceDesc;

            FileSystem::Path savePath;
            if ( !TryGetDestinationFilenameForDescriptor( &resourceDesc, savePath ) )
            {
                return;
            }

            //-------------------------------------------------------------------------

            resourceDesc.m_path = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), m_filePath );

            TrySaveDescriptorToDisk( &resourceDesc, savePath );
        }
    }
}
#include "RawFileInspector_Images.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryPNG, "png", RawFileInspectorImages );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryBMP, "bmp", RawFileInspectorImages );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryTGA, "tga", RawFileInspectorImages );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryJPG, "jpg", RawFileInspectorImages );

    //-------------------------------------------------------------------------

    RawFileInspectorImages::RawFileInspectorImages( ToolsContext const* pToolsContext, FileSystem::Path const& filePath )
        : RawFileInspector( pToolsContext, filePath )
    {
        EE_ASSERT( FileSystem::Exists( filePath ) );
        auto pDesc = EE::New<Render::TextureResourceDescriptor>();
        pDesc->m_path = ResourcePath::FromFileSystemPath( m_rawResourceDirectory, m_filePath );
        m_pDescriptor = pDesc;

        m_propertyGrid.SetTypeToEdit( m_pDescriptor );
    }

    void RawFileInspectorImages::DrawFileInfo()
    {
        ImGui::Text( m_filePath.c_str() );
    }

    void RawFileInspectorImages::DrawFileContents()
    {
    }

    void RawFileInspectorImages::DrawResourceDescriptorCreator()
    {
        if ( ImGui::Button( EE_ICON_PLUS " Create Texture", ImVec2( -1, 0 ) ) )
        {
            CreateNewDescriptor( Render::Texture::GetStaticResourceTypeID(), m_pDescriptor );
        }
    }
}
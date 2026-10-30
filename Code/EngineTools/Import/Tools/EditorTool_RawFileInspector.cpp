#include "EditorTool_RawFileInspector.h"
#include "EngineTools/Import/Importer.h"
#include "EngineTools/Import/ImportedMaterial.h"
#include "EngineTools/Import/ImportedImage.h"

//-------------------------------------------------------------------------

namespace EE::Import
{

    bool RawFileInspector::IsExtensionInspectable( FileSystem::Extension const& ext )
    {
        if ( RawImageInspectorEditorTool::SupportsExtension( ext ) )
        {
            return true;
        }

        if ( RawSceneInspectorEditorTool::SupportsExtension( ext ) )
        {
            return true;
        }

        return false;
    }

    bool RawFileInspector::RequiresWorld( FileSystem::Extension const& ext )
    {
        if ( RawSceneInspectorEditorTool::SupportsExtension( ext ) )
        {
            return true;
        }

        return false;
    }

    EditorTool* RawFileInspector::CreateInspector( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld )
    {
        FileSystem::Extension const ext = dataPath.GetExtension();

        if ( RawImageInspectorEditorTool::SupportsExtension( ext ) )
        {
            return EE::New<RawImageInspectorEditorTool>( pToolsContext, dataPath, pWorld );
        }

        if ( RawSceneInspectorEditorTool::SupportsExtension( ext ) )
        {
            return EE::New<RawSceneInspectorEditorTool>( pToolsContext, dataPath, pWorld );
        }

        EE_HALT();
        return nullptr;
    }
}

//-------------------------------------------------------------------------
// Image Inspector
//-------------------------------------------------------------------------

namespace EE::Import
{
    TArray<FileSystem::Extension, 12> const RawImageInspectorEditorTool::supportedExtensions = { "png", "tga", "jpg", "jpeg", "bmp", "hdr", "exr", "gif", "psd", };

    //-------------------------------------------------------------------------

    bool RawImageInspectorEditorTool::SupportsExtension( FileSystem::Extension const& ext )
    {
        return eastl::find( supportedExtensions.begin(), supportedExtensions.end(), ext ) != supportedExtensions.end();
    }

    RawImageInspectorEditorTool::RawImageInspectorEditorTool( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld )
        : EditorTool( pToolsContext, dataPath.GetFilename().c_str(), pWorld )
        , m_dataPath( dataPath )
        , m_filePath( dataPath.GetFileSystemPath( pToolsContext->GetSourceDataDirectory() ) )
    {
        SetUniqueID( Hash::FNV1a::GetHash32( m_dataPath.c_str() ) );

        Import::ReaderContext readerCtx =
        {
            m_pToolsContext->GetSourceDataDirectory(),
            [this] ( char const* pString ) { m_log.LogWarning( pString ); },
            [this] ( char const* pString ) { m_log.LogError( pString ); },
        };

        m_imageFile = Importer::LoadImageFile( readerCtx, m_filePath );
        m_image = m_imageFile->ReadImage();

        if ( m_image != nullptr && m_image->IsValid() )
        {
            m_previewImage = m_pToolsContext->m_pImageCache->LoadImageFromMemory( m_image->GetImageData(), m_image->GetWidth(), m_image->GetHeight(), m_image->GetNumChannels() );
        }
    }

    RawImageInspectorEditorTool::~RawImageInspectorEditorTool()
    {

    }

    void RawImageInspectorEditorTool::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), dockspaceID );
    }

    void RawImageInspectorEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
    }

    void RawImageInspectorEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {

    }

    //-------------------------------------------------------------------------

    void RawImageInspectorEditorTool::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( m_previewImage.m_ID != 0 )
        {
            ImGui::Image( m_previewImage.m_ID, ImGui::GetContentRegionAvail() );
        }
    }
}

//-------------------------------------------------------------------------
// Scene Inspector
//-------------------------------------------------------------------------

namespace EE::Import
{
    TArray<FileSystem::Extension, 12> const RawSceneInspectorEditorTool::supportedExtensions = { "fbx", "gltf", "glb", };

    //-------------------------------------------------------------------------

    bool RawSceneInspectorEditorTool::SupportsExtension( FileSystem::Extension const& ext )
    {
        return eastl::find( supportedExtensions.begin(), supportedExtensions.end(), ext ) != supportedExtensions.end();
    }

    RawSceneInspectorEditorTool::RawSceneInspectorEditorTool( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld )
        : EditorTool( pToolsContext, dataPath.GetFilename().c_str(), pWorld )
        , m_dataPath( dataPath )
        , m_filePath( dataPath.GetFileSystemPath( pToolsContext->GetSourceDataDirectory() ) )
    {
        SetUniqueID( Hash::FNV1a::GetHash32( m_dataPath.c_str() ) );

        Import::ReaderContext readerCtx =
        {
            m_pToolsContext->GetSourceDataDirectory(),
            [this] ( char const* pString ) { m_log.LogWarning( pString ); },
            [this] ( char const* pString ) { m_log.LogError( pString ); },
        };

        m_sceneFile = Importer::LoadSceneFile( readerCtx, m_filePath );

        if ( m_sceneFile != nullptr && m_sceneFile->IsValid() )
        {
            m_sceneFile->GetFileContents( m_fileContents );
            m_materials = m_sceneFile->ReadMaterials();
        }
    }

    RawSceneInspectorEditorTool::~RawSceneInspectorEditorTool()
    {
        
    }

    void RawSceneInspectorEditorTool::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID0 = 0;
        ImGuiID leftDockID1 = 0;
        ImGuiID centerDockID = 0;

        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.5f, &leftDockID0, &centerDockID );
        ImGui::DockBuilderSplitNode( leftDockID0, ImGuiDir_Right, 0.5f, &leftDockID1, &leftDockID0 );

        ImGui::DockBuilderDockWindow( GetToolWindowName( "File Outline" ).c_str(), leftDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Item Details" ).c_str(), leftDockID1 );
    }

    void RawSceneInspectorEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );

        CreateToolWindow( "File Outline", [this] ( UpdateContext const& context, bool isFocused ) { DrawSceneFileOutlineWindow( context, isFocused ); } );
        CreateToolWindow( "Item Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawSceneFileItemDetailsWindow( context, isFocused ); } );
    }

    void RawSceneInspectorEditorTool::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {

    }

    //-------------------------------------------------------------------------

    void RawSceneInspectorEditorTool::SetSelectedItem( SceneFile::Item const& item )
    {
        m_selectedItem = item;

        if ( m_selectedItem.m_type == SceneFile::ItemType::Material )
        {
            return;
        }
    }

    void RawSceneInspectorEditorTool::DrawSceneFileOutlineWindow( UpdateContext const& context, bool isFocused )
    {
        if ( m_sceneFile == nullptr )
        {
            String str;
            m_log.SaveLogToString( str );
            ImGui::TextUnformatted( str.c_str() );
            return;
        }

        //-------------------------------------------------------------------------

        if ( !m_sceneFile->IsValid() )
        {
            ImGui::TextUnformatted( m_sceneFile->GetErrorMessage().c_str() );
            return;
        }

        //-------------------------------------------------------------------------
        // Draw outline
        //-------------------------------------------------------------------------

        DrawItems( "Static Meshes", m_fileContents.m_staticMeshes );
        DrawItems( "Skeletal Meshes",  m_fileContents.m_skeletalMeshes );
        DrawItems( "Materials", m_fileContents.m_materials );
        DrawItems( "Animation Skeletons", m_fileContents.m_skeletons );
        DrawItems( "Animations", m_fileContents.m_animations );
    }

    void RawSceneInspectorEditorTool::DrawSceneFileItemDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !m_selectedItem.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_selectedItem.m_type == SceneFile::ItemType::Material )
        {
            DrawSelectedMaterialDetails();
        }
    }

    void RawSceneInspectorEditorTool::DrawSelectedMaterialDetails()
    {
        EE_ASSERT( m_selectedItem.IsValid() && m_selectedItem.m_type == SceneFile::ItemType::Material );

        for ( auto const& material : *( m_materials.get() ) )
        {
            if ( material.m_ID != m_selectedItem.m_ID )
            {
                continue;
            }

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::LargeBold, Colors::Gold );
                ImGui::Text( "Material: %s", material.m_ID.c_str());
            }

            //-------------------------------------------------------------------------

            auto DrawPropertyDetails = [] ( char const* pLabel, Material::Property const& prop )
            {
                ImGui::SeparatorText( pLabel );
                ImGui::Text( "Texture: %s", prop.m_rawTexturePathStr.empty() ? "None" : prop.m_rawTexturePathStr.c_str() );

                if ( prop.m_isValueSet )
                {
                    ImGui::ColorButton( "##color", prop.m_value );
                    ImGui::SameLine();
                    ImGui::Text( "R: %d, G: %d, B: %d, A: %d", prop.m_value.m_byteColor.m_r, prop.m_value.m_byteColor.m_g, prop.m_value.m_byteColor.m_b, prop.m_value.m_byteColor.m_a );
                }
            };

            DrawPropertyDetails( "Albedo", material.m_albedo );
            DrawPropertyDetails( "Normal", material.m_normal );
            DrawPropertyDetails( "Metalness", material.m_metalness );
            DrawPropertyDetails( "Roughness", material.m_roughness );
            DrawPropertyDetails( "Occlusion", material.m_occlusion );
            DrawPropertyDetails( "Displacement", material.m_displacement );
            DrawPropertyDetails( "Emissive", material.m_emissive );

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "General Settings" );

            ImGui::Text( "Emissive Strength: %.3f", material.m_emissiveStrength );
            ImGui::Text( "Opacity: %.3f", material.m_opacity );
            ImGui::Text( "Has Alpha: %s", material.m_hasAlpha ? "True" : "False" );
            ImGui::Text( "Is Two Sided: %s", material.m_isTwoSided ? "True" : "False" );
            ImGui::Text( "Is Specular Glossiness: %s", material.m_isSpecularGlossiness ? "True" : "False" );
        }
    }
}
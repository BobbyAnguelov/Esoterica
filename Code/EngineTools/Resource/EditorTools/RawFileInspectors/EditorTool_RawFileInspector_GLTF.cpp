#include "EditorTool_RawFileInspector_GLTF.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryGltf, "gltf", GLTFFileInspectorEditorTool );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryGlb, "glb", GLTFFileInspectorEditorTool );

    //-------------------------------------------------------------------------

    GLTFFileInspectorEditorTool::GLTFFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath )
        : RawFileInspectorEditorTool( pToolsContext, rawFilePath )
        , m_refreshFileInfo( true )
    {}

    void GLTFFileInspectorEditorTool::UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused )
    {
        // Load File
        //-------------------------------------------------------------------------

        if ( m_refreshFileInfo )
        {
            m_sceneContext.LoadFile( m_filePath );

            if ( m_sceneContext.IsValid() )
            {
                ReadFileContents();
            }

            m_refreshFileInfo = false;
        }

        // Display File Data
        //-------------------------------------------------------------------------

        if ( !m_sceneContext.IsValid() )
        {
            ImGui::Text( "Invalid FBX File" );
            ImGui::Text( m_sceneContext.GetErrorMessage().c_str() );
        }
        else
        {
            // File Info
            //-------------------------------------------------------------------------

            if ( ImGui::CollapsingHeader( "File Info", ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                ImGui::Text( "Path: %s", m_filePath.c_str() );

                if ( m_sceneContext.HasWarningOccurred() )
                {
                    ImGui::Text( "Warning: %s", m_sceneContext.GetWarningMessage().c_str() );
                }

                if ( ImGuiX::IconButton( EE_ICON_REFRESH, "Reload File" ) )
                {
                    m_refreshFileInfo = true;
                }
            }

            ImGui::Text( "TODO" );
        }
    }

    void GLTFFileInspectorEditorTool::ReadFileContents()
    {
        // TODO
    }

    //-------------------------------------------------------------------------

    //static void ReadMeshInfo( gltfSceneContext const& sceneCtx, RawAssets::RawAssetInfo& outInfo )
    //{
    //    cgltf_data const* pSceneData = sceneCtx.GetSceneData();
    //    for ( auto m = 0; m < pSceneData->meshes_count; m++ )
    //    {
    //        RawAssets::RawMeshInfo meshInfo;
    //        meshInfo.m_name = pSceneData->meshes[m].name;
    //        meshInfo.m_isSkinned = pSceneData->meshes[m].weights_count > 0;
    //        //meshInfo.m_materialName = pSceneData->meshes[m].name;
    //        outInfo.m_meshes.emplace_back( meshInfo );
    //    }
    //}

    //static void ReadAnimationInfo( gltfSceneContext const& sceneCtx, RawAssets::RawAssetInfo& outInfo )
    //{
    //    cgltf_data const* pSceneData = sceneCtx.GetSceneData();
    //    for ( auto i = 0; i < pSceneData->animations_count; i++ )
    //    {
    //        RawAssets::RawAnimationInfo animInfo;
    //        animInfo.m_name = pSceneData->animations[i].name;

    //        float animationDuration = -1.0f;
    //        size_t numFrames = 0;
    //        for ( auto s = 0; s < pSceneData->animations[i].samplers_count; s++ )
    //        {
    //            cgltf_accessor const* pInputAccessor = pSceneData->animations[i].samplers[s].input;
    //            EE_ASSERT( pInputAccessor->has_max );
    //            animationDuration = Math::Max( pInputAccessor->max[0], animationDuration );
    //            numFrames = Math::Max( pInputAccessor->count, numFrames );
    //        }

    //        animInfo.m_duration = animationDuration;
    //        animInfo.m_frameRate = animationDuration / numFrames;

    //        outInfo.m_animations.emplace_back( animInfo );
    //    }
    //}

    //static void ReadSkeletonInfo( gltfSceneContext const& sceneCtx, RawAssets::RawAssetInfo& outInfo )
    //{
    //    cgltf_data const* pSceneData = sceneCtx.GetSceneData();
    //    for ( int32_t i = 0; i < pSceneData->skins_count; i++ )
    //    {
    //        RawAssets::RawSkeletonInfo skeletonInfo;
    //        skeletonInfo.m_name = pSceneData->skins[i].joints[0]->name;
    //        outInfo.m_skeletons.emplace_back( skeletonInfo );
    //    }
    //}

    //bool ReadFileInfo( FileSystem::Path const& sourceFilePath, RawAssets::RawAssetInfo& outInfo )
    //{
    //    gltfSceneContext sceneCtx( sourceFilePath );
    //    if ( !sceneCtx.IsValid() )
    //    {
    //        return false;
    //    }

    //    outInfo.m_upAxis = Axis::Y;

    //    //-------------------------------------------------------------------------

    //    ReadMeshInfo( sceneCtx, outInfo );
    //    ReadAnimationInfo( sceneCtx, outInfo );
    //    ReadSkeletonInfo( sceneCtx, outInfo );

    //    return true;
    //}
}
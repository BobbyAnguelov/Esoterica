#include "RawFileInspector_GLTF.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathUtils.h"
#include "System/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryGltf, "gltf", RawFileInspectorGLTF );
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryGlb, "glb", RawFileInspectorGLTF );

    //-------------------------------------------------------------------------

    RawFileInspectorGLTF::RawFileInspectorGLTF( ToolsContext const* pToolsContext, FileSystem::Path const& filePath )
        : RawFileInspector( pToolsContext, filePath )
        , m_sceneContext( filePath )
    {
        EE_ASSERT( FileSystem::Exists( filePath ) );
        
    }

    void RawFileInspectorGLTF::DrawFileInfo()
    {
        ImGui::Text( "Raw File: %s", m_filePath.c_str() );
    }

    void RawFileInspectorGLTF::DrawFileContents()
    {
        ImGui::Text( "Raw File: %s", m_filePath.c_str() );

        //ImGui::Text( "Original Up Axis: %s", Math::ToString( m_assetInfo.m_upAxis ) );
        //ImGui::Text( "Scale: %.2f", m_assetInfo.m_scale );

        //if ( m_validAssetInfo )
        //{
        //    ImGui::Text( "Original Up Axis: %s", Math::ToString( m_assetInfo.m_upAxis ) );
        //    ImGui::Text( "Scale: %.2f", m_assetInfo.m_scale );

        //    if ( ImGui::Button( EE_ICON_CUBE " Create Static Mesh", ImVec2( -1, 24 ) ) )
        //    {
        //        Render::StaticMeshResourceDescriptor resourceDesc;
        //        resourceDesc.m_meshPath = ResourcePath::FromFileSystemPath( m_pModel->GetSourceResourceDirectory(), m_filePath );
        //        CreateNewDescriptor( Render::StaticMesh::GetStaticResourceTypeID(), resourceDesc );
        //    }

        //    ImGui::Separator();

        //    // Asset Info
        //    //-------------------------------------------------------------------------

        //    ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg;
        //    ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 4 ) );
        //    if ( ImGui::BeginTable( "Raw File Contents", 3, 0 ) )
        //    {
        //        ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed, 80 );
        //        ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide );
        //        ImGui::TableSetupColumn( "Controls", ImGuiTableColumnFlags_WidthFixed, 20 );

        //        // Static Meshes
        //        //-------------------------------------------------------------------------

        //        for ( auto const& mesh : m_assetInfo.m_meshes )
        //        {
        //            ImGui::TableNextRow();
        //            ImGui::PushID( &mesh );

        //            // Type
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGui::AlignTextToFramePadding();
        //            ImGui::Text( "Static Mesh" );

        //            // Name
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGui::PushID( "StaticMesh" );
        //            ImGuiX::SelectableText( mesh.m_name, -1 );
        //            ImGui::PopID();

        //            // Options
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            DrawStaticMeshControls( mesh );

        //            ImGui::PopID();
        //        }

        //        // Skeletal Meshes
        //        //-------------------------------------------------------------------------

        //        for ( auto const& mesh : m_assetInfo.m_meshes )
        //        {
        //            if ( !mesh.m_isSkinned )
        //            {
        //                continue;
        //            }

        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextRow();
        //            ImGui::PushID( &mesh );

        //            // Type
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGui::AlignTextToFramePadding();
        //            ImGui::Text( "Skeletal Mesh" );

        //            // Name
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGui::PushID( "SkeletalMesh" );
        //            ImGuiX::SelectableText( mesh.m_name, -1 );
        //            ImGui::PopID();

        //            // Options
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            DrawSkeletalMeshControls( mesh );

        //            ImGui::PopID();
        //        }

        //        // Skeletons
        //        //-------------------------------------------------------------------------

        //        for ( auto const& skeleton : m_assetInfo.m_skeletons )
        //        {
        //            ImGui::TableNextRow();
        //            ImGui::PushID( &skeleton );

        //            // Type
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGui::AlignTextToFramePadding();
        //            ImGui::Text( "Skeleton" );

        //            // Name
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGuiX::SelectableText( skeleton.m_name, -1 );

        //            // Options
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            DrawSkeletonControls( skeleton );

        //            ImGui::PopID();
        //        }

        //        // Animations
        //        //-------------------------------------------------------------------------

        //        for ( auto const& animation : m_assetInfo.m_animations )
        //        {
        //            ImGui::TableNextRow();
        //            ImGui::PushID( &animation );

        //            // Type
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGui::AlignTextToFramePadding();
        //            ImGui::Text( "Animation" );

        //            // Name
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            ImGuiX::SelectableText( animation.m_name, -1 );

        //            // Options
        //            //-------------------------------------------------------------------------

        //            ImGui::TableNextColumn();
        //            DrawAnimationControls( animation );

        //            ImGui::PopID();
        //        }

        //        ImGui::EndTable();
        //    }
        //    ImGui::PopStyleVar();
        //}
    }

    void RawFileInspectorGLTF::DrawResourceDescriptorCreator()
    {
        ImGui::Text( "TODO" );
    }

    //void ResourceInspectorGLTF::DrawStaticMeshControls( RawAssets::RawMeshInfo const& mesh )
    //{
    //    if ( ImGui::Button( EE_ICON_PLUS "##CreateStaticMesh", ImVec2( 19, 0 ) ) )
    //    {
    //        Render::StaticMeshResourceDescriptor resourceDesc;
    //        resourceDesc.m_meshPath = ResourcePath::FromFileSystemPath( m_pModel->GetSourceResourceDirectory(), m_filePath );
    //        resourceDesc.m_meshName = mesh.m_name;
    //        CreateNewDescriptor( Render::StaticMesh::GetStaticResourceTypeID(), resourceDesc );
    //    }

    //    ImGuiX::ItemTooltip( "Create static mesh resource for this sub-mesh" );
    //}

    //void ResourceInspectorGLTF::DrawSkeletalMeshControls( RawAssets::RawMeshInfo const& mesh )
    //{
    //    EE_ASSERT( mesh.m_isSkinned );

    //    if ( ImGui::Button( EE_ICON_PLUS "##CreateSkeletalMesh", ImVec2( 20, 0 ) ) )
    //    {
    //        Render::SkeletalMeshResourceDescriptor resourceDesc;
    //        resourceDesc.m_meshPath = ResourcePath::FromFileSystemPath( m_pModel->GetSourceResourceDirectory(), m_filePath );
    //        resourceDesc.m_meshName = mesh.m_name;
    //        CreateNewDescriptor( Render::SkeletalMesh::GetStaticResourceTypeID(), resourceDesc );
    //    }

    //    ImGuiX::ItemTooltip( "Create skeletal mesh resource for this sub-mesh" );
    //}

    //void ResourceInspectorGLTF::DrawSkeletonControls( RawAssets::RawSkeletonInfo const& skeleton )
    //{
    //    if ( ImGui::Button( EE_ICON_PLUS "##CreateSkeleton", ImVec2( 19, 0 ) ) )
    //    {
    //        Animation::SkeletonResourceDescriptor resourceDesc;
    //        resourceDesc.m_skeletonPath = ResourcePath::FromFileSystemPath( m_pModel->GetSourceResourceDirectory(), m_filePath );
    //        resourceDesc.m_skeletonRootBoneName = skeleton.m_name;
    //        CreateNewDescriptor( Animation::Skeleton::GetStaticResourceTypeID(), resourceDesc );
    //    }

    //    ImGuiX::ItemTooltip( "Create skeleton resource" );
    //}

    //void ResourceInspectorGLTF::DrawAnimationControls( RawAssets::RawAnimationInfo const& animation )
    //{
    //    if ( ImGui::Button( EE_ICON_PLUS "##CreateAnimation", ImVec2( 19, 0 ) ) )
    //    {
    //        Animation::AnimationClipResourceDescriptor resourceDesc;
    //        resourceDesc.m_animationPath = ResourcePath::FromFileSystemPath( m_pModel->GetSourceResourceDirectory(), m_filePath );
    //        resourceDesc.m_animationName = animation.m_name;
    //        CreateNewDescriptor( Animation::AnimationClip::GetStaticResourceTypeID(), resourceDesc );
    //    }

    //    ImGuiX::ItemTooltip( "Create animation resource" );
    //}

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
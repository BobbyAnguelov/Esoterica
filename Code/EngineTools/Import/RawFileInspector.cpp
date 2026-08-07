#include "RawFileInspector.h"
#include "Formats/FBX.h"
#include "Formats/GLTF.h"
#include "EngineTools/ThirdParty/ufbx/ufbx.h"
#include "Base/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    static bool InspectFBX( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        DataPath const resourcePath = DataPath( sourceFilePath, ctx.m_sourceDataDirectoryPath );

        //-------------------------------------------------------------------------

        UFbx::SceneContext sceneContext( sourceFilePath );

        if ( !sceneContext.IsValid() )
        {
            ctx.LogError( sceneContext.GetErrorMessage().c_str() );
            return false;
        }

        ufbx_scene* pScene = sceneContext.GetScene();

        if ( !sceneContext.IsBoneOrSkinImportAllowed() )
        {
            ctx.LogWarning( "The source file has an unsupported coordinate system so we dont allow importing animation or skinned meshes from it" );
        }

        //-------------------------------------------------------------------------
        // Meshes
        //-------------------------------------------------------------------------

        for ( ufbx_mesh* pMesh : pScene->meshes )
        {
            for ( ufbx_node* pMeshNode : pMesh->instances )
            {
                for ( size_t partIdx = 0; partIdx < pMesh->material_parts.count; ++partIdx )
                {
                    auto pImportableItem = EE::New<ImportableMesh>();
                    pImportableItem->m_sourceFile = resourcePath;
                    pImportableItem->m_nameID = StringID( pMeshNode->name.data );
                    pImportableItem->m_materialID = StringID( pMeshNode->materials[partIdx]->name.data );
                    pImportableItem->m_isSkeletalMesh = pMesh->skin_deformers.count > 0;
                    outFileInfo.emplace_back( pImportableItem );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Skeletons
        //-------------------------------------------------------------------------

        TVector<ufbx_node*> skeletonRootNodes;
        UFbx::FindAllRootNodes( pScene->root_node, ufbx_element_type::UFBX_ELEMENT_BONE, skeletonRootNodes );

        TVector<ImportableSkeleton*> foundSkeletons;
        for ( auto& pSkeletonNode : skeletonRootNodes )
        {
            StringID const skeletonID = StringID( pSkeletonNode->name.data );

            ufbx_node* pParentNode = pSkeletonNode->parent;
            bool const hasNullOrLocatorParent = ( pParentNode != nullptr ) ? ( pParentNode->attrib_type == ufbx_element_type::UFBX_ELEMENT_EMPTY ) : false;
            if ( hasNullOrLocatorParent )
            {
                StringID const parentRootID( pParentNode->name.data );

                // Try to find existing null root
                ImportableSkeleton* pExistingSkeleton = nullptr;
                for ( auto& pSkeleton : foundSkeletons )
                {
                    if ( pSkeleton->m_nameID == parentRootID )
                    {
                        EE_ASSERT( pSkeleton->IsNullOrLocatorNode() );
                        pExistingSkeleton = pSkeleton;
                        break;
                    }
                }

                // Add root if it doesnt exist
                if ( pExistingSkeleton == nullptr )
                {
                    auto pImportableItem = EE::New<ImportableSkeleton>();
                    pImportableItem->m_sourceFile = resourcePath;
                    pImportableItem->m_nameID = skeletonID;
                    outFileInfo.emplace_back( pImportableItem );

                    pExistingSkeleton = foundSkeletons.emplace_back( pImportableItem );
                }

                pExistingSkeleton->m_childSkeletonRoots.emplace_back( skeletonID );
            }
            else // No parent so just add it
            {
                auto pImportableItem = EE::New<ImportableSkeleton>();
                pImportableItem->m_sourceFile = resourcePath;
                pImportableItem->m_nameID = skeletonID;
                outFileInfo.emplace_back( pImportableItem );
            }
        }

        //-------------------------------------------------------------------------
        // Animations
        //-------------------------------------------------------------------------

        for ( ufbx_anim_stack *pStack : pScene->anim_stacks )
        {
            auto pImportableItem = EE::New<ImportableAnimation>();
            pImportableItem->m_sourceFile = resourcePath;
            pImportableItem->m_nameID = StringID( pStack->name.data );
            pImportableItem->m_duration = Seconds( (float) ( pStack->time_end - pStack->time_begin ) );

            outFileInfo.emplace_back( pImportableItem );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    static bool InspectGLTF( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        DataPath const resourcePath = DataPath( sourceFilePath, ctx.m_sourceDataDirectoryPath );

        //-------------------------------------------------------------------------

        gltf::SceneContext sceneContext( sourceFilePath );

        if ( !sceneContext.IsValid() )
        {
            ctx.LogError( sceneContext.GetErrorMessage().c_str() );
            return false;
        }

        cgltf_data const* pSceneData = sceneContext.GetScene();

        //-------------------------------------------------------------------------
        // Meshes
        //-------------------------------------------------------------------------

        for ( auto m = 0; m < pSceneData->meshes_count; m++ )
        {
            cgltf_mesh const& mesh = pSceneData->meshes[m];

            for ( auto p = 0; p < mesh.primitives_count; p++ )
            {
                cgltf_primitive const& primitive = mesh.primitives[p];
                cgltf_material const* pMaterial = primitive.material;

                auto pImportableItem = EE::New<ImportableMesh>();
                pImportableItem->m_sourceFile = resourcePath;
                pImportableItem->m_nameID = StringID( mesh.name );
                pImportableItem->m_isSkeletalMesh = pSceneData->meshes[m].weights_count > 0;

                if ( pMaterial == nullptr )
                {
                    pImportableItem->m_extraInfo = "No material assigned.";
                }
                else
                {
                    pImportableItem->m_materialID = StringID( pMaterial->name );
                }

                outFileInfo.emplace_back( pImportableItem );
            }
        }

        //-------------------------------------------------------------------------
        // Skeletons
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < pSceneData->skins_count; i++ )
        {
            auto pImportableItem = EE::New<ImportableSkeleton>();
            pImportableItem->m_sourceFile = resourcePath;
            pImportableItem->m_nameID = StringID( pSceneData->skins[i].joints[0]->name );
            outFileInfo.emplace_back( pImportableItem );
        }

        //-------------------------------------------------------------------------
        // Animations
        //-------------------------------------------------------------------------

        for ( auto i = 0; i < pSceneData->animations_count; i++ )
        {
            auto pImportableItem = EE::New<ImportableAnimation>();
            pImportableItem->m_sourceFile = resourcePath;
            pImportableItem->m_nameID = StringID( pSceneData->animations[i].name );

            float animationDuration = -1.0f;
            for ( auto s = 0; s < pSceneData->animations[i].samplers_count; s++ )
            {
                cgltf_accessor const* pInputAccessor = pSceneData->animations[i].samplers[s].input;
                EE_ASSERT( pInputAccessor->has_max );
                animationDuration = Math::Max( pInputAccessor->max[0], animationDuration );
            }

            pImportableItem->m_duration = animationDuration;

            outFileInfo.emplace_back( pImportableItem );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    static bool InspectImage( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        ImportableImage* pImg = EE::New<ImportableImage>();
        pImg->m_sourceFile = DataPath( sourceFilePath, ctx.m_sourceDataDirectoryPath );
        pImg->m_nameID = StringID( sourceFilePath.GetFilenameWithoutExtension() );

        if ( stbi_info( sourceFilePath.c_str(), &pImg->m_dimensions.m_x, &pImg->m_dimensions.m_y, &pImg->m_numChannels ) )
        {
            outFileInfo.emplace_back( pImg );
            return false;
        }
        else
        {
            ctx.LogError( "Failed to read file: %s", sourceFilePath.c_str() );
            EE::Delete( pImg );
            return false;
        }
    }

    //-------------------------------------------------------------------------

    static char const* const g_importableExtensions[] =
    {
        "fbx",  // 0

        "gltf", // 1
        "glb",

        "png",  // 3
        "jpg",
        "jpeg",
        "tga",
        "bmp",
        "tiff", // 8
    };

    InspectionResult InspectFile( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        EE_ASSERT( ctx.IsValid() && sourceFilePath.IsValid() );
        EE_ASSERT( outFileInfo.empty() );

        FileSystem::Extension extension = sourceFilePath.GetExtensionAsString();
        extension.make_lower();

        //-------------------------------------------------------------------------

        int32_t const numValidExtensions = sizeof( g_importableExtensions ) / sizeof( g_importableExtensions[0] );
        int32_t matchingExtensionIdx = InvalidIndex;

        for ( int32_t i = 0; i < numValidExtensions; i++ )
        {
            if ( extension == g_importableExtensions[i] )
            {
                matchingExtensionIdx = i;
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( matchingExtensionIdx == 0 )
        {
            return InspectFBX( ctx, sourceFilePath, outFileInfo ) ? InspectionResult::Success : InspectionResult::Failure;
        }
        else if ( matchingExtensionIdx >= 1 && matchingExtensionIdx <= 2 )
        {
            return InspectGLTF( ctx, sourceFilePath, outFileInfo ) ? InspectionResult::Success : InspectionResult::Failure;
        }
        else if ( matchingExtensionIdx >= 3 && matchingExtensionIdx <= 7 )
        {
            return InspectImage( ctx, sourceFilePath, outFileInfo ) ? InspectionResult::Success : InspectionResult::Failure;
        }

        //-------------------------------------------------------------------------

        return InspectionResult::UnsupportedExtension;
    }

    bool IsImportableFileType( FileSystem::Extension const& extension )
    {
        FileSystem::Extension lowerCaseExtension = extension;
        lowerCaseExtension.make_lower();

        int32_t const numValidExtensions = sizeof( g_importableExtensions ) / sizeof( g_importableExtensions[0] );

        for ( int32_t i = 0; i < numValidExtensions; i++ )
        {
            if ( lowerCaseExtension == g_importableExtensions[i] )
            {
                return true;
            }
        }

        return false;
    }
}
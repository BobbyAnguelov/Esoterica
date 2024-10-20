#include "RawFileInspector.h"
#include "Formats/FBX.h"
#include "Formats/GLTF.h"
#include "Base/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    static bool InspectFBX( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        DataPath const resourcePath = DataPath::FromFileSystemPath( ctx.m_sourceDataDirectoryPath, sourceFilePath );

        //-------------------------------------------------------------------------

        Fbx::SceneContext sceneContext;
        sceneContext.LoadFile( sourceFilePath );

        if ( !sceneContext.IsValid() )
        {
            ctx.LogError( sceneContext.GetErrorMessage().c_str() );
            return false;
        }

        //-------------------------------------------------------------------------
        // Meshes
        //-------------------------------------------------------------------------

        TInlineVector<FbxMesh*, 20> meshes;
        int32_t numGeometries = sceneContext.m_pScene->GetGeometryCount();
        for ( int32_t i = 0; i < numGeometries; i++ )
        {
            FbxGeometry* pGeometry = sceneContext.m_pScene->GetGeometry( i );
            if ( pGeometry->Is<FbxMesh>() )
            {
                FbxMesh* pMesh = reinterpret_cast<FbxMesh*>( pGeometry );

                auto pImportableItem = EE::New<ImportableMesh>();
                pImportableItem->m_sourceFile = resourcePath;
                pImportableItem->m_nameID = StringID( Import::Fbx::GetNameWithoutNamespace( pMesh->GetNode() ) );
                pImportableItem->m_isSkeletalMesh = pMesh->GetDeformerCount( FbxDeformer::eSkin ) > 0;

                // Get material name
                int32_t const numMaterials = pMesh->GetElementMaterialCount();
                if ( numMaterials == 1 )
                {
                    FbxGeometryElementMaterial* pMaterialElement = pMesh->GetElementMaterial( 0 );
                    FbxSurfaceMaterial* pMaterial = pMesh->GetNode()->GetMaterial( pMaterialElement->GetIndexArray().GetAt( 0 ) );
                    pImportableItem->m_materialID = StringID( pMaterial->GetName() );
                }
                else if ( numMaterials > 1 )
                {
                    pImportableItem->m_extraInfo = "More than one material detected - This is not supported.";
                }
                else
                {
                    pImportableItem->m_extraInfo = "No material assigned.";
                }

                outFileInfo.emplace_back( pImportableItem );
            }
        }

        //-------------------------------------------------------------------------
        // Skeletons
        //-------------------------------------------------------------------------

        TVector<ImportableSkeleton*> foundSkeletons;
        TVector<FbxNode*> skeletonRootNodes;
        sceneContext.FindAllNodesOfType( FbxNodeAttribute::eSkeleton, skeletonRootNodes );

        for ( auto& pSkeletonNode : skeletonRootNodes )
        {
            bool hasNullOrLocatorParent = false;
            auto pParentNode = pSkeletonNode->GetParent();
            if ( pParentNode != nullptr )
            {
                if ( auto pNodeAttribute = pParentNode->GetNodeAttribute() )
                {
                    hasNullOrLocatorParent = pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull;
                }
            }

            //-------------------------------------------------------------------------

            StringID const skeletonID = StringID( pSkeletonNode->GetName() );

            if ( hasNullOrLocatorParent )
            {
                StringID const parentRootID( pParentNode->GetName() );

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

        TVector<FbxAnimStack*> stacks;
        sceneContext.FindAllAnimStacks( stacks );
        for ( auto pAnimStack : stacks )
        {
            auto pTakeInfo = sceneContext.m_pScene->GetTakeInfo( Import::Fbx::GetNameWithoutNamespace( pAnimStack ) );
            if ( pTakeInfo != nullptr )
            {
                auto pImportableItem = EE::New<ImportableAnimation>();
                pImportableItem->m_sourceFile = resourcePath;
                pImportableItem->m_nameID = StringID( pAnimStack->GetName() );
                pImportableItem->m_duration = Seconds( (float) pTakeInfo->mLocalTimeSpan.GetDuration().GetSecondDouble() );

                FbxTime const duration = pTakeInfo->mLocalTimeSpan.GetDuration();
                FbxTime::EMode mode = duration.GetGlobalTimeMode();
                pImportableItem->m_frameRate = (float) duration.GetFrameRate( mode );

                outFileInfo.emplace_back( pImportableItem );
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    static bool InspectGLTF( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        DataPath const resourcePath = DataPath::FromFileSystemPath( ctx.m_sourceDataDirectoryPath, sourceFilePath );

        //-------------------------------------------------------------------------

        gltf::SceneContext sceneContext;
        sceneContext.LoadFile( sourceFilePath );

        if ( !sceneContext.IsValid() )
        {
            ctx.LogError( sceneContext.GetErrorMessage().c_str() );
            return false;
        }

        cgltf_data const* pSceneData = sceneContext.GetSceneData();

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
            size_t numFrames = 0;
            for ( auto s = 0; s < pSceneData->animations[i].samplers_count; s++ )
            {
                cgltf_accessor const* pInputAccessor = pSceneData->animations[i].samplers[s].input;
                EE_ASSERT( pInputAccessor->has_max );
                animationDuration = Math::Max( pInputAccessor->max[0], animationDuration );
                numFrames = Math::Max( pInputAccessor->count, numFrames );
            }

            pImportableItem->m_duration = animationDuration;
            pImportableItem->m_frameRate = animationDuration / numFrames;

            outFileInfo.emplace_back( pImportableItem );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    static bool InspectImage( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo )
    {
        ImportableImage* pImg = EE::New<ImportableImage>();
        pImg->m_sourceFile = DataPath::FromFileSystemPath( ctx.m_sourceDataDirectoryPath, sourceFilePath );
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

        "png",  // 2
        "jpg",
        "jpeg",
        "tga",
        "bmp",
        "tiff", //7
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
        else if ( matchingExtensionIdx == 1 )
        {
            return InspectGLTF( ctx, sourceFilePath, outFileInfo ) ? InspectionResult::Success : InspectionResult::Failure;
        }
        else if ( matchingExtensionIdx >= 2 && matchingExtensionIdx <= 7 )
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
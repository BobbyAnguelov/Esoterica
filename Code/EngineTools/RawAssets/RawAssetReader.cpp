#include "RawAssetReader.h"
#include "Fbx/FbxSkeleton.h"
#include "Fbx/FbxAnimation.h"
#include "Fbx/FbxMesh.h"
#include "gltf/gltfMesh.h"
#include "gltf/gltfSkeleton.h"
#include "gltf/gltfAnimation.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    static bool ValidateRawAsset( ReaderContext const& ctx, RawAssets::RawAsset const* pRawAsset )
    {
        if ( pRawAsset )
        {
            for ( auto const& warning : pRawAsset->GetWarnings() )
            {
                ctx.m_warningDelegate( warning.c_str() );
            }

            for ( auto const& error : pRawAsset->GetErrors() )
            {
                ctx.m_errorDelegate( error.c_str() );
            }

            return !pRawAsset->HasErrors() && pRawAsset->IsValid();
        }

        return false;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<RawAssets::RawMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& nameOfMeshToCompile )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<RawAssets::RawMesh> pRawMesh = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pRawMesh = Fbx::ReadStaticMesh( sourceFilePath, nameOfMeshToCompile );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pRawMesh = gltf::ReadStaticMesh( sourceFilePath, nameOfMeshToCompile );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pRawMesh.get() ) )
        {
            pRawMesh = nullptr;
        }

        //-------------------------------------------------------------------------

        return pRawMesh;
    }

    TUniquePtr<RawAssets::RawMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, int32_t maxBoneInfluences )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<RawAssets::RawMesh> pRawMesh = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pRawMesh = Fbx::ReadSkeletalMesh( sourceFilePath, maxBoneInfluences );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pRawMesh = gltf::ReadSkeletalMesh( sourceFilePath, maxBoneInfluences );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pRawMesh.get() ) )
        {
            pRawMesh = nullptr;
        }

        //-------------------------------------------------------------------------

        return pRawMesh;
    }

    TUniquePtr<RawAssets::RawSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<RawAssets::RawSkeleton> pRawSkeleton = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pRawSkeleton = Fbx::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pRawSkeleton = gltf::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pRawSkeleton.get() ) )
        {
            pRawSkeleton = nullptr;
        }

        //-------------------------------------------------------------------------

        return pRawSkeleton;
    }

    TUniquePtr<RawAssets::RawAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, RawAssets::RawSkeleton const& rawSkeleton, String const& animationName )
    {
        EE_ASSERT( ctx.IsValid() && sourceFilePath.IsValid() && rawSkeleton.IsValid() );

        TUniquePtr<RawAssets::RawAnimation> pRawAnimation = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pRawAnimation = Fbx::ReadAnimation( sourceFilePath, rawSkeleton, animationName );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pRawAnimation = gltf::ReadAnimation( sourceFilePath, rawSkeleton, animationName );
        }
        else
        {
            TInlineString<512> errorString;
            errorString.sprintf( "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( errorString.c_str() );
        }

        //-------------------------------------------------------------------------

        pRawAnimation->Finalize();

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pRawAnimation.get() ) )
        {
            pRawAnimation = nullptr;
        }

        //-------------------------------------------------------------------------

        return pRawAnimation;
    }
}
#include "RawAssetReader.h"
#include "RawMesh.h"
#include "RawSkeleton.h"
#include "RawAnimation.h"
#include "Formats/FBX.h"
#include "Formats/GLTF.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    static bool ValidateRawAsset( ReaderContext const& ctx, RawAsset const* pRawAsset )
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

    TUniquePtr<RawMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<RawMesh> pRawMesh = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pRawMesh = Fbx::ReadStaticMesh( sourceFilePath, meshesToInclude );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pRawMesh = gltf::ReadStaticMesh( sourceFilePath, meshesToInclude );
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

    TUniquePtr<RawMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<RawMesh> pRawMesh = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pRawMesh = Fbx::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pRawMesh = gltf::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
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

    TUniquePtr<RawSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName, TVector<StringID> const& listOfHighLODBones )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<RawSkeleton> pRawSkeleton = nullptr;

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

        pRawSkeleton->Finalize( listOfHighLODBones );

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pRawSkeleton.get() ) )
        {
            pRawSkeleton = nullptr;
        }

        //-------------------------------------------------------------------------

        return pRawSkeleton;
    }

    TUniquePtr<RawAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, RawSkeleton const& rawSkeleton, String const& animationName, StringID const& rootMotionBoneID )
    {
        EE_ASSERT( ctx.IsValid() && sourceFilePath.IsValid() && rawSkeleton.IsValid() );

        TUniquePtr<RawAnimation> pRawAnimation = nullptr;

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

        if ( !pRawAnimation->HasErrors() )
        {
            pRawAnimation->Finalize( rootMotionBoneID );
        }

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pRawAnimation.get() ) )
        {
            pRawAnimation = nullptr;
        }

        //-------------------------------------------------------------------------

        return pRawAnimation;
    }
}
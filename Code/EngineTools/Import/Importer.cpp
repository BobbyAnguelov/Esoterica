#include "Importer.h"
#include "ImportedMesh.h"
#include "ImportedSkeleton.h"
#include "ImportedAnimation.h"
#include "ImportedImage.h"
#include "Formats/FBX.h"
#include "Formats/GLTF.h"
#include "Base/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    static bool ValidateRawAsset( ReaderContext const& ctx, ImportedData const* pRawAsset )
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

    TUniquePtr<ImportedMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<ImportedMesh> pImportedMesh = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pImportedMesh = Fbx::ReadStaticMesh( sourceFilePath, meshesToInclude );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedMesh = gltf::ReadStaticMesh( sourceFilePath, meshesToInclude );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pImportedMesh.get() ) )
        {
            pImportedMesh = nullptr;
        }

        //-------------------------------------------------------------------------

        return pImportedMesh;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<ImportedMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<ImportedMesh> pImportedMesh = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pImportedMesh = Fbx::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedMesh = gltf::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pImportedMesh.get() ) )
        {
            pImportedMesh = nullptr;
        }

        //-------------------------------------------------------------------------

        return pImportedMesh;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<ImportedSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName, TVector<StringID> const& listOfHighLODBones )
    {
        EE_ASSERT( sourceFilePath.IsValid() && ctx.IsValid() );

        TUniquePtr<ImportedSkeleton> pImportedSkeleton = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pImportedSkeleton = Fbx::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedSkeleton = gltf::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        pImportedSkeleton->Finalize( listOfHighLODBones );

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pImportedSkeleton.get() ) )
        {
            pImportedSkeleton = nullptr;
        }

        //-------------------------------------------------------------------------

        return pImportedSkeleton;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<ImportedAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, ImportedSkeleton const& importedSkeleton, String const& animationName )
    {
        EE_ASSERT( ctx.IsValid() && sourceFilePath.IsValid() && importedSkeleton.IsValid() );

        TUniquePtr<ImportedAnimation> pImportedAnimation = nullptr;

        auto const extension = sourceFilePath.GetLowercaseExtensionAsString();
        if ( extension == "fbx" )
        {
            pImportedAnimation = Fbx::ReadAnimation( sourceFilePath, importedSkeleton, animationName );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedAnimation = gltf::ReadAnimation( sourceFilePath, importedSkeleton, animationName );
        }
        else
        {
            TInlineString<512> errorString;
            errorString.sprintf( "unsupported extension: %s", sourceFilePath.c_str() );
            ctx.m_errorDelegate( errorString.c_str() );
        }

        //-------------------------------------------------------------------------

        if ( pImportedAnimation != nullptr )
        {
            if ( !pImportedAnimation->HasErrors() )
            {
                pImportedAnimation->Finalize();
            }

            //-------------------------------------------------------------------------

            if ( !ValidateRawAsset( ctx, pImportedAnimation.get() ) )
            {
                pImportedAnimation = nullptr;
            }
        }

        //-------------------------------------------------------------------------

        return pImportedAnimation;
    }

    //-------------------------------------------------------------------------

    class StbImportedImage : public ImportedImage
    {
        friend TUniquePtr<ImportedImage> ReadImage( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath );

    public:

        using ImportedImage::ImportedImage;
    };

    TUniquePtr<ImportedImage> ReadImage( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath )
    {
        EE_ASSERT( ctx.IsValid() && sourceFilePath.IsValid() );

        TUniquePtr<ImportedImage> pImportedImage( EE::New<StbImportedImage>() );
        StbImportedImage* pImage = (StbImportedImage*) pImportedImage.get();

        //-------------------------------------------------------------------------

        pImage->m_pImageData = stbi_load( sourceFilePath.c_str(), &pImage->m_width, &pImage->m_height, &pImage->m_channels, STBI_rgb_alpha );
        if ( pImage->m_pImageData == nullptr )
        {
            pImportedImage = nullptr;
        }
        else
        {
            pImage->m_stride = pImage->m_width * STBI_rgb_alpha;
        }

        //-------------------------------------------------------------------------

        return pImportedImage;
    }
}
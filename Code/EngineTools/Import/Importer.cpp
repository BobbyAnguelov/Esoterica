#include "Importer.h"
#include "ImportedMesh.h"
#include "ImportedSkeleton.h"
#include "ImportedAnimation.h"
#include "ImportedImage.h"
#include "Formats/FBX.h"
#include "Formats/GLTF.h"
#include "Formats/DDS.h"
#include "EngineTools/ThirdParty/tinyexr/tinyexr.h"
#include "Base/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    static bool ValidateRawAsset( ReaderContext const& ctx, ImportedData const* pRawAsset )
    {
        if ( pRawAsset )
        {
            for ( auto const& entry : pRawAsset->GetLogEntries() )
            {
                if ( entry.m_severity == Severity::Error )
                {
                    ctx.m_errorDelegate( entry.m_message.c_str() );
                }
                else
                {
                    ctx.m_warningDelegate( entry.m_message.c_str() );
                }
            }

            return !pRawAsset->HasErrors() && pRawAsset->IsValid();
        }

        return false;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Mesh> Importer::ReadStaticMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude )
    {
        EE_ASSERT( source.IsValid() && ctx.IsValid() );

        TUniquePtr<Mesh> pImportedMesh = nullptr;

        auto const extension = source.GetExtension();
        if ( extension == "fbx" )
        {
            pImportedMesh = UFbx::ReadStaticMesh( source, meshesToInclude );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedMesh = gltf::ReadStaticMesh( source, meshesToInclude );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", source.m_path.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        pImportedMesh->Finalize();

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pImportedMesh.get() ) )
        {
            pImportedMesh = nullptr;
        }

        //-------------------------------------------------------------------------

        return pImportedMesh;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Mesh> Importer::ReadSkeletalMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude )
    {
        EE_ASSERT( source.IsValid() && ctx.IsValid() );

        TUniquePtr<Mesh> pImportedMesh = nullptr;

        auto const extension = source.GetExtension();
        if ( extension == "fbx" )
        {
            pImportedMesh = UFbx::ReadSkeletalMesh( source, meshesToInclude );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedMesh = gltf::ReadSkeletalMesh( source, meshesToInclude );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", source.m_path.c_str() );
            ctx.m_errorDelegate( buffer );
        }

        pImportedMesh->Finalize();

        //-------------------------------------------------------------------------

        if ( !ValidateRawAsset( ctx, pImportedMesh.get() ) )
        {
            pImportedMesh = nullptr;
        }

        //-------------------------------------------------------------------------

        return pImportedMesh;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Skeleton> Importer::ReadSkeleton( ReaderContext const& ctx, Source const& source, String const& skeletonRootBoneName, TVector<StringID> const& listOfHighLODBones )
    {
        EE_ASSERT( source.IsValid() && ctx.IsValid() );

        TUniquePtr<Skeleton> pImportedSkeleton = nullptr;

        auto const extension = source.GetExtension();
        if ( extension == "fbx" )
        {
            pImportedSkeleton = UFbx::ReadSkeleton( source, skeletonRootBoneName );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedSkeleton = gltf::ReadSkeleton( source, skeletonRootBoneName );
        }
        else
        {
            char buffer[512];
            Printf( buffer, 512, "unsupported extension: %s", source.m_path.c_str() );
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

    TUniquePtr<Animation> Importer::ReadAnimation( ReaderContext const& ctx, Source const& source, Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate )
    {
        EE_ASSERT( pPrimarySkeleton != nullptr && pPrimarySkeleton->IsValid() );
        EE_ASSERT( ctx.IsValid() && source.IsValid() );

        for ( auto pSecondarySkeleton : secondarySkeletons )
        {
            EE_ASSERT( pSecondarySkeleton != nullptr && pSecondarySkeleton->IsValid() );
        }

        TUniquePtr<Animation> pImportedAnimation = nullptr;

        auto const extension = source.GetExtension();
        if ( extension == "fbx" )
        {
            pImportedAnimation = UFbx::ReadAnimation( source, pPrimarySkeleton, secondarySkeletons, animationName, samplingFrameRate );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            pImportedAnimation = gltf::ReadAnimation( source, pPrimarySkeleton, secondarySkeletons, animationName );
        }
        else
        {
            TInlineString<512> errorString;
            errorString.sprintf( "unsupported extension: %s", source.m_path.c_str() );
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

    TUniquePtr<Image> Importer::ReadImage( ReaderContext const& ctx, Source const& source )
    {
        EE_ASSERT( ctx.IsValid() && source.IsValid() );

        // HDR
        //-------------------------------------------------------------------------
        // TODO: Load HDR directly in half-float format instead of float.

        FileSystem::Extension const extension = source.GetExtension();
        if ( extension == "hdr" )
        {
            TUniquePtr<Image> importedImage( EE::New<Image>() );

            float* pImportedHDR = nullptr;
            if( source.m_pFileData != nullptr )
            {
                pImportedHDR = stbi_loadf_from_memory( source.m_pFileData->data(), (int32_t) source.m_pFileData->size(), &importedImage->m_width, &importedImage->m_height, &importedImage->m_channels, STBI_rgb_alpha );
            }
            else
            {
                pImportedHDR = stbi_loadf( source.m_path.c_str(), &importedImage->m_width, &importedImage->m_height, &importedImage->m_channels, STBI_rgb_alpha );
            }

            importedImage->m_pImageData = reinterpret_cast<uint8_t*>( pImportedHDR );
            importedImage->m_hdr = true;
            importedImage->m_stride = int32_t( importedImage->m_width * STBI_rgb_alpha * sizeof( float ) );

            return importedImage;
        }

        // EXR
        //-------------------------------------------------------------------------
        // TODO: Load EXR directly in half-float format instead of float.

        else if ( extension == "exr" )
        {
            TUniquePtr<Image> importedImage( EE::New<Image>() );

            float* pImportedHDR = nullptr;
            char const* pErrorMessage = nullptr;

            if ( source.m_pFileData != nullptr )
            {
                if ( LoadEXRFromMemory( &pImportedHDR, &importedImage->m_width, &importedImage->m_height, source.m_pFileData->data(), source.m_pFileData->size(), &pErrorMessage ) != TINYEXR_SUCCESS )
                {
                    String errorMessage;
                    errorMessage.sprintf( "Failed to load EXR file %s : %s", source.m_path.c_str(), pErrorMessage );
                    ctx.m_errorDelegate( errorMessage.c_str() );

                    EE::Free( pErrorMessage );
                    return nullptr;
                }
            }
            else
            {
                if ( LoadEXR( &pImportedHDR, &importedImage->m_width, &importedImage->m_height, source.m_path.c_str(), &pErrorMessage ) != TINYEXR_SUCCESS )
                {
                    String errorMessage;
                    errorMessage.sprintf( "Failed to load EXR file %s : %s", source.m_path.c_str(), pErrorMessage );
                    ctx.m_errorDelegate( errorMessage.c_str() );

                    EE::Free( pErrorMessage );
                    return nullptr;
                }
            }

            importedImage->m_pImageData = reinterpret_cast<uint8_t*>( pImportedHDR );
            importedImage->m_hdr = true;
            importedImage->m_stride = int32_t( importedImage->m_width * STBI_rgb_alpha * sizeof( float ) );

            return importedImage;
        }

        // DDS
        //-------------------------------------------------------------------------

        else if ( extension == "dds" )
        {
            TUniquePtr<Image> importedImage;

            if ( source.m_pFileData != nullptr )
            {
                importedImage = DDS::ReadImage( *source.m_pFileData );
            }
            else
            {
                importedImage = DDS::ReadImage( source.m_path );
            }

            return importedImage;
        }

        // Regular Formats
        //-------------------------------------------------------------------------

        else
        {
            TUniquePtr<Image> importedImage( EE::New<Image>() );

            if ( source.m_pFileData != nullptr )
            {
                importedImage->m_pImageData = stbi_load_from_memory( source.m_pFileData->data(), (int32_t) source.m_pFileData->size(), &importedImage->m_width, &importedImage->m_height, &importedImage->m_channels, STBI_rgb_alpha );
            }
            else
            {
                importedImage->m_pImageData = stbi_load( source.m_path.c_str(), &importedImage->m_width, &importedImage->m_height, &importedImage->m_channels, STBI_rgb_alpha );
            }

            importedImage->m_stride = importedImage->m_width * STBI_rgb_alpha;
            importedImage->m_channels = 4;

            return importedImage;
        }

        return nullptr;
    }
}

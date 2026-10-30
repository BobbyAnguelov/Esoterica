#include "Importer.h"
#include "ImportedMesh.h"
#include "ImportedSkeleton.h"
#include "ImportedAnimation.h"
#include "ImportedImage.h"

#include "Formats/FBX.h"
#include "Formats/GLTF.h"
#include "Formats/DDS.h"
#include "Formats/EXR.h"
#include "Formats/HDR.h"
#include "Formats/Bitmaps.h"

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

    static bool ValidateFile( ReaderContext const& ctx, Source const& source, Import::File const* pFile )
    {
        if ( pFile == nullptr )
        {
            InlineString str( InlineString::CtorSprintf(), "Invalid source file extension: %s", source.m_path.c_str() );
            ctx.m_errorDelegate( str.c_str() );
            return false;
        }
        
        if ( !pFile->IsValid() )
        {
            InlineString str( InlineString::CtorSprintf(), "Invalid source file: %s, error: %s", source.m_path.c_str(), pFile->GetErrorMessage().c_str() );
            ctx.m_errorDelegate( str.c_str() );
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<ImageFile> Importer::LoadImageFile( ReaderContext const& ctx, Source const& source )
    {
        TUniquePtr<ImageFile> imageFile;

        FileSystem::Extension const extension = source.GetExtension();

        if ( extension == "hdr" )
        {
            imageFile = TUniquePtr<ImageFile>( EE::New<HDR::File>( source ) );
        }
        else if ( extension == "exr" )
        {
            imageFile = TUniquePtr<ImageFile>( EE::New<EXR::File>( source ) );
        }
        else if ( extension == "dds" )
        {
            imageFile = TUniquePtr<ImageFile>( EE::New<DDS::File>( source ) );
        }
        else
        {
            imageFile = TUniquePtr<ImageFile>( EE::New<BitmapImage::File>( source ) );
        }

        return imageFile;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<SceneFile> Importer::LoadSceneFile( ReaderContext const& ctx, Source const& source )
    {
        TUniquePtr<SceneFile> sceneFile;

        FileSystem::Extension const extension = source.GetExtension();

        if ( extension == "fbx" )
        {
            sceneFile = TUniquePtr<SceneFile>( EE::New<UFbx::File>( source, ctx.m_sourceDataDirectoryPath ) );
        }
        else if ( extension == "gltf" || extension == "glb" )
        {
            sceneFile = TUniquePtr<SceneFile>( EE::New<gltf::File>( source, ctx.m_sourceDataDirectoryPath ) );
        }

        return sceneFile;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Mesh> Importer::ReadStaticMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude, bool finalizeMaterials )
    {
        EE_ASSERT( source.IsValid() && ctx.IsValid() );

        TUniquePtr<SceneFile> const sceneFile = LoadSceneFile( ctx, source );
        if ( !ValidateFile( ctx, source, sceneFile.get() ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TUniquePtr<Mesh> pImportedMesh = sceneFile->ReadStaticMesh( meshesToInclude );
        if ( pImportedMesh != nullptr )
        {
            pImportedMesh->Finalize();

            if ( finalizeMaterials )
            {
                pImportedMesh->FinalizeMaterials( ctx.m_sourceDataDirectoryPath );
            }

            if ( !ValidateRawAsset( ctx, pImportedMesh.get() ) )
            {
                pImportedMesh = nullptr;
            }
        }

        return pImportedMesh;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Mesh> Importer::ReadSkeletalMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude, bool finalizeMaterials )
    {
        EE_ASSERT( source.IsValid() && ctx.IsValid() );

        TUniquePtr<SceneFile> const sceneFile = LoadSceneFile( ctx, source );
        if ( !ValidateFile( ctx, source, sceneFile.get() ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TUniquePtr<Mesh> pImportedMesh = sceneFile->ReadSkeletalMesh( meshesToInclude );
        if ( pImportedMesh != nullptr )
        {
            pImportedMesh->Finalize();

            if ( finalizeMaterials )
            {
                pImportedMesh->FinalizeMaterials( ctx.m_sourceDataDirectoryPath );
            }

            if ( !ValidateRawAsset( ctx, pImportedMesh.get() ) )
            {
                pImportedMesh = nullptr;
            }
        }

        return pImportedMesh;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Skeleton> Importer::ReadSkeleton( ReaderContext const& ctx, Source const& source, String const& skeletonRootBoneName, TVector<StringID> const& listOfHighLODBones )
    {
        EE_ASSERT( source.IsValid() && ctx.IsValid() );

        TUniquePtr<SceneFile> const sceneFile = LoadSceneFile( ctx, source );
        if ( !ValidateFile( ctx, source, sceneFile.get() ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TUniquePtr<Skeleton> pImportedSkeleton = sceneFile->ReadSkeleton( skeletonRootBoneName );
        if ( pImportedSkeleton != nullptr )
        {
            pImportedSkeleton->Finalize( listOfHighLODBones );

            if ( !ValidateRawAsset( ctx, pImportedSkeleton.get() ) )
            {
                pImportedSkeleton = nullptr;
            }
        }

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

        //-------------------------------------------------------------------------

        TUniquePtr<SceneFile> const sceneFile = LoadSceneFile( ctx, source );
        if ( !ValidateFile( ctx, source, sceneFile.get() ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TUniquePtr<Animation> pImportedAnimation = sceneFile->ReadAnimation( pPrimarySkeleton, secondarySkeletons, animationName, samplingFrameRate );
        if ( pImportedAnimation != nullptr )
        {
            if ( !pImportedAnimation->HasErrors() )
            {
                pImportedAnimation->Finalize();
            }

            if ( !ValidateRawAsset( ctx, pImportedAnimation.get() ) )
            {
                pImportedAnimation = nullptr;
            }
        }

        return pImportedAnimation;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Materials> Importer::ReadMaterials( ReaderContext const& ctx, Source const& source, FileSystem::Path const& overrideTextureSearchPath )
    {
        EE_ASSERT( ctx.IsValid() && source.IsValid() );

        TUniquePtr<SceneFile> const sceneFile = LoadSceneFile( ctx, source );
        if ( !ValidateFile( ctx, source, sceneFile.get() ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TUniquePtr<Materials> pImportedMaterials = sceneFile->ReadMaterials();
        if ( pImportedMaterials != nullptr )
        {
            if ( !pImportedMaterials->HasErrors() )
            {
                pImportedMaterials->Finalize( ctx.m_sourceDataDirectoryPath, overrideTextureSearchPath );
            }

            if ( !ValidateRawAsset( ctx, pImportedMaterials.get() ) )
            {
                pImportedMaterials = nullptr;
            }
        }

        return pImportedMaterials;
    }

    //-------------------------------------------------------------------------

    TUniquePtr<Image> Importer::ReadImage( ReaderContext const& ctx, Source const& source )
    {
        EE_ASSERT( ctx.IsValid() && source.IsValid() );

        TUniquePtr<ImageFile> const imageFile = LoadImageFile( ctx, source );
        if ( !ValidateFile( ctx, source, imageFile.get() ) )
        {
            return nullptr;
        }

        //-------------------------------------------------------------------------

        TUniquePtr<Image> importedImage = imageFile->ReadImage();
        if ( !ValidateRawAsset( ctx, importedImage.get() ) )
        {
            importedImage = nullptr;
        }

        return importedImage;
    }
}

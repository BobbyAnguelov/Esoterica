#pragma once

#include "Formats/FormatFile.h"
#include "Base/Types/Function.h"
#include "Base/Types/StringID.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    struct ReaderContext
    {
        inline bool IsValid() const{ return m_warningDelegate != nullptr && m_errorDelegate != nullptr; }

        FileSystem::Path                m_sourceDataDirectoryPath;
        TFunction<void( char const* )>  m_warningDelegate;
        TFunction<void( char const* )>  m_errorDelegate;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API Importer
    {
        static TUniquePtr<ImageFile> LoadImageFile( ReaderContext const& ctx, Source const& source );
        static TUniquePtr<SceneFile> LoadSceneFile( ReaderContext const& ctx, Source const& source );

        static TUniquePtr<Skeleton> ReadSkeleton( ReaderContext const& ctx, Source const& source, String const& skeletonRootBoneName = String(), TVector<StringID> const& listOfHighLODBones = TVector<StringID>() );
        static TUniquePtr<Animation> ReadAnimation( ReaderContext const& ctx, Source const& source, Import::Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName = String(), float samplingFrameRate = 30 );
        static TUniquePtr<Mesh> ReadStaticMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude = TVector<String>(), bool finalizeMaterials = false );
        static TUniquePtr<Mesh> ReadSkeletalMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude = TVector<String>(), bool finalizeMaterials = false );
        static TUniquePtr<Materials> ReadMaterials( ReaderContext const& ctx, Source const& source, FileSystem::Path const& overrideTextureSearchPath = FileSystem::Path() );
        static TUniquePtr<Image> ReadImage( ReaderContext const& ctx, Source const& source );
    };
}
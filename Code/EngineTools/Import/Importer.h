#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Memory/Pointers.h"
#include "Base/Types/Function.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class ImportedMesh;
    class ImportedSkeleton;
    class ImportedAnimation;
    class ImportedImage;

    //-------------------------------------------------------------------------

    struct ReaderContext
    {
        inline bool IsValid() const{ return m_warningDelegate != nullptr && m_errorDelegate != nullptr; }

        TFunction<void( char const* )>  m_warningDelegate;
        TFunction<void( char const* )>  m_errorDelegate;
    };

    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<ImportedSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName = String(), TVector<StringID> const& listOfHighLODBones = TVector<StringID>() );
    EE_ENGINETOOLS_API TUniquePtr<ImportedAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, Import::ImportedSkeleton const& importedSkeleton, String const& animationName = String() );
    EE_ENGINETOOLS_API TUniquePtr<ImportedMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude = TVector<String>() );
    EE_ENGINETOOLS_API TUniquePtr<ImportedMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude = TVector<String>(), int32_t maxBoneInfluences = 4 );
    EE_ENGINETOOLS_API TUniquePtr<ImportedImage> ReadImage( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath );
}
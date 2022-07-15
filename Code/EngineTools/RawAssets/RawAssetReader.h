#pragma once

#include "EngineTools/_Module/API.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Memory/Pointers.h"
#include "System/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class RawMesh;
    class RawSkeleton;
    class RawAnimation;

    //-------------------------------------------------------------------------

    struct ReaderContext
    {
        inline bool IsValid() const{ return m_warningDelegate != nullptr && m_errorDelegate != nullptr; }

        TFunction<void( char const* )>  m_warningDelegate;
        TFunction<void( char const* )>  m_errorDelegate;
    };

    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& nameOfMeshToCompile = String() );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, int32_t maxBoneInfluences = 4 );

    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName = String() );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, RawAssets::RawSkeleton const& rawSkeleton, String const& animationName = String() );
}
#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Memory/Pointers.h"
#include "Base/Types/Function.h"
#include "Base/Types/StringID.h"

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

    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawSkeleton> ReadSkeleton( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName = String(), TVector<StringID> const& listOfHighLODBones = TVector<StringID>() );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawAnimation> ReadAnimation( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, RawAssets::RawSkeleton const& rawSkeleton, String const& animationName = String(), StringID const& rootMotionBoneID = StringID() );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadStaticMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude = TVector<String>() );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadSkeletalMesh( ReaderContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude = TVector<String>(), int32_t maxBoneInfluences = 4 );
}
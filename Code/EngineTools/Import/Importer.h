#pragma once

#include "EngineTools/_Module/API.h"
#include "ImporterSource.h"
#include "Base/Types/Function.h"
#include "Base/Types/StringID.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class Mesh;
    class Skeleton;
    class Animation;
    class Image;

    //-------------------------------------------------------------------------

    struct ReaderContext
    {
        inline bool IsValid() const{ return m_warningDelegate != nullptr && m_errorDelegate != nullptr; }

        TFunction<void( char const* )>  m_warningDelegate;
        TFunction<void( char const* )>  m_errorDelegate;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API Importer
    {
        static TUniquePtr<Skeleton> ReadSkeleton( ReaderContext const& ctx, Source const& source, String const& skeletonRootBoneName = String(), TVector<StringID> const& listOfHighLODBones = TVector<StringID>() );
        static TUniquePtr<Animation> ReadAnimation( ReaderContext const& ctx, Source const& source, Import::Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName = String(), float samplingFrameRate = 30 );
        static TUniquePtr<Mesh> ReadStaticMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude = TVector<String>() );
        static TUniquePtr<Mesh> ReadSkeletalMesh( ReaderContext const& ctx, Source const& source, TVector<String> const& meshesToInclude = TVector<String>() );
        static TUniquePtr<Image> ReadImage( ReaderContext const& ctx, Source const& source );
    };
}
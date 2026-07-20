#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Import/ImporterSource.h"
#include "Base/Types/String.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class Mesh;
    class Animation;
    class Skeleton;
}

//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    //-------------------------------------------------------------------------
    // Import Functions
    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName );
    EE_ENGINETOOLS_API TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate );
    EE_ENGINETOOLS_API TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude );
    EE_ENGINETOOLS_API TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude );
}
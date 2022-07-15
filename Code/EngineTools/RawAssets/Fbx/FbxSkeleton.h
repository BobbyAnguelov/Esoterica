#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/RawAssets/RawSkeleton.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Memory/Pointers.h"

//-------------------------------------------------------------------------

namespace EE::Fbx
{
    class FbxSceneContext;

    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName );

    // Temp HACK
    EE_ENGINETOOLS_API void ReadSkeleton( FbxSceneContext const& sceneCtx, String const& skeletonRootBoneName, RawAssets::RawSkeleton& skeleton );
}
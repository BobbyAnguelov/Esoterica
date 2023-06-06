#pragma once

#include "EngineTools/RawAssets/RawMesh.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Memory/Pointers.h"

//-------------------------------------------------------------------------

namespace EE::Fbx
{
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences );
}
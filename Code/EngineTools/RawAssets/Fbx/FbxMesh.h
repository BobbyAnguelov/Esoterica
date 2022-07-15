#pragma once

#include "EngineTools/RawAssets/RawMesh.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Memory/Pointers.h"

//-------------------------------------------------------------------------

namespace EE::Fbx
{
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, String const& nameOfMeshToCompile = String() );
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, int32_t maxBoneInfluences );
}
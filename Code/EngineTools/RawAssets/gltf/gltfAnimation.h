#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/RawAssets/RawAnimation.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Memory/Pointers.h"

//-------------------------------------------------------------------------

namespace EE::gltf
{
    EE_ENGINETOOLS_API TUniquePtr<RawAssets::RawAnimation> ReadAnimation( FileSystem::Path const& animationFilePath, RawAssets::RawSkeleton const& rawSkeleton, String const& animationName = String() );
}
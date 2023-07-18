#pragma once

#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    // Create a DDS texture from the supplied texture
    bool ConvertTexture( FileSystem::Path const& texturePath, TextureType type, Blob& rawData );
}
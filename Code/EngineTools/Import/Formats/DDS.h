#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Math/Math.h"
#include "EngineTools/Import/ImportedImage.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class Image;
}

//-------------------------------------------------------------------------

namespace EE::Import::DDS
{
    TUniquePtr<Image> ReadImage( FileSystem::Path const& path );
    TUniquePtr<Image> ReadImage( Blob const& blob );
}

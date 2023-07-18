#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Resource/ResourceTypeID.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE
{
    EE_ENGINETOOLS_API FileSystem::Path SaveDialog( String const& extension, FileSystem::Path const& startingPath = FileSystem::Path(), String const& friendlyFilterName = "" );
    EE_ENGINETOOLS_API FileSystem::Path SaveDialog( ResourceTypeID resourceTypeID, FileSystem::Path const& startingPath = FileSystem::Path(), String const& friendlyFilterName = "" );
}
#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Resource/ResourceTypeID.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE
{
    // Create a save dialog restricted to a specific extension, return true if the user has selected a valid path, returns false if they cancel or somehow select an invalid path
    EE_ENGINETOOLS_API bool SaveDialog( String const& extension, FileSystem::Path& outPath, FileSystem::Path const& startingPath = FileSystem::Path(), String const& friendlyFilterName = "" );

    // Create a save dialog restricted to a specific resource type, return true if the user has selected a valid path, returns false if they cancel or somehow select an invalid path
    EE_ENGINETOOLS_API bool SaveDialog( ResourceTypeID resourceTypeID, FileSystem::Path& outPath, FileSystem::Path const& startingPath = FileSystem::Path(), String const& friendlyFilterName = "" );
}
#pragma once

#include "Base/_Module/API.h"
#include "Base/Memory/Memory.h"
#include "Base/Types/String.h"
#include "Base/ThirdParty/pugixml/src/pugixml.hpp"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

using namespace pugi;

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    EE_BASE_API bool ReadXmlFromFile( FileSystem::Path const& path, xml_document& outDoc );
    EE_BASE_API bool ReadXmlFromString( String const& str, xml_document& outDoc, bool suppressLogging = false );
    EE_BASE_API bool WriteXmlToFile( xml_document const& doc, FileSystem::Path const& path, bool onlyWriteFileIfContentsChanged = false );
    EE_BASE_API bool WriteXmlToString( xml_document const& doc, String& outStr );
}
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

    // Load from a string but do not modify the string (will cause a copy of the data internally)
    EE_BASE_API bool ReadXmlFromBuffer( void const* pBuffer, size_t bufferSize, xml_document& outDoc, bool suppressLogging = false );

    // Load from a string but allow for modification of the buffer in place.
    // You are responsible for ensure the buffer lifetime outlives the xml_doc!
    EE_BASE_API bool ReadXmlFromBufferInPlace( void* pBuffer, size_t bufferSize, xml_document& outDoc, bool suppressLogging = false );

    // Load from a string but do not modify the string (will cause a copy of the data internally)
    inline bool ReadXmlFromString( String const& str, xml_document& outDoc, bool suppressLogging = false )
    {
        if ( str.empty() )
        {
            return false;
        }

        return ReadXmlFromBuffer( str.data(), str.size(), outDoc, suppressLogging );
    }

    //-------------------------------------------------------------------------

    EE_BASE_API bool WriteXmlToFile( xml_document const& doc, FileSystem::Path const& path, bool onlyWriteFileIfContentsChanged = false );
    EE_BASE_API bool WriteXmlToString( xml_document const& doc, String& outStr );
}
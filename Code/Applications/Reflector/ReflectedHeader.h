#pragma once
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ReflectedHeader
    {
    public:

        inline static StringID GetHeaderID( FileSystem::Path const& headerFilePath )
        {
            String lowercasePath = headerFilePath.GetString();
            lowercasePath.make_lower();
            return StringID( lowercasePath.c_str() );
        }

    public:

        StringID                                        m_ID;
        StringID                                        m_projectID;
        FileSystem::Path                                m_path;
        String                                          m_exportMacro;
        uint64_t                                        m_timestamp = 0;
        uint64_t                                        m_checksum = 0;
        TVector<String>                                 m_fileContents;
        FileSystem::Path                                m_typeInfoPath;
        bool                                            m_isToolsOnlyHeader = false;
    };
}
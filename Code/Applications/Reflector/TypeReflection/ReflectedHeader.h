#pragma once
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectedHeader
    {

    public:

        enum class ReflectionMacro
        {
            ReflectModule,
            ReflectEnum,
            ReflectType,
            ReflectProperty,
            Resource,
            DataFile,
            EntityComponent,
            SingletonEntityComponent,
            EntitySystem,
            EntityWorldSystem,

            NumMacros,
            Unknown = NumMacros,
        };

        static char const* GetReflectionMacroText( ReflectionMacro macro );

        //-------------------------------------------------------------------------

        enum class ParseResult
        {
            ErrorOccured,
            ProcessHeader,
            IgnoreHeader,
        };

        inline static StringID GetHeaderID( FileSystem::Path const& headerFilePath )
        {
            String lowercasePath = headerFilePath.GetString();
            lowercasePath.make_lower();
            return StringID( lowercasePath.c_str() );
        }

    public:

        ReflectedHeader( FileSystem::Path const& headerPath, FileSystem::Path const& projectTypeInfoPath, StringID projectID, bool isToolsProject );

        ParseResult Parse();

    public:

        StringID                                        m_ID;
        StringID                                        m_projectID;
        FileSystem::Path                                m_path;
        String                                          m_exportMacro;
        uint64_t                                        m_timestamp = 0;
        uint64_t                                        m_checksum = 0;
        TVector<String>                                 m_fileContents;
        FileSystem::Path                                m_typeInfoPath;
        bool                                            m_isModuleAPIHeader = false;
        bool                                            m_isModuleHeader = false;
        bool                                            m_isToolsHeader = false;
    };
}
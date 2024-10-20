#pragma once
#include "EngineTools/Resource/ResourceCompilerRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceServerContext
    {
        bool IsValid() const;

    public:

        FileSystem::Path                        m_sourceDataDirectoryPath;
        FileSystem::Path                        m_compiledResourceDirectoryPath;
        FileSystem::Path                        m_compilerExecutablePath;
        TypeSystem::TypeRegistry const*         m_pTypeRegistry = nullptr;
        CompilerRegistry const*                 m_pCompilerRegistry = nullptr;

        // Set when we shutdown the server to skip processing of any scheduled tasks
        bool                                    m_isExiting = false;
    };
}
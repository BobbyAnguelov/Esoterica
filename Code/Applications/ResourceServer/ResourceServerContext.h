#pragma once
#include "CompiledResourceDatabase.h"
#include "EngineTools/Resource/ResourceCompilerRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceServerContext
    {
        bool IsValid() const;

    public:

        FileSystem::Path                        m_rawResourcePath;
        FileSystem::Path                        m_compiledResourcePath;
        FileSystem::Path                        m_compilerExecutablePath;
        TypeSystem::TypeRegistry const*         m_pTypeRegistry = nullptr;
        CompilerRegistry const*                 m_pCompilerRegistry = nullptr;
        CompiledResourceDatabase const*         m_pCompiledResourceDB = nullptr;

        // Set when we shutdown the server to skip processing of any scheduled tasks
        bool                                    m_isExiting = false;
    };
}
#pragma once
#include "EngineTools/Resource/ResourceCompiler.h"
#include "CompiledResourceDatabase.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Settings/SettingsRegistry.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct CommandLineArgumentParser;
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceSettings;
    class CompilerRegistry;

    //-------------------------------------------------------------------------

    class ResourceCompilerApplication
    {
        struct CompileDependencyNode
        {
            void Reset();
            void DestroyDependencies();

            bool IsCompileableResource() const { return m_compilerVersion >= 0; }
            bool IsUpToDate() const;

        public:

            ResourceID                              m_ID;
            FileSystem::Path                        m_sourcePath;
            FileSystem::Path                        m_targetPath;
            bool                                    m_sourceExists = false;
            bool                                    m_targetExists = false;
            bool                                    m_errorOccurredReadingDependencies = true;
            bool                                    m_forceRecompile = false;
            int32_t                                 m_compilerVersion = -1;
            CompiledResourceRecord                  m_compiledRecord;
            uint64_t                                m_timestamp = 0;
            uint64_t                                m_combinedHash = 0;

            CompileDependencyNode*                  m_pParentNode = nullptr;
            TVector<CompileDependencyNode*>         m_dependencies;
        };

    public:

        static bool ShouldCheckCompileDependenciesForResourceType( ResourceID const& resourceID );

    public:

        ResourceCompilerApplication();
        ~ResourceCompilerApplication();

        bool Initialize( CommandLineArgumentParser& argParser, FileSystem::Path const& iniFilePath );
        void Shutdown();

        CompilationResult Run();

    private:

        bool BuildCompileDependencyTree( ResourceID const& resourceID );
        bool TryReadCompileDependencies( ResourceID const& resourceID, TVector<ResourcePath>& outDependencies );
        bool FillCompileDependencyNode( CompileDependencyNode* pNode, ResourcePath const& resourceID );

    private:

        TypeSystem::TypeRegistry                m_typeRegistry;
        Settings::SettingsRegistry              m_settingsRegistry;
        CompiledResourceDatabase                m_compiledResourceDB;
        CompilerRegistry*                       m_pCompilerRegistry = nullptr;
        CompileContext*                         m_pCompileContext = nullptr;
        bool                                    m_forceCompilation = false;

        TVector<ResourceID>                     m_uniqueCompileDependencies;
        CompileDependencyNode                   m_compileDependencyTreeRoot;
        String                                  m_errorMessage;
    };
}
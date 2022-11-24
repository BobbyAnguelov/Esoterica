#pragma once

#include "CompiledResourceDatabase.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceServerContext;

    //-------------------------------------------------------------------------

    bool ShouldCheckCompileDependenciesForResourceType( ResourceID const& resourceID );

    //-------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------

    class CompileDependencyTree
    {
    public:

        CompileDependencyTree( ResourceServerContext const& context );
        ~CompileDependencyTree();

        bool BuildTree( ResourceID const& resourceID );

        CompileDependencyNode const* GetRoot() const { return &m_root; }

        String const& GetErrorMessage() const { return m_errorMessage; }

    private:

        bool TryReadCompileDependencies( FileSystem::Path const& resourceFilePath, TVector<ResourceID>& outDependencies ) const;
        bool FillNode( CompileDependencyNode* pNode, ResourceID const& resourceID );

    private:

        ResourceServerContext const&            m_context;
        TVector<ResourceID>                     m_uniqueDependencies;
        CompileDependencyNode                   m_root;
        String                                  m_errorMessage;

    };
}
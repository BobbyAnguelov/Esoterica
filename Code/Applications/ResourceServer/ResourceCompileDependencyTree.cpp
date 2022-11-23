#include "ResourceCompileDependencyTree.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "ResourceServerContext.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class CompilerRegistry;

    //-------------------------------------------------------------------------

    bool ShouldCheckCompileDependencies( ResourceID const& resourceID )
    {
        return resourceID.GetResourceTypeID() != ResourceTypeID( "map" );
    }

    //-------------------------------------------------------------------------

    void CompileDependencyNode::Reset()
    {
        m_ID.Clear();
        m_compiledRecord.Clear();
        m_sourcePath.Clear();
        m_targetPath.Clear();
        m_timestamp = m_combinedHash = 0;
        m_sourceExists = m_targetExists = false;
        m_errorOccurredReadingDependencies = false;
        m_compilerVersion = -1;
        DestroyDependencies();
    }

    void CompileDependencyNode::DestroyDependencies()
    {
        for ( auto pDep : m_dependencies )
        {
            pDep->DestroyDependencies();
            EE::Delete( pDep );
        }

        m_dependencies.clear();
    }

    bool CompileDependencyNode::IsUpToDate() const
    {
        if ( !m_sourceExists )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( IsCompileableResource() )
        {
            if ( !m_targetExists )
            {
                return false;
            }

            if ( !m_compiledRecord.IsValid() )
            {
                return false;
            }

            if ( m_compiledRecord.m_compilerVersion != m_compilerVersion )
            {
                return false;
            }

            if ( m_compiledRecord.m_sourceTimestampHash != m_combinedHash )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& pDep : m_dependencies )
        {
            if ( !pDep->IsUpToDate() )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    //-------------------------------------------------------------------------

    CompileDependencyTree::CompileDependencyTree( ResourceServerContext const& context )
        : m_context( context )
    {
        EE_ASSERT( context.IsValid() );
    }

    CompileDependencyTree::~CompileDependencyTree()
    {
        m_root.DestroyDependencies();
    }

    bool CompileDependencyTree::BuildTree( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        //-------------------------------------------------------------------------

        m_errorMessage.clear();
        m_uniqueDependencies.clear();
        m_root.Reset();
        return FillNode( &m_root, resourceID );
    }

    bool CompileDependencyTree::TryReadCompileDependencies( FileSystem::Path const& resourceFilePath, TVector<ResourceID>& outDependencies ) const
    {
        EE_ASSERT( resourceFilePath.IsValid() );

        auto pDescriptor = ResourceDescriptor::TryReadFromFile( *m_context.m_pTypeRegistry, resourceFilePath );
        if ( pDescriptor == nullptr )
        {
            return false;
        }
        
        pDescriptor->GetCompileDependencies( outDependencies );

        EE::Delete( pDescriptor );

        return true;
    }

    bool CompileDependencyTree::FillNode( CompileDependencyNode* pNode, ResourceID const& resourceID )
    {
        EE_ASSERT( pNode != nullptr );

        // Basic resource info
        //-------------------------------------------------------------------------

        pNode->m_ID = resourceID;

        pNode->m_sourcePath = ResourcePath::ToFileSystemPath( m_context.m_rawResourcePath, resourceID.GetResourcePath() );
        pNode->m_sourceExists = FileSystem::Exists( pNode->m_sourcePath );
        pNode->m_timestamp = pNode->m_sourceExists ? FileSystem::GetFileModifiedTime( pNode->m_sourcePath ) : 0;

        // Handle compilable resources
        //-------------------------------------------------------------------------

        auto pCompiler = m_context.m_pCompilerRegistry->GetCompilerForResourceType( resourceID.GetResourceTypeID() );
        bool const isCompilableResource = pCompiler != nullptr;
        if ( isCompilableResource )
        {
            pNode->m_targetPath = ResourcePath::ToFileSystemPath( m_context.m_compiledResourcePath, resourceID.GetResourcePath() );
            pNode->m_targetExists = FileSystem::Exists( pNode->m_targetPath );

            pNode->m_compilerVersion = pCompiler->GetVersion();
            pNode->m_compiledRecord = m_context.m_pCompiledResourceDB->GetRecord( resourceID );
        }

        // Generate dependencies
        //-------------------------------------------------------------------------

        if ( isCompilableResource && ShouldCheckCompileDependencies( resourceID ) )
        {
            TVector<ResourceID> dependencies;
            if ( TryReadCompileDependencies( pNode->m_sourcePath, dependencies ) )
            {
                for ( auto const& dependencyResourceID : dependencies )
                {
                    // Skip resources already in the tree!
                    if ( VectorContains( m_uniqueDependencies, dependencyResourceID ) )
                    {
                        continue;
                    }

                    // Check for circular references
                    //-------------------------------------------------------------------------

                    auto pNodeToCheck = pNode;
                    while ( pNodeToCheck != nullptr )
                    {
                        if ( pNodeToCheck->m_ID == dependencyResourceID )
                        {
                            m_errorMessage = "Circular dependency detected!";
                            return false;
                        }

                        pNodeToCheck = pNodeToCheck->m_pParentNode;
                    }

                    // Create dependency
                    //-------------------------------------------------------------------------

                    auto pChildDependencyNode = pNode->m_dependencies.emplace_back( EE::New<CompileDependencyNode>() );
                    pChildDependencyNode->m_pParentNode = pNode;
                    if ( !FillNode( pChildDependencyNode, dependencyResourceID ) )
                    {
                        return false;
                    }

                    m_uniqueDependencies.emplace_back( dependencyResourceID );
                }
            }
            else
            {
                pNode->m_errorOccurredReadingDependencies = true;
                return false;
            }
        }

        // Generate combined hash
        //-------------------------------------------------------------------------

        pNode->m_combinedHash = pNode->m_timestamp;
        for ( auto const pDep : pNode->m_dependencies )
        {
            pNode->m_combinedHash += pDep->m_combinedHash;
        }

        return true;
    }
}
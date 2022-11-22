#include "ResourceCompileDependencyTree.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Resource/ResourceCompilerRegistry.h"

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

    CompileDependencyTree::CompileDependencyTree( FileSystem::Path const& rawResourcePath, FileSystem::Path const& compiledResourcePath, CompilerRegistry const& compilerRegistry, CompiledResourceDatabase const& compiledResourceDB )
        : m_rawResourcePath( rawResourcePath )
        , m_compiledResourcePath( compiledResourcePath )
        , m_compilerRegistry( compilerRegistry )
        , m_compiledResourceDB( compiledResourceDB )
    {
        EE_ASSERT( rawResourcePath.IsValid() && rawResourcePath.Exists() );
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
        m_root.Reset();
        return FillNode( &m_root, resourceID );
    }

    bool CompileDependencyTree::TryReadCompileDependencies( FileSystem::Path const& resourceFilePath, TVector<ResourceID>& outDependencies ) const
    {
        EE_ASSERT( resourceFilePath.IsValid() );

        // Read JSON descriptor file - we do this by hand since we dont want to create a type registry in the resource server
        if ( FileSystem::Exists( resourceFilePath ) )
        {
            FILE* fp = fopen( resourceFilePath, "r" );
            if ( fp == nullptr )
            {
                return false;
            }

            fseek( fp, 0, SEEK_END );
            size_t filesize = (size_t) ftell( fp );
            fseek( fp, 0, SEEK_SET );

            String fileContents;
            fileContents.resize( filesize );
            size_t const readLength = fread( fileContents.data(), 1, filesize, fp );
            fclose( fp );

            if ( readLength == 0 )
            {
                return false;
            }

            ResourceDescriptor::ReadCompileDependencies( fileContents, outDependencies );
        }
        else
        {
            return false;
        }

        return true;
    }

    bool CompileDependencyTree::FillNode( CompileDependencyNode* pNode, ResourceID const& resourceID )
    {
        EE_ASSERT( pNode != nullptr );

        //-------------------------------------------------------------------------

        pNode->m_ID = resourceID;

        pNode->m_sourcePath = ResourcePath::ToFileSystemPath( m_rawResourcePath, resourceID.GetResourcePath() );
        pNode->m_sourceExists = FileSystem::Exists( pNode->m_sourcePath );
        pNode->m_timestamp = pNode->m_sourceExists ? FileSystem::GetFileModifiedTime( pNode->m_sourcePath ) : 0;

        pNode->m_targetPath = ResourcePath::ToFileSystemPath( m_compiledResourcePath, resourceID.GetResourcePath() );
        pNode->m_targetExists = FileSystem::Exists( pNode->m_targetPath );

        auto pCompiler = m_compilerRegistry.GetCompilerForResourceType( resourceID.GetResourceTypeID() );
        if ( pCompiler != nullptr )
        {
            pNode->m_compilerVersion = pCompiler->GetVersion();
            pNode->m_compiledRecord = m_compiledResourceDB.GetRecord( resourceID );
        }

        // Generate dependencies
        //-------------------------------------------------------------------------

        if ( ShouldCheckCompileDependencies( resourceID ) )
        {
            TVector<ResourceID> dependencies;
            if ( TryReadCompileDependencies( pNode->m_sourcePath, dependencies ) )
            {
                for ( auto const& dependencyResourceID : dependencies )
                {
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
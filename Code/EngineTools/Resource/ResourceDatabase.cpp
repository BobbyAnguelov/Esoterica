#include "ResourceDatabase.h"
#include "System/FileSystem/FileSystemUtils.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    void ResourceDatabase::Directory::ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath )
    {
        FileSystem::Path const oldPath = m_filePath;
        m_name = StringID( newPath.GetDirectoryName() );
        m_filePath = newPath;

        // Rename sub-directories
        //-------------------------------------------------------------------------

        for ( auto& directory : m_directories )
        {
            FileSystem::Path const newSubdirectoryPath = newPath + directory.m_name.c_str();
            directory.ChangePath( rawResourceDirectoryPath, newSubdirectoryPath );
        }

        // Rename files
        //-------------------------------------------------------------------------

        for ( auto pRecord : m_files )
        {
            pRecord->m_filePath.ReplaceParentDirectory( newPath );
            pRecord->m_resourceID = ResourcePath::FromFileSystemPath( rawResourceDirectoryPath, pRecord->m_filePath );
        }
    }

    void ResourceDatabase::Directory::Clear()
    {
        for ( auto& dir : m_directories )
        {
            dir.Clear();
        }

        //-------------------------------------------------------------------------

        for ( auto& pRecord : m_files )
        {
            EE::Delete( pRecord );
        }

        //-------------------------------------------------------------------------

        m_directories.clear();
        m_files.clear();
    }

    //-------------------------------------------------------------------------

    ResourceDatabase::~ResourceDatabase()
    {
        EE_ASSERT( m_rootDir.IsEmpty() && m_resourcesPerType.empty() );
    }

    //-------------------------------------------------------------------------

    void ResourceDatabase::Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath )
    {
        EE_ASSERT( m_pTypeRegistry == nullptr );
        EE_ASSERT( pTypeRegistry != nullptr );
        EE_ASSERT( !m_rawResourceDirPath.IsValid() && FileSystem::Exists( rawResourceDirPath ) );
        EE_ASSERT( !m_compiledResourceDirPath.IsValid() && FileSystem::Exists( compiledResourceDirPath ) );

        m_rawResourceDirPath = rawResourceDirPath;
        m_compiledResourceDirPath = compiledResourceDirPath;
        m_dataDirectoryPathDepth = m_rawResourceDirPath.GetDirectoryDepth();
        m_pTypeRegistry = pTypeRegistry;

        //-------------------------------------------------------------------------

        RebuildDatabase();

        // Start file watcher
        //-------------------------------------------------------------------------

        m_fileSystemWatcher.RegisterChangeListener( this );
        m_fileSystemWatcher.StartWatching( m_rawResourceDirPath );
    }

    void ResourceDatabase::Shutdown()
    {
        // Stop file watcher
        //-------------------------------------------------------------------------

        m_fileSystemWatcher.StopWatching();
        m_fileSystemWatcher.UnregisterChangeListener( this );

        //-------------------------------------------------------------------------

        ClearDatabase();

        //-------------------------------------------------------------------------

        m_rawResourceDirPath.Clear();
        m_pTypeRegistry = nullptr;
    }

    bool ResourceDatabase::Update()
    {
        EE_ASSERT( m_fileSystemWatcher.IsWatching() );
        bool const databaseUpdated = m_fileSystemWatcher.Update();
        if ( databaseUpdated )
        {
            if ( m_databaseUpdatedEvent.HasBoundUsers() )
            {
                m_databaseUpdatedEvent.Execute();
            }
        }
        return databaseUpdated;
    }

    //-------------------------------------------------------------------------

    void ResourceDatabase::ClearDatabase()
    {
        m_resourcesPerType.clear();
        m_rootDir.Clear();

        if ( m_databaseUpdatedEvent.HasBoundUsers() )
        {
            m_databaseUpdatedEvent.Execute();
        }
    }

    // TODO: Make this async!
    void ResourceDatabase::RebuildDatabase()
    {
        m_resourcesPerType.clear();
        m_rootDir.Clear();

        //-------------------------------------------------------------------------

        m_rootDir.m_name = StringID( m_rawResourceDirPath.GetDirectoryName() );
        m_rootDir.m_filePath = m_rawResourceDirPath;

        //-------------------------------------------------------------------------

        TVector<FileSystem::Path> foundPaths;
        if ( !FileSystem::GetDirectoryContents( m_rawResourceDirPath, foundPaths, FileSystem::DirectoryReaderOutput::All, FileSystem::DirectoryReaderMode::Expand ) )
        {
            EE_HALT();
        }

        //-------------------------------------------------------------------------

        for ( auto const& filePath : foundPaths )
        {
            if ( filePath.IsDirectoryPath() )
            {
                Directory* pDirectory = FindOrCreateDirectory( filePath );
                EE_ASSERT( pDirectory != nullptr );
            }
            else
            {
                AddFileRecord( filePath );
            }
        }

        //-------------------------------------------------------------------------

        if ( m_databaseUpdatedEvent.HasBoundUsers() )
        {
            m_databaseUpdatedEvent.Execute();
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceDatabase::DoesResourceExist( ResourceID const& resourceID ) const
    {
        EE_ASSERT( resourceID.IsValid() );
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceID.GetResourceTypeID() ) );

        auto const& allResourcesofSameType = m_resourcesPerType.at( resourceID.GetResourceTypeID() );
        for ( auto pResourceRecord : allResourcesofSameType )
        {
            if ( pResourceRecord->m_resourceID == resourceID )
            {
                return true;
            }
        }

        return false;
    }

    TVector<ResourceDatabase::ResourceEntry*> const& ResourceDatabase::GetAllResourcesOfType( ResourceTypeID typeID ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( typeID ) );
        return m_resourcesPerType.at( typeID );
    }

    //-------------------------------------------------------------------------

    ResourceDatabase::Directory* ResourceDatabase::FindDirectory( FileSystem::Path const& dirPathToFind )
    {
        EE_ASSERT( dirPathToFind.IsDirectoryPath() );

        Directory* pCurrentDir = &m_rootDir;
        FileSystem::Path directoryPath = m_rawResourceDirPath;
        TInlineVector<String, 10> splitPath = dirPathToFind.Split();

        //-------------------------------------------------------------------------

        int32_t const pathDepth = (int32_t) splitPath.size();
        for ( int32_t i = m_dataDirectoryPathDepth + 1; i < pathDepth; i++ )
        {
            directoryPath.Append( splitPath[i] );

            StringID const intermediateName( splitPath[i] );
            auto searchPredicate = [&intermediateName] ( Directory& dir ) { return dir.m_name == intermediateName; };

            Directory* pFoundDirectory = nullptr;
            auto itemIter = eastl::find_if( pCurrentDir->m_directories.begin(), pCurrentDir->m_directories.end(), searchPredicate );
            if ( itemIter != pCurrentDir->m_directories.end() )
            {
                pCurrentDir = itemIter;
            }
            else
            {
                return nullptr;
            }
        }

        //-------------------------------------------------------------------------

        return pCurrentDir;
    }

    ResourceDatabase::Directory* ResourceDatabase::FindOrCreateDirectory( FileSystem::Path const& dirPathToFind )
    {
        EE_ASSERT( dirPathToFind.IsDirectoryPath() );

        Directory* pCurrentDir = &m_rootDir;
        FileSystem::Path directoryPath = m_rawResourceDirPath;
        TInlineVector<String, 10> splitPath = dirPathToFind.Split();

        //-------------------------------------------------------------------------

        int32_t const pathDepth = (int32_t) splitPath.size();
        for ( int32_t i = m_dataDirectoryPathDepth + 1; i < pathDepth; i++ )
        {
            directoryPath.Append( splitPath[i] );

            StringID const intermediateName( splitPath[i] );
            auto searchPredicate = [&intermediateName] ( Directory& dir ) { return dir.m_name == intermediateName; };

            Directory* pFoundDirectory = nullptr;
            auto itemIter = eastl::find_if( pCurrentDir->m_directories.begin(), pCurrentDir->m_directories.end(), searchPredicate );
            if ( itemIter != pCurrentDir->m_directories.end() )
            {
                pCurrentDir = itemIter;
            }
            else
            {
                auto& newDirectory = pCurrentDir->m_directories.emplace_back( Directory() );
                newDirectory.m_name = StringID( splitPath[i] );
                newDirectory.m_filePath = directoryPath;

                pCurrentDir = &newDirectory;
            }
        }

        //-------------------------------------------------------------------------

        return pCurrentDir;
    }

    void ResourceDatabase::AddFileRecord( FileSystem::Path const& path )
    {
        auto const resourcePath = ResourcePath::FromFileSystemPath(m_rawResourceDirPath, path);
        EE_ASSERT( resourcePath.IsFile() );

        auto pNewEntry = EE::New<ResourceEntry>();
        pNewEntry->m_filePath = path;
        pNewEntry->m_resourceID = resourcePath;

        // Add to directory list
        Directory* pDirectory = FindOrCreateDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );
        pDirectory->m_files.emplace_back( pNewEntry );

        // Add to per-type lists
        ResourceTypeID const typeID = pNewEntry->m_resourceID.GetResourceTypeID();
        if ( m_pTypeRegistry->IsRegisteredResourceType( typeID ) )
        {
            m_resourcesPerType[typeID].emplace_back( pNewEntry );
        }
    }

    void ResourceDatabase::RemoveFileRecord( FileSystem::Path const& path )
    {
        Directory* pDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );

        int32_t const numFiles = (int32_t) pDirectory->m_files.size();
        for ( int32_t i = 0; i < numFiles; i++ )
        {
            if ( pDirectory->m_files[i]->m_filePath == path )
            {
                // Remove from categorized resource lists
                ResourceID const& resourceID = pDirectory->m_files[i]->m_resourceID;
                if ( resourceID.IsValid() )
                {
                    ResourceTypeID const typeID = resourceID.GetResourceTypeID();

                    auto iter = m_resourcesPerType.find( typeID );
                    if ( iter != m_resourcesPerType.end() )
                    {
                        TVector<ResourceEntry*>& category = iter->second;
                        category.erase_first_unsorted( pDirectory->m_files[i] );
                    }

                    m_resourceDeletedEvent.Execute( resourceID );
                }

                // Destroy record
                EE::Delete( pDirectory->m_files[i] );
                pDirectory->m_files.erase_unsorted( pDirectory->m_files.begin() + i );
                return;
            }
        }
    }

    //-------------------------------------------------------------------------

    void ResourceDatabase::OnFileCreated( FileSystem::Path const& path )
    {
        AddFileRecord( path );
    }

    void ResourceDatabase::OnFileDeleted( FileSystem::Path const& path )
    {
        RemoveFileRecord( path );
    }

    void ResourceDatabase::OnFileRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath )
    {
        RemoveFileRecord( oldPath );
        AddFileRecord( newPath );
    }

    void ResourceDatabase::OnDirectoryCreated( FileSystem::Path const& newDirectoryPath )
    {
        TVector<FileSystem::Path> foundPaths;
        if ( !FileSystem::GetDirectoryContents( newDirectoryPath, foundPaths, FileSystem::DirectoryReaderOutput::OnlyFiles, FileSystem::DirectoryReaderMode::Expand ) )
        {
            EE_HALT();
        }

        // If this is an empty directory, add to the directory list
        if ( foundPaths.empty() )
        {
            Directory* pDirectory = FindOrCreateDirectory( newDirectoryPath );
            EE_ASSERT( pDirectory != nullptr );
        }
        else // Add file records (this will automatically create the directory record)
        {
            for ( auto const& filePath : foundPaths )
            {
                AddFileRecord( filePath );
            }
        }
    }

    void ResourceDatabase::OnDirectoryDeleted( FileSystem::Path const& path )
    {
        auto pParentDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pParentDirectory != nullptr );

        int32_t const numDirectories = (int32_t) pParentDirectory->m_directories.size();
        for ( int32_t i = 0; i < numDirectories; i++ )
        {
            if ( pParentDirectory->m_directories[i].m_filePath == path )
            {
                // Delete all children and remove directory
                pParentDirectory->m_directories[i].Clear();
                pParentDirectory->m_directories.erase_unsorted( pParentDirectory->m_directories.begin() + i );
                break;
            }
        }
    }

    void ResourceDatabase::OnDirectoryRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath )
    {
        EE_ASSERT( oldPath.IsDirectoryPath() );
        EE_ASSERT( newPath.IsDirectoryPath() );

        Directory* pDirectory = nullptr;

        // Check if the directory was also moved
        FileSystem::Path const oldParentPath = oldPath.GetParentDirectory();
        FileSystem::Path const newParentPath = newPath.GetParentDirectory();
        if ( oldParentPath != newParentPath )
        {
            auto pOldParentDirectory = FindDirectory( oldParentPath );
            EE_ASSERT( pOldParentDirectory != nullptr );

            auto pNewParentDirectory = FindOrCreateDirectory( newParentPath );
            EE_ASSERT( pNewParentDirectory );

            // Move directory to new parent
            //-------------------------------------------------------------------------

            bool directoryMoved = false;
            int32_t const numOldDirectories = (int32_t) pOldParentDirectory->m_directories.size();
            for ( int32_t i = 0; i < numOldDirectories; i++ )
            {
                if ( pOldParentDirectory->m_directories[i].m_filePath == oldPath )
                {
                    pNewParentDirectory->m_directories.emplace_back( pOldParentDirectory->m_directories[i] );
                    pOldParentDirectory->m_directories.erase_unsorted( pOldParentDirectory->m_directories.begin() + i );
                    directoryMoved = true;
                    break;
                }
            }

            EE_ASSERT( directoryMoved );

            // Update directory
            //-------------------------------------------------------------------------

            pDirectory = &pNewParentDirectory->m_directories.back();
        }
        else
        {
            pDirectory = FindDirectory( oldPath );
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( pDirectory != nullptr );
        pDirectory->ChangePath( m_rawResourceDirPath, newPath );
    }
}
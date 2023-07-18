#include "ResourceDatabase.h"
#include "ResourceDescriptor.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceDatabase::FileEntry::~FileEntry()
    {
        EE::Delete( m_pDescriptor );
    }

    //-------------------------------------------------------------------------

    void ResourceDatabase::DirectoryEntry::ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath )
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

    void ResourceDatabase::DirectoryEntry::Clear()
    {
        for ( auto& dir : m_directories )
        {
            dir.Clear();
        }

        //-------------------------------------------------------------------------

        for ( auto& pFile : m_files )
        {
            EE::Delete( pFile );
        }

        //-------------------------------------------------------------------------

        m_directories.clear();
        m_files.clear();
    }

    //-------------------------------------------------------------------------

    ResourceDatabase::~ResourceDatabase()
    {
        EE_ASSERT( m_reflectedDataDirectory.IsEmpty() && m_resourcesPerType.empty() && m_resourcesPerPath.empty() );
    }

    //-------------------------------------------------------------------------

    void ResourceDatabase::Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, TaskSystem* pTaskSystem, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath )
    {
        EE_ASSERT( m_pTypeRegistry == nullptr && pTypeRegistry != nullptr );
        EE_ASSERT( m_pTaskSystem == nullptr && pTaskSystem != nullptr );
        EE_ASSERT( !m_rawResourceDirPath.IsValid() && FileSystem::Exists( rawResourceDirPath ) );
        EE_ASSERT( !m_compiledResourceDirPath.IsValid() && FileSystem::Exists( compiledResourceDirPath ) );

        m_rawResourceDirPath = rawResourceDirPath;
        m_compiledResourceDirPath = compiledResourceDirPath;
        m_dataDirectoryPathDepth = m_rawResourceDirPath.GetDirectoryDepth();
        m_pTaskSystem = pTaskSystem;
        m_pTypeRegistry = pTypeRegistry;

        //-------------------------------------------------------------------------

        RequestDatabaseRebuild();
    }

    void ResourceDatabase::Shutdown()
    {
        CancelDatabaseRebuild();

        // Stop file watcher
        //-------------------------------------------------------------------------

        m_fileSystemWatcher.StopWatching();
        m_fileSystemWatcher.UnregisterChangeListener( this );

        //-------------------------------------------------------------------------

        ClearDatabase();

        if ( m_databaseUpdatedEvent.HasBoundUsers() )
        {
            m_databaseUpdatedEvent.Execute();
        }

        //-------------------------------------------------------------------------

        m_rawResourceDirPath.Clear();
        m_pTypeRegistry = nullptr;
    }

    bool ResourceDatabase::Update()
    {
        // Wait for rebuild to complete
        //-------------------------------------------------------------------------

        if ( m_status != Status::Idle )
        {
            EE_ASSERT( m_pAsyncTask != nullptr );
            if ( m_pAsyncTask->GetIsComplete() )
            {
                EE::Delete( m_pAsyncTask );

                // Notify users that the DB has been rebuilt
                m_databaseUpdatedEvent.Execute();

                // Set status to idle
                m_status = Status::Idle;
            }
        }

        // Update file watcher
        //-------------------------------------------------------------------------

        if ( m_status == Status::Idle )
        {
            EE_ASSERT( m_fileSystemWatcher.IsWatching() );
            bool const fileSystemChangesDetected = m_fileSystemWatcher.Update();
            if ( fileSystemChangesDetected )
            {
                if ( m_databaseUpdatedEvent.HasBoundUsers() )
                {
                    m_databaseUpdatedEvent.Execute();
                }
            }

            return fileSystemChangesDetected;
        }

        //-------------------------------------------------------------------------

        return false;
    }

    //-------------------------------------------------------------------------

    void ResourceDatabase::ClearDatabase()
    {
        m_resourcesPerType.clear();
        m_resourcesPerPath.clear();
        m_reflectedDataDirectory.Clear();
    }

    void ResourceDatabase::CancelDatabaseRebuild()
    {
        if ( m_pAsyncTask != nullptr )
        {
            m_cancelActiveTask = true;
            m_pTaskSystem->WaitForTask( m_pAsyncTask );
            EE::Delete( m_pAsyncTask );
            ClearDatabase();
            m_cancelActiveTask = false;
        }
    }

    void ResourceDatabase::RequestDatabaseRebuild()
    {
        // Cancel any running task
        //-------------------------------------------------------------------------

        CancelDatabaseRebuild();

        // Stop file watcher
        //-------------------------------------------------------------------------

        if ( m_fileSystemWatcher.IsWatching() )
        {
            m_fileSystemWatcher.StopWatching();
            m_fileSystemWatcher.UnregisterChangeListener( this );
        }

        // Kick off new task
        //-------------------------------------------------------------------------

        struct Task : public ITaskSet
        {
            Task( ResourceDatabase* pDB ) : m_pDB( pDB ) {}

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
            {
                m_pDB->RebuildDatabase();
            }

            ResourceDatabase* m_pDB;
        };

        EE_ASSERT( m_pAsyncTask == nullptr );
        m_status = Status::BuildingDB;
        m_pAsyncTask = EE::New<Task>( this );
        m_pTaskSystem->ScheduleTask( m_pAsyncTask );
        m_rebuildProgress = 0.0f;
    }

    void ResourceDatabase::RebuildDatabase()
    {
        EE_ASSERT( m_status == Status::BuildingDB );

        // Reset the resource type category and add an entry for for every known resource type
        //-------------------------------------------------------------------------

        m_resourcesPerType.clear();
        for ( auto const& resourceInfoPair : m_pTypeRegistry->GetRegisteredResourceTypes() )
        {
            auto const& resourceInfo = resourceInfoPair.second;
            m_resourcesPerType.insert( TPair<ResourceTypeID, TVector<FileEntry*>>( resourceInfo.m_resourceTypeID, TVector<FileEntry*>() ) );
        }

        // Reset file map
        //-------------------------------------------------------------------------

        m_resourcesPerPath.clear();

        // Reset the root dir
        //-------------------------------------------------------------------------

        m_reflectedDataDirectory.Clear();
        m_reflectedDataDirectory.m_name = StringID( m_rawResourceDirPath.GetDirectoryName() );
        m_reflectedDataDirectory.m_filePath = m_rawResourceDirPath;

        // Get all files in the data directory
        //-------------------------------------------------------------------------

        if ( m_cancelActiveTask )
        {
            return;
        }

        TVector<FileSystem::Path> foundPaths;
        if ( !FileSystem::GetDirectoryContents( m_rawResourceDirPath, foundPaths, FileSystem::DirectoryReaderOutput::All, FileSystem::DirectoryReaderMode::Expand ) )
        {
            EE_HALT();
        }

        // Add record for all files
        //-------------------------------------------------------------------------

        int32_t const numFoundPaths = (int32_t) foundPaths.size();
        for ( int32_t i = 0; i < numFoundPaths; i++ )
        {
            auto const& filePath = foundPaths[i];

            if ( m_cancelActiveTask )
            {
                m_rebuildProgress = 1.0f;
                return;
            }

            if ( filePath.IsDirectoryPath() )
            {
                DirectoryEntry* pDirectory = FindOrCreateDirectory( filePath );
                EE_ASSERT( pDirectory != nullptr );
            }
            else
            {
                AddFileRecord( filePath );
            }

            m_rebuildProgress = float( i ) / ( numFoundPaths - 1 );
        }

        EE_ASSERT( m_rebuildProgress == 1.0f );

        // Start file system watcher
        //-------------------------------------------------------------------------

        EE_ASSERT( !m_fileSystemWatcher.IsWatching() );
        m_fileSystemWatcher.RegisterChangeListener( this );
        m_fileSystemWatcher.StartWatching( m_rawResourceDirPath );
    }

    //-------------------------------------------------------------------------

    ResourceDatabase::FileEntry const* ResourceDatabase::GetFileEntry( ResourcePath const& resourcePath ) const
    {
        auto fileEntryIter = m_resourcesPerPath.find( resourcePath );;
        if ( fileEntryIter != m_resourcesPerPath.end() )
        {
            return fileEntryIter->second;
        }

        return  nullptr;
    }

    bool ResourceDatabase::DoesResourceExist( ResourceID const& resourceID ) const
    {
        EE_ASSERT( resourceID.IsValid() );
        return m_resourcesPerPath.find( resourceID.GetResourcePath() ) != m_resourcesPerPath.end();
    }

    bool ResourceDatabase::DoesResourceExist( ResourcePath const& resourcePath ) const
    {
        EE_ASSERT( resourcePath.IsValid() );
        return m_resourcesPerPath.find( resourcePath ) != m_resourcesPerPath.end();
    }

    TVector<ResourceID> ResourceDatabase::GetAllResourcesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<ResourceID> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( auto const& entry : foundEntries )
        {
            results.emplace_back( entry->m_resourceID );
        }

        //-------------------------------------------------------------------------

        if ( includeDerivedTypes )
        {
            auto const derivedResourceTypeIDs = m_pTypeRegistry->GetAllDerivedResourceTypes( resourceTypeID );
            for ( ResourceTypeID derivedResourceTypeID : derivedResourceTypeIDs )
            {
                auto const& derivedResources = m_resourcesPerType.at( derivedResourceTypeID );
                for ( auto const& entry : derivedResources )
                {
                    results.emplace_back( entry->m_resourceID );
                }
            }
        }

        return results;
    }

    TVector<EE::ResourceID> ResourceDatabase::GetAllResourcesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( ResourceDescriptor const* )> const& filter, bool includeDerivedTypes ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<ResourceID> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( auto const& entry : foundEntries )
        {
            if ( filter( entry->m_pDescriptor ) )
            {
                results.emplace_back( entry->m_resourceID );
            }
        }

        //-------------------------------------------------------------------------

        if ( includeDerivedTypes )
        {
            auto const derivedResourceTypeIDs = m_pTypeRegistry->GetAllDerivedResourceTypes( resourceTypeID );
            for ( ResourceTypeID derivedResourceTypeID : derivedResourceTypeIDs )
            {
                auto const& derivedResources = m_resourcesPerType.at( derivedResourceTypeID );
                for ( auto const& entry : derivedResources )
                {
                    if ( filter( entry->m_pDescriptor ) )
                    {
                        results.emplace_back( entry->m_resourceID );
                    }
                }
            }
        }

        return results;
    }

    //-------------------------------------------------------------------------

    ResourceDatabase::DirectoryEntry* ResourceDatabase::FindDirectory( FileSystem::Path const& dirPathToFind )
    {
        EE_ASSERT( dirPathToFind.IsDirectoryPath() );

        DirectoryEntry* pCurrentDir = &m_reflectedDataDirectory;
        FileSystem::Path directoryPath = m_rawResourceDirPath;
        TInlineVector<String, 10> splitPath = dirPathToFind.Split();

        //-------------------------------------------------------------------------

        int32_t const pathDepth = (int32_t) splitPath.size();
        for ( int32_t i = m_dataDirectoryPathDepth + 1; i < pathDepth; i++ )
        {
            directoryPath.Append( splitPath[i] );

            StringID const intermediateName( splitPath[i] );
            auto searchPredicate = [&intermediateName] ( DirectoryEntry& dir ) { return dir.m_name == intermediateName; };

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

    ResourceDatabase::DirectoryEntry* ResourceDatabase::FindOrCreateDirectory( FileSystem::Path const& dirPathToFind )
    {
        EE_ASSERT( dirPathToFind.IsDirectoryPath() );

        DirectoryEntry* pCurrentDir = &m_reflectedDataDirectory;
        FileSystem::Path directoryPath = m_rawResourceDirPath;
        TInlineVector<String, 10> splitPath = dirPathToFind.Split();

        //-------------------------------------------------------------------------

        int32_t const pathDepth = (int32_t) splitPath.size();
        for ( int32_t i = m_dataDirectoryPathDepth + 1; i < pathDepth; i++ )
        {
            directoryPath.Append( splitPath[i] );

            StringID const intermediateName( splitPath[i] );
            auto searchPredicate = [&intermediateName] ( DirectoryEntry& dir ) { return dir.m_name == intermediateName; };

            auto itemIter = eastl::find_if( pCurrentDir->m_directories.begin(), pCurrentDir->m_directories.end(), searchPredicate );
            if ( itemIter != pCurrentDir->m_directories.end() )
            {
                pCurrentDir = itemIter;
            }
            else
            {
                auto& newDirectory = pCurrentDir->m_directories.emplace_back( DirectoryEntry() );
                newDirectory.m_name = StringID( splitPath[i] );
                newDirectory.m_filePath = directoryPath;
                newDirectory.m_resourcePath = ResourcePath::FromFileSystemPath( m_rawResourceDirPath, newDirectory.m_filePath );

                pCurrentDir = &newDirectory;
            }
        }

        //-------------------------------------------------------------------------

        return pCurrentDir;
    }

    void ResourceDatabase::AddFileRecord( FileSystem::Path const& path )
    {
        auto const resourcePath = ResourcePath::FromFileSystemPath( m_rawResourceDirPath, path );
        EE_ASSERT( resourcePath.IsFile() );

        // Create entry
        auto pNewEntry = EE::New<FileEntry>();
        pNewEntry->m_filePath = path;
        pNewEntry->m_resourceID = resourcePath;
        pNewEntry->m_isRegisteredResourceType = m_pTypeRegistry->IsRegisteredResourceType( pNewEntry->m_resourceID.GetResourceTypeID() );

        // Read descriptor
        if ( pNewEntry->m_isRegisteredResourceType )
        {
            pNewEntry->m_pDescriptor = ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, path );
        }

        // Add to directory list
        DirectoryEntry* pDirectory = FindOrCreateDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );
        pDirectory->m_files.emplace_back( pNewEntry );

        // Add to per-type lists
        if ( pNewEntry->m_isRegisteredResourceType )
        {
            m_resourcesPerType[pNewEntry->m_resourceID.GetResourceTypeID()].emplace_back( pNewEntry );
        }

        // Add to file map
        m_resourcesPerPath[resourcePath] = pNewEntry;
    }

    void ResourceDatabase::RemoveFileRecord( FileSystem::Path const& path )
    {
        DirectoryEntry* pDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );

        int32_t const numFiles = (int32_t) pDirectory->m_files.size();
        for ( int32_t i = 0; i < numFiles; i++ )
        {
            if ( pDirectory->m_files[i]->m_filePath == path )
            {
                // Remove from file map
                auto fileMapiter = m_resourcesPerPath.find( pDirectory->m_files[i]->m_resourceID.GetResourcePath() );
                if ( fileMapiter != m_resourcesPerPath.end() )
                {
                    m_resourcesPerPath.erase( fileMapiter );
                }

                // Remove from categorized resource lists
                ResourceID const& resourceID = pDirectory->m_files[i]->m_resourceID;
                if ( resourceID.IsValid() )
                {
                    ResourceTypeID const typeID = resourceID.GetResourceTypeID();

                    auto iter = m_resourcesPerType.find( typeID );
                    if ( iter != m_resourcesPerType.end() )
                    {
                        TVector<FileEntry*>& category = iter->second;
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

    // Watcher Events
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

    void ResourceDatabase::OnFileModified( FileSystem::Path const& path )
    {
        DirectoryEntry* pDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );

        int32_t const numFiles = (int32_t) pDirectory->m_files.size();
        for ( int32_t i = 0; i < numFiles; i++ )
        {
            if ( pDirectory->m_files[i]->m_filePath == path )
            {
                if ( pDirectory->m_files[i]->m_isRegisteredResourceType )
                {
                    EE::Delete( pDirectory->m_files[i]->m_pDescriptor );
                    pDirectory->m_files[i]->m_pDescriptor = ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, path );
                }

                break;
            }
        }
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
            DirectoryEntry* pDirectory = FindOrCreateDirectory( newDirectoryPath );
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

        DirectoryEntry* pDirectory = nullptr;

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
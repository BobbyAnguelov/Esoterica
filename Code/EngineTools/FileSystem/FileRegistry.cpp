#include "FileRegistry.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    FileRegistry::FileInfo::~FileInfo()
    {
        EE::Delete( m_pDataFile );
    }

    FileRegistry::FileInfo& FileRegistry::FileInfo::operator=( FileInfo&& rhs )
    {
        m_dataPath = rhs.m_dataPath;
        m_filePath = rhs.m_filePath;
        m_extension = rhs.m_extension;
        m_extensionFourCC = rhs.m_extensionFourCC;
        m_fileType = rhs.m_fileType;
        m_pDataFile = rhs.m_pDataFile;

        rhs.m_pDataFile = nullptr;

        return *this;
    }

    FileRegistry::FileInfo& FileRegistry::FileInfo::operator=( FileInfo const& rhs )
    {
        m_dataPath = rhs.m_dataPath;
        m_filePath = rhs.m_filePath;
        m_extension = rhs.m_extension;
        m_extensionFourCC = rhs.m_extensionFourCC;
        m_fileType = rhs.m_fileType;

        return *this;
    }

    void FileRegistry::FileInfo::LoadDataFile( TypeSystem::TypeRegistry const& typeRegistry )
    {
        EE_ASSERT( IsResourceDescriptorFile() || IsDataFile() );
        EE_ASSERT( m_pDataFile == nullptr );
        EE_ASSERT( m_filePath.IsValid() );

        m_pDataFile = IDataFile::TryReadFromFile( typeRegistry, m_filePath );
    }

    void FileRegistry::FileInfo::ReloadDataFile( TypeSystem::TypeRegistry const& typeRegistry )
    {
        EE::Delete( m_pDataFile );
        LoadDataFile( typeRegistry );
    }

    //-------------------------------------------------------------------------

    void FileRegistry::DirectoryInfo::ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath )
    {
        FileSystem::Path const oldPath = m_filePath;
        m_name = newPath.GetDirectoryName();
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
            pRecord->m_dataPath = DataPath::FromFileSystemPath( rawResourceDirectoryPath, pRecord->m_filePath );
        }
    }

    void FileRegistry::DirectoryInfo::Clear()
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

    void FileRegistry::DirectoryInfo::GetAllFiles( TVector<FileInfo const*>& files, bool recurseIntoChildDirectories ) const
    {
        for ( auto pFile : m_files )
        {
            files.emplace_back( pFile );
        }

        if ( recurseIntoChildDirectories )
        {
            for ( DirectoryInfo const& directory : m_directories )
            {
                directory.GetAllFiles( files, recurseIntoChildDirectories );
            }
        }
    }

    //-------------------------------------------------------------------------

    FileRegistry::~FileRegistry()
    {
        EE_ASSERT( m_state == DatabaseState::Empty );
        EE_ASSERT( m_sourceDataDirectoryInfo.IsEmpty() && m_resourcesPerType.empty() && m_filesPerPath.empty() );
    }

    float FileRegistry::GetProgress() const
    {
        if ( m_totalItemsToProcess == 0 )
        {
            return 1.0f;
        }
        else
        {
            float const progress = float( m_numItemsProcessed ) / m_totalItemsToProcess;
            EE_ASSERT( !Math::IsNaNOrInf( progress ) );
            return progress;
        }
    }

    //-------------------------------------------------------------------------

    void FileRegistry::Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, TaskSystem* pTaskSystem, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath )
    {
        EE_ASSERT( m_pTypeRegistry == nullptr && pTypeRegistry != nullptr );
        EE_ASSERT( m_pTaskSystem == nullptr && pTaskSystem != nullptr );
        EE_ASSERT( !m_sourceDataDirPath.IsValid() && FileSystem::Exists( rawResourceDirPath ) );
        EE_ASSERT( !m_compiledResourceDirPath.IsValid() && FileSystem::Exists( compiledResourceDirPath ) );

        m_sourceDataDirPath = rawResourceDirPath;
        m_compiledResourceDirPath = compiledResourceDirPath;
        m_dataDirectoryPathDepth = m_sourceDataDirPath.GetDirectoryDepth();
        m_pTaskSystem = pTaskSystem;
        m_pTypeRegistry = pTypeRegistry;

        // Get list of all known resource descriptors
        //-------------------------------------------------------------------------

        m_resourceTypesWithDescriptors.clear();

        TVector<TypeSystem::TypeInfo const*> descriptorTypeInfos = m_pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false, false );
        for ( auto pTypeInfo : descriptorTypeInfos )
        {
            auto pDescriptorDefaultInstance = Cast<Resource::ResourceDescriptor>( pTypeInfo->GetDefaultInstance() );
            m_resourceTypesWithDescriptors.emplace_back( pDescriptorDefaultInstance->GetCompiledResourceTypeID() );
        }

        // Start database build
        //-------------------------------------------------------------------------

        StartFilesystemCacheBuild();

        // Start file system watcher
        //-------------------------------------------------------------------------

        m_massiveFileSystemChangeDetectedEventBinding = m_fileSystemWatcher.OnMassiveChangeDetected().Bind( [this] () { HandleMassiveFileSystemChangeDetected(); } );

        EE_ASSERT( !m_fileSystemWatcher.IsWatching() );
        m_fileSystemWatcher.StartWatching( m_sourceDataDirPath );
    }

    void FileRegistry::Shutdown()
    {
        CancelDatabaseBuild();

        // Stop file watcher
        //-------------------------------------------------------------------------

        m_fileSystemWatcher.StopWatching();
        m_fileSystemWatcher.OnMassiveChangeDetected().Unbind( m_massiveFileSystemChangeDetectedEventBinding );

        //-------------------------------------------------------------------------

        ClearDatabase();

        if ( m_fileCacheUpdatedEvent.HasBoundUsers() )
        {
            m_fileCacheUpdatedEvent.Execute();
        }

        //-------------------------------------------------------------------------

        m_resourceTypesWithDescriptors.clear();
        m_sourceDataDirPath.Clear();
        m_pTypeRegistry = nullptr;
    }

    bool FileRegistry::Update()
    {
        // Wait for rebuild to complete
        //-------------------------------------------------------------------------

        if ( m_pAsyncTask != nullptr )
        {
            if ( m_pAsyncTask->GetIsComplete() )
            {
                EE::Delete( m_pAsyncTask );

                if ( m_state == DatabaseState::BuildingFileSystemCache )
                {
                    EE_ASSERT( m_numItemsProcessed == m_totalItemsToProcess );

                    // Notify users that the DB has been rebuilt
                    m_fileCacheUpdatedEvent.Execute();

                    // Start loading descriptors
                    if ( !m_dataFilesToLoad.empty() )
                    {
                        StartDataFileCacheBuild();
                    }
                    else // Nothing else to do
                    {
                        m_state = DatabaseState::Ready;
                    }
                }
                else if ( m_state == DatabaseState::BuildingDataFileCache )
                {
                    EE_ASSERT( m_numItemsProcessed == m_totalItemsToProcess );

                    m_dataFilesToLoad.clear();
                    m_state = DatabaseState::Ready;
                }
                else // Error
                {
                    EE_UNREACHABLE_CODE();
                }
            }
        }

        // Update file watcher
        //-------------------------------------------------------------------------

        if ( m_fileSystemWatcher.Update() )
        {
            ProcessFileSystemChanges();
            return true;
        }

        //-------------------------------------------------------------------------

        return false;
    }

    //-------------------------------------------------------------------------

    void FileRegistry::RequestRebuild()
    {
        CancelDatabaseBuild();
        ClearDatabase();
        StartFilesystemCacheBuild();
    }

    void FileRegistry::ClearDatabase()
    {
        m_resourcesPerType.clear();
        m_filesPerPath.clear();
        m_sourceDataDirectoryInfo.Clear();
        m_dataFilesToLoad.empty();
        m_numItemsProcessed = m_totalItemsToProcess = 0;
        m_state = DatabaseState::Empty;
    }

    void FileRegistry::CancelDatabaseBuild()
    {
        if ( m_pAsyncTask != nullptr )
        {
            m_cancelActiveTask = true;
            m_pTaskSystem->WaitForTask( m_pAsyncTask );
            EE::Delete( m_pAsyncTask );
            ClearDatabase();
            m_cancelActiveTask = false;
            m_numItemsProcessed = 0;
            m_totalItemsToProcess = 1;
        }
    }

    void FileRegistry::HandleMassiveFileSystemChangeDetected()
    {
        RequestRebuild();
    }

    void FileRegistry::StartFilesystemCacheBuild()
    {
        EE_ASSERT( m_state == DatabaseState::Empty );
        EE_ASSERT( m_pAsyncTask == nullptr );
        EE_ASSERT( m_dataFilesToLoad.empty() );

        //-------------------------------------------------------------------------

        auto BuildFileSystemCache = [this] ( TaskSetPartition range, uint32_t threadnum )
        {
            // Reset the resource type category and add an entry for for every known resource type
            //-------------------------------------------------------------------------

            m_resourcesPerType.clear();
            for ( auto const& resourceInfoPair : m_pTypeRegistry->GetRegisteredResourceTypes() )
            {
                TypeSystem::ResourceInfo const* pResourceInfo = resourceInfoPair.second;
                m_resourcesPerType.insert( TPair<ResourceTypeID, TVector<FileInfo*>>( pResourceInfo->m_resourceTypeID, TVector<FileInfo*>() ) );
            }

            // Reset file map
            //-------------------------------------------------------------------------

            m_filesPerPath.clear();

            // Reset the root dir
            //-------------------------------------------------------------------------

            m_sourceDataDirectoryInfo.Clear();
            m_sourceDataDirectoryInfo.m_name = m_sourceDataDirPath.GetDirectoryName();
            m_sourceDataDirectoryInfo.m_filePath = m_sourceDataDirPath;
            m_sourceDataDirectoryInfo.m_dataPath = DataPath::FromFileSystemPath( m_sourceDataDirPath, m_sourceDataDirPath );

            // Get all files in the data directory
            //-------------------------------------------------------------------------

            if ( m_cancelActiveTask )
            {
                return;
            }

            TVector<FileSystem::Path> foundPaths;
            if ( !FileSystem::GetDirectoryContents( m_sourceDataDirPath, foundPaths, FileSystem::DirectoryReaderOutput::All, FileSystem::DirectoryReaderMode::Recursive ) )
            {
                EE_HALT();
            }

            // Add record for all files
            //-------------------------------------------------------------------------

            m_totalItemsToProcess = (int32_t) foundPaths.size();
            for ( int32_t i = 0; i < m_totalItemsToProcess; i++ )
            {
                auto const& filePath = foundPaths[i];

                if ( m_cancelActiveTask )
                {
                    m_numItemsProcessed = m_totalItemsToProcess;
                    return;
                }

                //-------------------------------------------------------------------------

                if ( filePath.IsDirectoryPath() )
                {
                    DirectoryInfo* pDirectory = FindOrCreateDirectory( filePath );
                    EE_ASSERT( pDirectory != nullptr );
                }
                else
                {
                    auto pCreatedFileEntry = AddFileRecord( filePath, false );

                    // Queue for descriptor load
                    if ( pCreatedFileEntry->IsResourceDescriptorFile() || pCreatedFileEntry->IsDataFile() )
                    {
                        m_dataFilesToLoad.emplace_back( pCreatedFileEntry );
                    }
                }

                m_numItemsProcessed++;
            }

            EE_ASSERT( m_numItemsProcessed == m_totalItemsToProcess );
        };

        //-------------------------------------------------------------------------

        m_numItemsProcessed = 0;
        m_totalItemsToProcess = 0;

        m_state = DatabaseState::BuildingFileSystemCache;
        m_pAsyncTask = EE::New<AsyncTask>( BuildFileSystemCache );
        m_pTaskSystem->ScheduleTask( m_pAsyncTask );
    }

    void FileRegistry::StartDataFileCacheBuild()
    {
        EE_ASSERT( m_state == DatabaseState::BuildingFileSystemCache );
        EE_ASSERT( m_pAsyncTask == nullptr );
        EE_ASSERT( !m_dataFilesToLoad.empty() );

        //-------------------------------------------------------------------------

        auto BuildDescriptorCache = [this] ( TaskSetPartition range, uint32_t threadnum )
        {
            for ( uint32_t i = range.start; i < range.end; i++ )
            {
                m_dataFilesToLoad[i]->LoadDataFile( *m_pTypeRegistry );
                m_dataFilesToLoad[i] = nullptr;
                m_numItemsProcessed++;
            }
        };

        //-------------------------------------------------------------------------

        m_numItemsProcessed = 0;
        m_totalItemsToProcess = (int32_t) m_dataFilesToLoad.size();

        m_state = DatabaseState::BuildingDataFileCache;
        m_pAsyncTask = EE::New<AsyncTask>( m_totalItemsToProcess, BuildDescriptorCache );
        m_pTaskSystem->ScheduleTask( m_pAsyncTask );
    }

    //-------------------------------------------------------------------------

    FileRegistry::FileInfo const* FileRegistry::GetFileEntry( DataPath const& dataPath ) const
    {
        auto fileEntryIter = m_filesPerPath.find( dataPath );;
        if ( fileEntryIter != m_filesPerPath.end() )
        {
            return fileEntryIter->second;
        }

        return  nullptr;
    }

    TVector<FileRegistry::FileInfo const*> FileRegistry::GetAllResourceFileEntries( ResourceTypeID resourceTypeID, bool includeDerivedTypes ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<FileInfo const*> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( auto const& entry : foundEntries )
        {
            results.emplace_back( entry );
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
                    results.emplace_back( entry );
                }
            }
        }

        return results;
    }

    TVector<FileRegistry::FileInfo const*> FileRegistry::GetAllResourceFileEntriesFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes /*= false */ ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<FileInfo const*> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( auto const& entry : foundEntries )
        {
            if ( !entry->IsResourceDescriptorFile() )
            {
                continue;
            }

            if ( filter( TryCast< Resource::ResourceDescriptor>( entry->m_pDataFile ) ) )
            {
                results.emplace_back( entry );
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
                    if ( !entry->IsResourceDescriptorFile() )
                    {
                        continue;
                    }

                    if ( filter( TryCast< Resource::ResourceDescriptor>( entry->m_pDataFile ) ) )
                    {
                        results.emplace_back( entry );
                    }
                }
            }
        }

        return results;
    }

    TVector<FileRegistry::FileInfo const*> FileRegistry::GetAllDataFileEntries( uint32_t extensionFourCC ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredDataFileType( extensionFourCC ) );

        TVector<FileInfo const*> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_dataFilesPerExtension.at( extensionFourCC );
        for ( auto const& entry : foundEntries )
        {
            results.emplace_back( entry );
        }

        return results;
    }

    bool FileRegistry::DoesFileExist( DataPath const& path ) const
    {
        EE_ASSERT( path.IsValid() );
        DataPath const actualPath = path.GetIntraFilePathParent();
        return m_filesPerPath.find( actualPath ) != m_filesPerPath.end();
    }

    TVector<ResourceID> FileRegistry::GetAllResourcesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<ResourceID> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( auto const& entry : foundEntries )
        {
            results.emplace_back( entry->m_dataPath );
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
                    results.emplace_back( entry->m_dataPath );
                }
            }
        }

        return results;
    }

    TVector<EE::ResourceID> FileRegistry::GetAllResourcesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<ResourceID> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( auto const& entry : foundEntries )
        {
            if ( !entry->IsResourceDescriptorFile() )
            {
                continue;
            }

            if ( filter( TryCast< Resource::ResourceDescriptor>( entry->m_pDataFile ) ) )
            {
                results.emplace_back( entry->m_dataPath );
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
                    if ( !entry->IsResourceDescriptorFile() )
                    {
                        continue;
                    }

                    if ( filter( TryCast< Resource::ResourceDescriptor>( entry->m_pDataFile ) ) )
                    {
                        results.emplace_back( entry->m_dataPath );
                    }
                }
            }
        }

        return results;
    }

    TVector<DataPath> FileRegistry::GetAllDependentResources( DataPath sourceFile ) const
    {
        TVector<DataPath> dependentResources;
        TVector<DataPath> compileDependencies;
        for ( auto const& fileEntryPair : m_filesPerPath )
        {
            compileDependencies.clear();

            auto pDescriptor = TryCast<Resource::ResourceDescriptor>( fileEntryPair.second->m_pDataFile );
            if ( pDescriptor != nullptr )
            {
                pDescriptor->GetCompileDependencies( compileDependencies );
                if ( VectorContains( compileDependencies, sourceFile ) )
                {
                    dependentResources.emplace_back( fileEntryPair.first );
                }
            }
        }

        return dependentResources;
    }

    //-------------------------------------------------------------------------

    FileRegistry::DirectoryInfo* FileRegistry::FindDirectory( FileSystem::Path const& dirPathToFind )
    {
        EE_ASSERT( dirPathToFind.IsDirectoryPath() );

        DirectoryInfo* pCurrentDir = &m_sourceDataDirectoryInfo;
        FileSystem::Path directoryPath = m_sourceDataDirPath;
        TInlineVector<String, 10> splitPath = dirPathToFind.Split();

        //-------------------------------------------------------------------------

        int32_t const pathDepth = (int32_t) splitPath.size();
        for ( int32_t i = m_dataDirectoryPathDepth + 1; i < pathDepth; i++ )
        {
            directoryPath.Append( splitPath[i] );

            String const intermediateName( splitPath[i] );
            auto searchPredicate = [&intermediateName] ( DirectoryInfo& dir ) { return dir.m_name.comparei( intermediateName ) == 0; };

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

    FileRegistry::DirectoryInfo* FileRegistry::FindOrCreateDirectory( FileSystem::Path const& dirPathToFind )
    {
        EE_ASSERT( dirPathToFind.IsDirectoryPath() );

        DirectoryInfo* pCurrentDir = &m_sourceDataDirectoryInfo;
        FileSystem::Path directoryPath = m_sourceDataDirPath;
        TInlineVector<String, 10> splitPath = dirPathToFind.Split();

        //-------------------------------------------------------------------------

        int32_t const pathDepth = (int32_t) splitPath.size();
        for ( int32_t i = m_dataDirectoryPathDepth + 1; i < pathDepth; i++ )
        {
            directoryPath.Append( splitPath[i] );

            String const intermediateName( splitPath[i] );
            auto searchPredicate = [&intermediateName] ( DirectoryInfo& dir ) { return dir.m_name.comparei( intermediateName ) == 0; };

            // Try to find an existing directory record
            auto itemIter = eastl::find_if( pCurrentDir->m_directories.begin(), pCurrentDir->m_directories.end(), searchPredicate );
            if ( itemIter != pCurrentDir->m_directories.end() )
            {
                pCurrentDir = itemIter;
            }
            else // Create new directory
            {
                auto& newDirectory = pCurrentDir->m_directories.emplace_back( DirectoryInfo() );
                newDirectory.m_name = splitPath[i];
                newDirectory.m_filePath = directoryPath;
                newDirectory.m_dataPath = DataPath::FromFileSystemPath( m_sourceDataDirPath, newDirectory.m_filePath );

                pCurrentDir = &newDirectory;
            }
        }

        //-------------------------------------------------------------------------

        return pCurrentDir;
    }

    bool FileRegistry::HasFileRecord( FileSystem::Path const& path ) const
    {
        DirectoryInfo const* pDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );

        int32_t const numFiles = (int32_t) pDirectory->m_files.size();
        for ( int32_t i = 0; i < numFiles; i++ )
        {
            if ( pDirectory->m_files[i]->m_filePath == path )
            {
                return true;
            }
        }

        return false;
    }

    FileRegistry::FileInfo* FileRegistry::AddFileRecord( FileSystem::Path const& path, bool shouldLoadDataFile )
    {
        auto const dataPath = DataPath::FromFileSystemPath( m_sourceDataDirPath, path );
        EE_ASSERT( dataPath.IsFilePath() );
        char const* pExtension = dataPath.GetExtension();

        // Create entry
        auto pNewEntry = EE::New<FileInfo>();
        pNewEntry->m_filePath = path;
        pNewEntry->m_dataPath = dataPath;
        pNewEntry->m_extensionFourCC = 0;
        pNewEntry->m_extension = pExtension ? pExtension : "";
        pNewEntry->m_fileType = FileType::Unknown;

        // Process extension
        if ( FourCC::IsValidLowercase( pNewEntry->m_extension.c_str() ) )
        {
            pNewEntry->m_extensionFourCC = FourCC::FromLowercaseString( pNewEntry->m_extension.c_str() );
        }

        // Data file
        if ( m_pTypeRegistry->IsRegisteredDataFileType( pNewEntry->m_extensionFourCC ) )
        {
            pNewEntry->m_fileType = FileType::DataFile;
        }

        // Resource
        ResourceTypeID resourceTypeID;
        if( pNewEntry->m_extensionFourCC != 0 )
        {
            resourceTypeID = ResourceTypeID( pNewEntry->m_extensionFourCC );
            if ( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) )
            {
                if ( EntityModel::IsResourceAnEntityDescriptor( resourceTypeID ) )
                {
                    pNewEntry->m_fileType = FileType::EntityDescriptor;
                }
                else if( VectorContains( m_resourceTypesWithDescriptors, resourceTypeID ) )
                {
                    pNewEntry->m_fileType = FileType::ResourceDescriptor;
                }
            }
        }

        // Add to directory list
        DirectoryInfo* pDirectory = FindOrCreateDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );
        pDirectory->m_files.emplace_back( pNewEntry );

        // Add to per-type lists
        if ( pNewEntry->IsResourceDescriptorFile() || pNewEntry->IsEntityDescriptorFile() )
        {
            m_resourcesPerType[resourceTypeID].emplace_back( pNewEntry );
        }
        else if ( pNewEntry->IsDataFile() )
        {
            m_dataFilesPerExtension[pNewEntry->m_extensionFourCC].emplace_back( pNewEntry );
        }

        // Add to file map
        m_filesPerPath[dataPath] = pNewEntry;

        // Load descriptor
        if ( shouldLoadDataFile )
        {
            if ( pNewEntry->IsResourceDescriptorFile() || pNewEntry->IsDataFile() )
            {
                pNewEntry->LoadDataFile( *m_pTypeRegistry );
            }
        }

        return pNewEntry;
    }

    void FileRegistry::RemoveFileRecord( FileSystem::Path const& path, bool fireDeletedEvent )
    {
        DirectoryInfo* pDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );

        int32_t const numFiles = (int32_t) pDirectory->m_files.size();
        for ( int32_t i = 0; i < numFiles; i++ )
        {
            if ( pDirectory->m_files[i]->m_filePath == path )
            {
                // Remove from file map
                auto fileMapIter = m_filesPerPath.find( pDirectory->m_files[i]->m_dataPath );
                if ( fileMapIter != m_filesPerPath.end() )
                {
                    m_filesPerPath.erase( fileMapIter );
                }

                // Remove from categorized resource lists
                if ( pDirectory->m_files[i]->IsResourceDescriptorFile() || pDirectory->m_files[i]->IsEntityDescriptorFile() )
                {
                    ResourceTypeID const typeID = ResourceTypeID( pDirectory->m_files[i]->m_extensionFourCC );
                    auto iter = m_resourcesPerType.find( typeID );
                    if ( iter != m_resourcesPerType.end() )
                    {
                        TVector<FileInfo*>& category = iter->second;
                        category.erase_first_unsorted( pDirectory->m_files[i] );
                    }
                }
                else if ( pDirectory->m_files[i]->IsDataFile() )
                {
                    auto iter = m_dataFilesPerExtension.find( pDirectory->m_files[i]->m_extensionFourCC );
                    if ( iter != m_dataFilesPerExtension.end() )
                    {
                        TVector<FileInfo*>& category = iter->second;
                        category.erase_first_unsorted( pDirectory->m_files[i] );
                    }
                }

                // Fire event
                if ( fireDeletedEvent )
                {
                    m_fileDeletedEvent.Execute( pDirectory->m_files[i]->m_dataPath );
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

    void FileRegistry::ProcessFileSystemChanges()
    {
        auto const& fsEvents = m_fileSystemWatcher.GetFileSystemChangeEvents();
        for ( auto const& fsEvent : fsEvents )
        {
            switch ( fsEvent.m_type )
            {
                case FileSystem::Watcher::Event::FileCreated:
                {
                    AddFileRecord( fsEvent.m_path, true );
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::FileDeleted:
                {
                    RemoveFileRecord( fsEvent.m_path );
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::FileRenamed:
                {
                    RemoveFileRecord( fsEvent.m_oldPath );

                    // Handle renaming over existing file
                    if ( HasFileRecord( fsEvent.m_path ) )
                    {
                        RemoveFileRecord( fsEvent.m_path, false );
                    }

                    AddFileRecord( fsEvent.m_path, true );
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::FileModified:
                {
                    DirectoryInfo* pDirectory = FindDirectory( fsEvent.m_path.GetParentDirectory() );
                    EE_ASSERT( pDirectory != nullptr );

                    int32_t const numFiles = (int32_t) pDirectory->m_files.size();
                    for ( int32_t i = 0; i < numFiles; i++ )
                    {
                        if ( pDirectory->m_files[i]->m_filePath == fsEvent.m_path )
                        {
                            if ( pDirectory->m_files[i]->IsResourceDescriptorFile() || pDirectory->m_files[i]->IsDataFile() )
                            {
                                pDirectory->m_files[i]->ReloadDataFile( *m_pTypeRegistry );
                            }

                            break;
                        }
                    }
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::DirectoryCreated:
                {
                    TVector<FileSystem::Path> foundPaths;
                    if ( !FileSystem::GetDirectoryContents( fsEvent.m_path, foundPaths, FileSystem::DirectoryReaderOutput::OnlyFiles, FileSystem::DirectoryReaderMode::Recursive ) )
                    {
                        EE_HALT();
                    }

                    // If this is an empty directory, add to the directory list
                    if ( foundPaths.empty() )
                    {
                        DirectoryInfo* pDirectory = FindOrCreateDirectory( fsEvent.m_path );
                        EE_ASSERT( pDirectory != nullptr );
                    }
                    else // Add file records (this will automatically create the directory record)
                    {
                        for ( auto const& filePath : foundPaths )
                        {
                            AddFileRecord( filePath, true );
                        }
                    }
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::DirectoryDeleted:
                {
                    auto pParentDirectory = FindDirectory( fsEvent.m_path.GetParentDirectory() );
                    EE_ASSERT( pParentDirectory != nullptr );

                    int32_t const numDirectories = (int32_t) pParentDirectory->m_directories.size();
                    for ( int32_t i = 0; i < numDirectories; i++ )
                    {
                        if ( pParentDirectory->m_directories[i].m_filePath == fsEvent.m_path )
                        {
                            // Delete all children and remove directory
                            pParentDirectory->m_directories[i].Clear();
                            pParentDirectory->m_directories.erase_unsorted( pParentDirectory->m_directories.begin() + i );
                            break;
                        }
                    }
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::DirectoryRenamed:
                {
                    EE_ASSERT( fsEvent.m_oldPath.IsDirectoryPath() );
                    EE_ASSERT( fsEvent.m_path.IsDirectoryPath() );

                    DirectoryInfo* pDirectory = nullptr;

                    // Check if the directory was also moved
                    FileSystem::Path const oldParentPath = fsEvent.m_oldPath.GetParentDirectory();
                    FileSystem::Path const newParentPath = fsEvent.m_path.GetParentDirectory();
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
                            if ( pOldParentDirectory->m_directories[i].m_filePath == fsEvent.m_oldPath )
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
                        pDirectory = FindDirectory( fsEvent.m_oldPath );
                    }

                    //-------------------------------------------------------------------------

                    EE_ASSERT( pDirectory != nullptr );
                    pDirectory->ChangePath( m_sourceDataDirPath, fsEvent.m_path );
                }
                break;

                //-------------------------------------------------------------------------

                case FileSystem::Watcher::Event::DirectoryModified:
                {
                    // Do Nothing
                }
                break;

                //-------------------------------------------------------------------------

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( m_fileCacheUpdatedEvent.HasBoundUsers() )
        {
            m_fileCacheUpdatedEvent.Execute();
        }
    }
}
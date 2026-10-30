#include "DataFileRegistry.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    DataFileRegistry::FileInfo::~FileInfo()
    {
        EE::Delete( m_pDataFile );
    }

    DataFileRegistry::FileInfo& DataFileRegistry::FileInfo::operator=( FileInfo&& rhs )
    {
        m_dataPath = rhs.m_dataPath;
        m_filePath = rhs.m_filePath;
        m_extension = rhs.m_extension;
        m_dataFileExtension = rhs.m_dataFileExtension;
        m_fileType = rhs.m_fileType;
        m_pDataFile = rhs.m_pDataFile;
        m_compileDependencies.swap( rhs.m_compileDependencies );
        m_installDependencies.swap( rhs.m_installDependencies );
        m_referencedPaths.swap( rhs.m_referencedPaths );

        rhs.m_pDataFile = nullptr;

        return *this;
    }

    DataFileRegistry::FileInfo& DataFileRegistry::FileInfo::operator=( FileInfo const& rhs )
    {
        m_dataPath = rhs.m_dataPath;
        m_filePath = rhs.m_filePath;
        m_extension = rhs.m_extension;
        m_dataFileExtension = rhs.m_dataFileExtension;
        m_fileType = rhs.m_fileType;

        return *this;
    }

    void DataFileRegistry::FileInfo::LoadDataFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log )
    {
        EE_ASSERT( IsResourceDescriptorFile() || IsDataFile() );
        EE_ASSERT( m_pDataFile == nullptr );
        EE_ASSERT( m_filePath.IsValid() );

        auto const result = IDataFile::TryReadFromFile( typeRegistry, log, m_filePath );
        m_pDataFile = result.m_pDataFile;
    }

    void DataFileRegistry::FileInfo::ReloadDataFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log )
    {
        EE::Delete( m_pDataFile );
        LoadDataFile( typeRegistry, log );
        ClearDependencyCache();
    }

    TInlineVector<ResourceID, 10> DataFileRegistry::FileInfo::GetCompileDependents( DataPath const& path ) const
    {
        TInlineVector<ResourceID, 10> deps;
        for ( auto const& cdep : m_compileDependencies )
        {
            if ( cdep.m_compileDependency == path )
            {
                deps.emplace_back( cdep.m_resourceID );
            }
        }

        return deps;
    }

    TInlineVector<ResourceID, 10> DataFileRegistry::FileInfo::GetInstallDependents( ResourceID const& resourceID ) const
    {
        TInlineVector<ResourceID, 10> deps;
        for ( auto const& idep : m_installDependencies )
        {
            if ( idep.m_installDependency == resourceID )
            {
                deps.emplace_back( idep.m_resourceID );
            }
        }

        return deps;
    }

    void DataFileRegistry::FileInfo::ClearDependencyCache()
    {
        m_compileDependencies.clear();
        m_installDependencies.clear();
        m_referencedPaths.clear();
    }

    void DataFileRegistry::FileInfo::BuildDependencyCache( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDataDirPath )
    {
        EE_ASSERT( m_pDataFile != nullptr );

        ClearDependencyCache();

        // Update all path references
        //-------------------------------------------------------------------------

        m_pDataFile->GetReferencedPaths( m_referencedPaths );

        if ( !IsResourceDescriptorFile() )
        {
            return;
        }

        // Ask descriptor for all dependencies
        //-------------------------------------------------------------------------

        auto pDescriptor = TryCast<Resource::ResourceDescriptor>( m_pDataFile );
        EE_ASSERT( pDescriptor != nullptr );

        TVector<Resource::CompileDependency> compileDependencies;
        TVector<ResourceID> installDependencies;

        auto AddDependencies = [this, &compileDependencies, &installDependencies] ( ResourceID const& dependentID )
        {
            for ( auto const& compileDep : compileDependencies )
            {
                m_compileDependencies.emplace_back( dependentID, compileDep );
            }

            for ( auto const& installDep : installDependencies )
            {
                m_installDependencies.emplace_back( dependentID, installDep );
            }
        };

        // Get all compile/install dependencies for main resource
        //-------------------------------------------------------------------------

        pDescriptor->GetCompileDependencies( typeRegistry, sourceDataDirPath, "", compileDependencies );
        pDescriptor->GetInstallDependencies( typeRegistry, sourceDataDirPath, "", installDependencies );

        AddDependencies( GetResourceID() );

        // Get all compile/install dependencies for sub-resources
        //-------------------------------------------------------------------------

        TVector<String> subResources;
        pDescriptor->GetAllSubResources( subResources );

        for ( String const& subResourceName : subResources )
        {
            compileDependencies.clear();
            installDependencies.clear();

            pDescriptor->GetCompileDependencies( typeRegistry, sourceDataDirPath, subResourceName, compileDependencies );
            pDescriptor->GetInstallDependencies( typeRegistry, sourceDataDirPath, subResourceName, installDependencies );

            DataPath subresourcePath = m_dataPath;
            subresourcePath.SetSubFilename( subResourceName );
            AddDependencies( ResourceID( subresourcePath ) );
        }

        // De-duplicate dependencies
        //-------------------------------------------------------------------------

        for ( int32_t i = int32_t( m_compileDependencies.size() ) - 1; i >= 0; i-- )
        {
            for ( int32_t j = i - 1; j >= 0; j-- )
            {
                if ( m_compileDependencies[i] == m_compileDependencies[j] )
                {
                    m_compileDependencies.erase_unsorted( m_compileDependencies.begin() + i );
                    break;
                }
            }
        }

        for ( int32_t i = int32_t( m_installDependencies.size() ) - 1; i >= 0; i-- )
        {
            for ( int32_t j = i - 1; j >= 0; j-- )
            {
                if ( m_installDependencies[i] == m_installDependencies[j] )
                {
                    m_installDependencies.erase_unsorted( m_installDependencies.begin() + i );
                    break;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    DataFileRegistry::DirectoryInfo::~DirectoryInfo()
    {
        EE_ASSERT( m_directories.empty() && m_files.empty() );
    }

    void DataFileRegistry::DirectoryInfo::ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath )
    {
        FileSystem::Path const oldPath = m_filePath;
        m_name = newPath.GetDirectoryName();
        m_filePath = newPath;
        m_dataPath = DataPath( m_filePath, rawResourceDirectoryPath );

        // Rename sub-directories
        //-------------------------------------------------------------------------

        for ( auto& pDirectory : m_directories )
        {
            FileSystem::Path const newSubdirectoryPath = newPath + pDirectory->m_name.c_str();
            pDirectory->ChangePath( rawResourceDirectoryPath, newSubdirectoryPath );
        }

        // Rename files
        //-------------------------------------------------------------------------

        for ( auto pRecord : m_files )
        {
            pRecord->m_filePath.ReplaceParentDirectory( newPath );
            pRecord->m_dataPath = DataPath( pRecord->m_filePath, rawResourceDirectoryPath );
        }
    }

    void DataFileRegistry::DirectoryInfo::Clear()
    {
        for ( auto& pDir : m_directories )
        {
            pDir->Clear();
            EE::Delete( pDir );
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

    void DataFileRegistry::DirectoryInfo::GetAllFiles( TVector<FileInfo const*>& files, bool recurseIntoChildDirectories ) const
    {
        for ( auto pFile : m_files )
        {
            files.emplace_back( pFile );
        }

        if ( recurseIntoChildDirectories )
        {
            for ( DirectoryInfo const* pDirectory : m_directories )
            {
                pDirectory->GetAllFiles( files, recurseIntoChildDirectories );
            }
        }
    }

    void DataFileRegistry::DirectoryInfo::GetAllResourceOrDataFiles( TVector<FileInfo const*>& files, bool recurseIntoChildDirectories ) const
    {
        for ( auto pFile : m_files )
        {
            if ( !pFile->IsDataFile() && !pFile->IsResourceDescriptorFile() )
            {
                continue;
            }

            files.emplace_back( pFile );
        }

        if ( recurseIntoChildDirectories )
        {
            for ( DirectoryInfo const* pDirectory : m_directories )
            {
                pDirectory->GetAllResourceOrDataFiles( files, recurseIntoChildDirectories );
            }
        }
    }

    //-------------------------------------------------------------------------

    DataFileRegistry::~DataFileRegistry()
    {
        EE_ASSERT( m_state == DatabaseState::Empty );
        EE_ASSERT( m_sourceDataDirectoryInfo.IsEmpty() && m_resourcesPerType.empty() && m_filesPerPath.empty() );
    }

    float DataFileRegistry::GetCacheBuildProgress() const
    {
        EE_ASSERT( IsBuildingCaches() );

        if ( m_totalItemsToProcess == 0 )
        {
            return 0.0f;
        }

        float const progress = float( m_numItemsProcessed ) / m_totalItemsToProcess;
        EE_ASSERT( Math::IsFinite( progress ) );
        return progress;
    }

    //-------------------------------------------------------------------------

    void DataFileRegistry::Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, TaskSystem* pTaskSystem, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath )
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
            auto pDescriptorDefaultInstance = pTypeInfo->GetDefaultInstance<Resource::ResourceDescriptor>();
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

    void DataFileRegistry::Shutdown()
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

    bool DataFileRegistry::Update()
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

                    m_fileCacheUpdatedEvent.Execute();

                    if ( m_dataFilesToLoad.empty() )
                    {
                        m_state = DatabaseState::Ready;
                    }
                    else
                    {
                        StartDataFileCacheBuild();
                    }
                }
                else if ( m_state == DatabaseState::BuildingDataFileCache )
                {
                    EE_ASSERT( m_numItemsProcessed == m_totalItemsToProcess );
                    m_dataFilesToLoad.clear();
                    StartDependencyCacheBuild();
                }
                else if ( m_state == DatabaseState::BuildingDependencyCache )
                {
                    EE_ASSERT( m_numItemsProcessed == m_totalItemsToProcess );
                    m_state = DatabaseState::Ready;
                    m_dependencyCacheUpdatedEvent.Execute();
                }
                else
                {
                    EE_UNREACHABLE_CODE();
                }
            }
        }

        // Update file watcher
        //-------------------------------------------------------------------------
        // Do not update or run the file system watcher when we have any async task running that is either reading or writing the caches

        if ( m_pAsyncTask == nullptr )
        {
            if ( m_fileSystemWatcher.Update() )
            {
                ProcessFileSystemChanges();
                return true;
            }
        }

        //-------------------------------------------------------------------------

        return false;
    }

    //-------------------------------------------------------------------------

    void DataFileRegistry::RequestRebuild()
    {
        CancelDatabaseBuild();
        ClearDatabase();
        StartFilesystemCacheBuild();
    }

    void DataFileRegistry::ClearDatabase()
    {
        m_resourcesPerType.clear();
        m_filesPerPath.clear();
        m_sourceDataDirectoryInfo.Clear();
        m_dataFilesToLoad.empty();
        m_numItemsProcessed = m_totalItemsToProcess = 0;
        m_state = DatabaseState::Empty;
    }

    void DataFileRegistry::CancelDatabaseBuild()
    {
        if ( m_pAsyncTask != nullptr )
        {
            m_cancelActiveTask = true;
            m_pTaskSystem->WaitForTask( m_pAsyncTask );
            EE::Delete( m_pAsyncTask );
            ClearDatabase();
            m_cancelActiveTask = false;
            m_numItemsProcessed = 0;
            m_totalItemsToProcess = 0;
        }
    }

    void DataFileRegistry::HandleMassiveFileSystemChangeDetected()
    {
        RequestRebuild();
    }

    void DataFileRegistry::StartFilesystemCacheBuild()
    {
        EE_ASSERT( m_state == DatabaseState::Empty );
        EE_ASSERT( m_pAsyncTask == nullptr );
        EE_ASSERT( m_dataFilesToLoad.empty() );

        //-------------------------------------------------------------------------

        auto BuildFileSystemCache = [this] ( TaskSetPartition range, uint32_t threadnum )
        {
            Threading::ScopeLockWrite const sw( m_mutex );

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
            m_sourceDataDirectoryInfo.m_dataPath = DataPath( m_sourceDataDirPath, m_sourceDataDirPath );

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
        m_totalItemsToProcess = 1;

        m_state = DatabaseState::BuildingFileSystemCache;
        m_pAsyncTask = EE::New<AsyncTask>( BuildFileSystemCache );
        m_pTaskSystem->ScheduleTask( m_pAsyncTask );
    }

    void DataFileRegistry::StartDataFileCacheBuild()
    {
        EE_ASSERT( m_state == DatabaseState::BuildingFileSystemCache );
        EE_ASSERT( m_pAsyncTask == nullptr );

        //-------------------------------------------------------------------------

        auto BuildDescriptorCache = [this] ( TaskSetPartition range, uint32_t threadnum )
        {
            Threading::ScopeLockWrite const sw( m_mutex );

            Log tempLog;

            for ( uint32_t i = range.start; i < range.end; i++ )
            {
                m_dataFilesToLoad[i]->LoadDataFile( *m_pTypeRegistry, tempLog );
                m_dataFilesToLoad[i] = nullptr;
                m_numItemsProcessed++;
            }
        };

        //-------------------------------------------------------------------------

        m_numItemsProcessed = 0;
        m_totalItemsToProcess = (int32_t) m_dataFilesToLoad.size();

        EE_ASSERT( m_totalItemsToProcess > 0 );

        m_state = DatabaseState::BuildingDataFileCache;
        m_pAsyncTask = EE::New<AsyncTask>( m_totalItemsToProcess, BuildDescriptorCache );
        m_pTaskSystem->ScheduleTask( m_pAsyncTask );
    }

    void DataFileRegistry::StartDependencyCacheBuild()
    {
        EE_ASSERT( m_state == DatabaseState::BuildingDataFileCache || m_state == DatabaseState::Ready );
        EE_ASSERT( m_pAsyncTask == nullptr );

        //-------------------------------------------------------------------------

        auto BuildDependencyCache = [this] ( TaskSetPartition range, uint32_t threadnum )
        {
            Threading::ScopeLockWrite const sw( m_mutex );

            auto iter = m_filesPerPath.begin();
            for ( uint32_t i = 0; i < range.start; i++ )
            {
                iter++;
            }

            for ( uint32_t i = range.start; i < range.end; i++ )
            {
                auto const& fileEntryPair = *iter;
                iter++;

                FileInfo* pFileInfo = fileEntryPair.second;
                if ( pFileInfo->HasLoadedDataFile() )
                {
                    pFileInfo->BuildDependencyCache( *m_pTypeRegistry, m_sourceDataDirPath );
                }

                m_numItemsProcessed++;
            }
        };

        //-------------------------------------------------------------------------

        m_numItemsProcessed = 0;
        m_totalItemsToProcess = (int32_t) m_filesPerPath.size();

        if ( m_totalItemsToProcess == 0 )
        {
            m_dependencyCacheUpdatedEvent.Execute();
            m_state = DatabaseState::Ready;
        }
        else
        {
            m_state = DatabaseState::BuildingDependencyCache;
            m_pAsyncTask = EE::New<AsyncTask>( m_totalItemsToProcess, BuildDependencyCache );
            m_pTaskSystem->ScheduleTask( m_pAsyncTask );
        }
    }

    //-------------------------------------------------------------------------

    DataFileRegistry::FileInfo const* DataFileRegistry::GetFileEntry( DataPath const& dataPath ) const
    {
        auto fileEntryIter = m_filesPerPath.find( dataPath );;
        if ( fileEntryIter != m_filesPerPath.end() )
        {
            return fileEntryIter->second;
        }

        return  nullptr;
    }

    TVector<DataFileRegistry::FileInfo const*> DataFileRegistry::GetAllResourceFileEntries( ResourceTypeID resourceTypeID, bool includeDerivedTypes ) const
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

    TVector<DataFileRegistry::FileInfo const*> DataFileRegistry::GetAllResourceFileEntriesFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes /*= false */ ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<FileInfo const*> results;

        //-------------------------------------------------------------------------

        TVector<FileInfo*> const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( FileInfo const* pEntry : foundEntries )
        {
            if ( !pEntry->IsResourceDescriptorFile() )
            {
                continue;
            }

            if ( !pEntry->HasLoadedDataFile() )
            {
                continue;
            }

            if ( filter( Cast<Resource::ResourceDescriptor>( pEntry->m_pDataFile ) ) )
            {
                results.emplace_back( pEntry );
            }
        }

        //-------------------------------------------------------------------------

        if ( includeDerivedTypes )
        {
            auto const derivedResourceTypeIDs = m_pTypeRegistry->GetAllDerivedResourceTypes( resourceTypeID );
            for ( ResourceTypeID derivedResourceTypeID : derivedResourceTypeIDs )
            {
                TVector<FileInfo*> const& derivedResources = m_resourcesPerType.at( derivedResourceTypeID );
                for ( FileInfo const* pEntry : derivedResources )
                {
                    if ( !pEntry->IsResourceDescriptorFile() )
                    {
                        continue;
                    }

                    if ( !pEntry->HasLoadedDataFile() )
                    {
                        continue;
                    }

                    if ( filter( Cast<Resource::ResourceDescriptor>( pEntry->m_pDataFile ) ) )
                    {
                        results.emplace_back( pEntry );
                    }
                }
            }
        }

        return results;
    }

    TVector<DataFileRegistry::FileInfo const*> DataFileRegistry::GetAllDataFileEntries( DataFileExtension extension ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredDataFileType( extension ) );

        TVector<FileInfo const*> results;

        //-------------------------------------------------------------------------

        auto const& foundEntries = m_dataFilesPerExtension.at( extension );
        for ( auto const& entry : foundEntries )
        {
            results.emplace_back( entry );
        }

        return results;
    }

    TVector<DataFileRegistry::FileInfo const*> DataFileRegistry::GetAllDataFileEntries() const
    {
        TVector<FileInfo const*> results;

        for ( auto const& dpe : m_dataFilesPerExtension )
        {
            for ( auto const& entry : dpe.second )
            {
                results.emplace_back( entry );
            }
        }

        return results;
    }

    bool DataFileRegistry::DoesFileExist( DataPath const& path ) const
    {
        EE_ASSERT( path.IsValid() );

        if ( path.HasSubFilename() )
        {
            DataPath const parentPath = path.GetPathWithoutSubFilename();
            return m_filesPerPath.find( parentPath ) != m_filesPerPath.end();
        }
        else
        {
            return m_filesPerPath.find( path ) != m_filesPerPath.end();
        }
    }

    TVector<ResourceID> DataFileRegistry::GetAllResourcesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes ) const
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

    TVector<EE::ResourceID> DataFileRegistry::GetAllResourcesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes ) const
    {
        EE_ASSERT( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

        TVector<ResourceID> results;

        //-------------------------------------------------------------------------

        TVector<FileInfo*> const& foundEntries = m_resourcesPerType.at( resourceTypeID );
        for ( FileInfo const* pEntry : foundEntries )
        {
            if ( !pEntry->IsResourceDescriptorFile() )
            {
                continue;
            }

            if ( !pEntry->HasLoadedDataFile() )
            {
                continue;
            }

            if ( filter( Cast<Resource::ResourceDescriptor>( pEntry->m_pDataFile ) ) )
            {
                results.emplace_back( pEntry->m_dataPath );
            }
        }

        //-------------------------------------------------------------------------

        if ( includeDerivedTypes )
        {
            auto const derivedResourceTypeIDs = m_pTypeRegistry->GetAllDerivedResourceTypes( resourceTypeID );
            for ( ResourceTypeID derivedResourceTypeID : derivedResourceTypeIDs )
            {
                TVector<FileInfo*> const& derivedResources = m_resourcesPerType.at( derivedResourceTypeID );
                for ( FileInfo const* pEntry : derivedResources )
                {
                    if ( !pEntry->IsResourceDescriptorFile() )
                    {
                        continue;
                    }

                    if ( !pEntry->HasLoadedDataFile() )
                    {
                        continue;
                    }

                    if ( filter( Cast<Resource::ResourceDescriptor>( pEntry->m_pDataFile ) ) )
                    {
                        results.emplace_back( pEntry->m_dataPath );
                    }
                }
            }
        }

        return results;
    }

    void DataFileRegistry::GetAllResourcesThatDependOnFile( DataPath const& sourceFile, TVector<DataPath>& outCompileDependents, TVector<ResourceID>* pOutInstallDependents ) const
    {
        EE_ASSERT( IsDependencyCacheBuilt() );

        outCompileDependents.clear();

        bool const shouldReturnInstallDependencies = pOutInstallDependents != nullptr;
        if ( shouldReturnInstallDependencies )
        {
            pOutInstallDependents->clear();
        }

        if ( !sourceFile.IsValid() )
        {
            return;
        }

        // Search all descriptors for dependencies
        //-------------------------------------------------------------------------

        Threading::ScopeLockRead const sr( m_mutex );

        ResourceID const sourceFileResourceID( sourceFile );

        // Check each descriptor if it depends on the specified source file 
        for ( auto const& iter : m_filesPerPath )
        {
            FileInfo const* pFileInfo = iter.second;

            if ( !pFileInfo->IsResourceDescriptorFile() )
            {
                continue;
            }

            for ( ResourceID const& ID : pFileInfo->GetCompileDependents( sourceFile ) )
            {
                outCompileDependents.emplace_back( ID.GetDataPath() );
            }

            if ( shouldReturnInstallDependencies )
            {
                for ( ResourceID const& ID : pFileInfo->GetInstallDependents( sourceFileResourceID ) )
                {
                    pOutInstallDependents->emplace_back( ID );
                }
            }
        }
    }

    void DataFileRegistry::GetAllFilesThatReferenceFile( DataPath const& sourceFile, TVector<DataFileRegistry::FileInfo const*>& outReferencers ) const
    {
        EE_ASSERT( IsDependencyCacheBuilt() );

        outReferencers.clear();

        if ( !sourceFile.IsValid() )
        {
            return;
        }

        // Search all data files for dependencies
        //-------------------------------------------------------------------------

        Threading::ScopeLockRead const sr( m_mutex );
        for ( auto const& iter : m_filesPerPath )
        {
            FileInfo const* pFileInfo = iter.second;
            if ( VectorContains( pFileInfo->m_referencedPaths, sourceFile ) )
            {
                outReferencers.emplace_back( pFileInfo );
            }
        }
    }

    //-------------------------------------------------------------------------

    DataFileRegistry::DirectoryInfo* DataFileRegistry::FindDirectory( FileSystem::Path const& dirPathToFind )
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
            auto searchPredicate = [&intermediateName] ( DirectoryInfo const* pDir ) { return pDir->m_name.comparei( intermediateName ) == 0; };

            auto itemIter = eastl::find_if( pCurrentDir->m_directories.begin(), pCurrentDir->m_directories.end(), searchPredicate );
            if ( itemIter != pCurrentDir->m_directories.end() )
            {
                pCurrentDir = *itemIter;
            }
            else
            {
                return nullptr;
            }
        }

        //-------------------------------------------------------------------------

        return pCurrentDir;
    }

    DataFileRegistry::DirectoryInfo* DataFileRegistry::FindOrCreateDirectory( FileSystem::Path const& dirPathToFind )
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
            auto searchPredicate = [&intermediateName] ( DirectoryInfo const* pDir ) { return pDir->m_name.comparei( intermediateName ) == 0; };

            // Try to find an existing directory record
            auto itemIter = eastl::find_if( pCurrentDir->m_directories.begin(), pCurrentDir->m_directories.end(), searchPredicate );
            if ( itemIter != pCurrentDir->m_directories.end() )
            {
                pCurrentDir = *itemIter;
            }
            else // Create new directory
            {
                DirectoryInfo* pNewDirectory = pCurrentDir->m_directories.emplace_back( EE::New<DirectoryInfo>() );
                pNewDirectory->m_name = splitPath[i];
                pNewDirectory->m_filePath = directoryPath;
                pNewDirectory->m_dataPath = DataPath( pNewDirectory->m_filePath, m_sourceDataDirPath );

                pCurrentDir = pNewDirectory;
            }
        }

        //-------------------------------------------------------------------------

        return pCurrentDir;
    }

    void DataFileRegistry::RemoveDirectory( FileSystem::Path const& dirPath )
    {
        DirectoryInfo* pParentDirectory = FindDirectory( dirPath.GetParentDirectory() );
        EE_ASSERT( pParentDirectory != nullptr );

        int32_t const numDirectories = (int32_t) pParentDirectory->m_directories.size();
        int32_t directoryIdx = InvalidIndex;
        for ( int32_t i = 0; i < numDirectories; i++ )
        {
            if ( pParentDirectory->m_directories[i]->m_filePath == dirPath )
            {
                directoryIdx = i;
                break;
            }
        }

        EE_ASSERT( directoryIdx != InvalidIndex );
        RemoveDirectory( pParentDirectory, directoryIdx );
    }

    void DataFileRegistry::RemoveDirectory( DirectoryInfo* pParentDirectory, int32_t directoryIdx )
    {
        EE_ASSERT( pParentDirectory != nullptr );

        DirectoryInfo* pDirectory = pParentDirectory->m_directories[directoryIdx];

        // Remove all sub-directories
        while ( !pDirectory->m_directories.empty() )
        {
            RemoveDirectory( pDirectory, 0 );
        }

        // Remove all files
        RemoveAllFilesFromDirectory( pDirectory );

        // Remove directory
        EE::Delete( pDirectory );
        pParentDirectory->m_directories.erase( pParentDirectory->m_directories.begin() + directoryIdx );
    }

    void DataFileRegistry::RemoveAllFilesFromDirectory( DirectoryInfo* pDirectory, bool fireDeletedEvent )
    {
        EE_ASSERT( pDirectory != nullptr );

        while ( !pDirectory->m_files.empty() )
        {
            RemoveFileRecord( pDirectory, 0, fireDeletedEvent );
        }
    }

    bool DataFileRegistry::HasFileRecord( FileSystem::Path const& path ) const
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

    DataFileRegistry::FileInfo* DataFileRegistry::AddFileRecord( FileSystem::Path const& path, bool shouldLoadDataFile )
    {
        auto const dataPath = DataPath( path, m_sourceDataDirPath );
        EE_ASSERT( dataPath.IsFilePath() );

        // Create entry
        auto pNewEntry = EE::New<FileInfo>();
        pNewEntry->m_filePath = path;
        pNewEntry->m_dataPath = dataPath;
        pNewEntry->m_extension = dataPath.GetExtension();
        pNewEntry->m_dataFileExtension = DataFileExtension( pNewEntry->m_extension );
        pNewEntry->m_fileType = FileType::Unknown;

        // Data file
        if ( m_pTypeRegistry->IsRegisteredDataFileType( pNewEntry->m_dataFileExtension ) )
        {
            pNewEntry->m_fileType = FileType::DataFile;
        }

        // Resource
        ResourceTypeID resourceTypeID;
        if( pNewEntry->m_dataFileExtension.IsValid() )
        {
            resourceTypeID = ResourceTypeID( pNewEntry->m_dataFileExtension );
            if ( m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) )
            {
                if( VectorContains( m_resourceTypesWithDescriptors, resourceTypeID ) )
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
        if ( pNewEntry->IsResourceDescriptorFile() )
        {
            m_resourcesPerType[resourceTypeID].emplace_back( pNewEntry );
        }
        else if ( pNewEntry->IsDataFile() )
        {
            m_dataFilesPerExtension[pNewEntry->m_dataFileExtension].emplace_back( pNewEntry );
        }

        // Add to file map
        m_filesPerPath[dataPath] = pNewEntry;

        // Load descriptor
        if ( shouldLoadDataFile )
        {
            if ( pNewEntry->IsResourceDescriptorFile() || pNewEntry->IsDataFile() )
            {
                Log log;
                pNewEntry->LoadDataFile( *m_pTypeRegistry, log );
            }
        }

        return pNewEntry;
    }

    void DataFileRegistry::RemoveFileRecord( FileSystem::Path const& path, bool fireDeletedEvent )
    {
        DirectoryInfo* pDirectory = FindDirectory( path.GetParentDirectory() );
        EE_ASSERT( pDirectory != nullptr );

        int32_t const numFiles = (int32_t) pDirectory->m_files.size();
        for ( int32_t i = 0; i < numFiles; i++ )
        {
            if ( pDirectory->m_files[i]->m_filePath == path )
            {
                RemoveFileRecord( pDirectory, i );
                return;
            }
        }
    }

    void DataFileRegistry::RemoveFileRecord( DirectoryInfo* pDirectory, int32_t fileIdx, bool fireDeletedEvent )
    {
        EE_ASSERT( pDirectory != nullptr );

        // Remove from file map
        auto fileMapIter = m_filesPerPath.find( pDirectory->m_files[fileIdx]->m_dataPath );
        if ( fileMapIter != m_filesPerPath.end() )
        {
            m_filesPerPath.erase( fileMapIter );
        }

        // Remove from categorized resource lists
        if ( pDirectory->m_files[fileIdx]->IsResourceDescriptorFile() )
        {
            ResourceTypeID const typeID = ResourceTypeID( pDirectory->m_files[fileIdx]->m_dataFileExtension );
            auto iter = m_resourcesPerType.find( typeID );
            if ( iter != m_resourcesPerType.end() )
            {
                TVector<FileInfo*>& category = iter->second;
                category.erase_first_unsorted( pDirectory->m_files[fileIdx] );
            }
        }
        else if ( pDirectory->m_files[fileIdx]->IsDataFile() )
        {
            auto iter = m_dataFilesPerExtension.find( pDirectory->m_files[fileIdx]->m_dataFileExtension );
            if ( iter != m_dataFilesPerExtension.end() )
            {
                TVector<FileInfo*>& category = iter->second;
                category.erase_first_unsorted( pDirectory->m_files[fileIdx] );
            }
        }

        // Fire event
        if ( fireDeletedEvent )
        {
            m_fileDeletedEvent.Execute( pDirectory->m_files[fileIdx]->m_dataPath );
        }

        // Destroy record
        EE::Delete( pDirectory->m_files[fileIdx] );
        pDirectory->m_files.erase_unsorted( pDirectory->m_files.begin() + fileIdx );
    }

    // Watcher Events
    //-------------------------------------------------------------------------

    void DataFileRegistry::ProcessFileSystemChanges()
    {
        EE_ASSERT( m_pAsyncTask == nullptr );

        auto const& fsEvents = m_fileSystemWatcher.GetFileSystemChangeEvents();
        if ( fsEvents.empty() )
        {
            return;
        }

        // Process events
        //-------------------------------------------------------------------------

        bool rebuildDependencyCache = false;
        bool notifyDependencyCacheUpdated = false;

        {
            Threading::ScopeLockWrite const sw( m_mutex );

            for ( auto const& fsEvent : fsEvents )
            {
                switch ( fsEvent.m_type )
                {
                    case FileSystem::Watcher::Event::FileCreated:
                    {
                        auto pFile = AddFileRecord( fsEvent.m_path, true );
                        if ( pFile->HasLoadedDataFile() )
                        {
                            pFile->BuildDependencyCache( *m_pTypeRegistry, m_sourceDataDirPath );
                            notifyDependencyCacheUpdated = true;
                        }
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case FileSystem::Watcher::Event::FileDeleted:
                    {
                        RemoveFileRecord( fsEvent.m_path );
                        rebuildDependencyCache = true;
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
                        rebuildDependencyCache = true;
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
                            auto pFile = pDirectory->m_files[i];
                            if ( pFile->m_filePath == fsEvent.m_path )
                            {
                                if ( pFile->IsResourceDescriptorFile() || pFile->IsDataFile() )
                                {
                                    Log log;
                                    pFile->ReloadDataFile( *m_pTypeRegistry, log );

                                    if ( pFile->HasLoadedDataFile() )
                                    {
                                        pFile->BuildDependencyCache( *m_pTypeRegistry, m_sourceDataDirPath );
                                        notifyDependencyCacheUpdated = true;
                                    }
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

                        rebuildDependencyCache = true;
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case FileSystem::Watcher::Event::DirectoryDeleted:
                    {
                        if ( FindDirectory( fsEvent.m_path ) != nullptr )
                        {
                            RemoveDirectory( fsEvent.m_path );
                            rebuildDependencyCache = true;
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
                                if ( pOldParentDirectory->m_directories[i]->m_filePath == fsEvent.m_oldPath )
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

                            pDirectory = pNewParentDirectory->m_directories.back();
                        }
                        else
                        {
                            pDirectory = FindDirectory( fsEvent.m_oldPath );
                        }

                        //-------------------------------------------------------------------------

                        EE_ASSERT( pDirectory != nullptr );
                        pDirectory->ChangePath( m_sourceDataDirPath, fsEvent.m_path );

                        rebuildDependencyCache = true;
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
        }

        // Dispatch Notifications
        //-------------------------------------------------------------------------

        m_fileCacheUpdatedEvent.Execute();

        // Rebuild dependency cache
        //-------------------------------------------------------------------------

        if ( rebuildDependencyCache )
        {
            StartDependencyCacheBuild();
        }
        else if ( notifyDependencyCacheUpdated )
        {
            m_dependencyCacheUpdatedEvent.Execute();
        }
    }
}
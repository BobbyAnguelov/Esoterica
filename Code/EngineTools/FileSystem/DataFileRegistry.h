#pragma once
#include "FileSystemWatcher.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Types/Event.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Threading/Threading.h"
#include "Base/Types/Function.h"
#include "Base/FileSystem/FileSystemExtension.h"

//-------------------------------------------------------------------------

namespace EE
{
    class TaskSystem;
    class Log;
    struct IDataFile;
    namespace TypeSystem { class TypeRegistry; }

    namespace Resource
    {
        struct ResourceDescriptor;
        struct CompileDependency;
    }
}

//-------------------------------------------------------------------------

namespace EE
{
    // Maintains a DB of all source resources in the source data folder
    //-------------------------------------------------------------------------
    // The way we load descriptors is pretty naive and can definitely be improved

    class EE_ENGINETOOLS_API DataFileRegistry final
    {
    public:

        enum class DatabaseState
        {
            Empty,
            BuildingFileSystemCache,
            BuildingDataFileCache,
            BuildingDependencyCache,
            Ready
        };

        enum class FileType
        {
            Unknown,
            DataFile,
            ResourceDescriptor,
        };

        struct CompileDependencyInfo
        {
            inline bool operator==( CompileDependencyInfo const& rhs ) const { return m_resourceID == rhs.m_resourceID && m_compileDependency == rhs.m_compileDependency; }

            ResourceID                          m_resourceID; // The resource that depends on this compile dependency
            DataPath                            m_compileDependency;
        };

        struct InstallDependencyInfo
        {
            inline bool operator==( InstallDependencyInfo const& rhs ) const { return m_resourceID == rhs.m_resourceID && m_installDependency == rhs.m_installDependency; }

            ResourceID                          m_resourceID; // The resource that depends on this install dependency
            ResourceID                          m_installDependency;
        };

        struct FileInfo
        {
            friend class DataFileRegistry;

        public:

            FileInfo() = default;
            ~FileInfo();

            // Custom copying - does not copy datafile ptr!
            FileInfo( FileInfo const& rhs ) { operator=( rhs ); }
            FileInfo& operator=( FileInfo const& rhs );

            // Custom move - transfers datafile ptr!
            FileInfo( FileInfo&& rhs ) { operator=( eastl::move( rhs ) ); }
            FileInfo& operator=( FileInfo&& rhs );

            // Unknown/Raw files
            inline bool IsUnknownFileType() const { return m_fileType == FileType::Unknown; }

            // Data file
            inline bool IsDataFile() const { return m_fileType == FileType::DataFile; }
            inline bool HasLoadedDataFile() const { return m_pDataFile != nullptr; }
            void LoadDataFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log );
            void ReloadDataFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log );

            // Descriptor
            inline bool IsResourceDescriptorFile() const { return m_fileType == FileType::ResourceDescriptor; }
            inline bool HasLoadedDescriptor() const { return HasLoadedDataFile(); }

            // Helpers
            inline ResourceTypeID GetResourceTypeID() const
            {
                EE_ASSERT( IsResourceDescriptorFile() );
                EE_ASSERT( m_dataFileExtension.IsValid() );
                return ResourceTypeID( m_dataFileExtension );
            }

            inline ResourceID GetResourceID() const
            {
                EE_ASSERT( IsResourceDescriptorFile() );
                return ResourceID( m_dataPath );
            }

            // Get any resource/sub-resource IDs generated from this file that have a compile dependency on the specified path
            TInlineVector<ResourceID, 10> GetCompileDependents( DataPath const& path ) const;

            // Get any resource/sub-resource IDs generated from this file that have a install dependency on the specified resource ID
            TInlineVector<ResourceID, 10> GetInstallDependents( ResourceID const& resourceID ) const;

        private:

            void ClearDependencyCache();
            void BuildDependencyCache( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDataDirPath );

        public:

            DataPath                                                m_dataPath;
            FileSystem::Path                                        m_filePath;
            FileSystem::Extension                                   m_extension;
            DataFileExtension                                       m_dataFileExtension;
            FileType                                                m_fileType = FileType::Unknown;
            IDataFile*                                              m_pDataFile = nullptr;

            TVector<CompileDependencyInfo>                          m_compileDependencies;
            TVector<InstallDependencyInfo>                          m_installDependencies;
            TVector<DataPath>                                       m_referencedPaths;
        };

        struct DirectoryInfo
        {
            friend class DataFileRegistry;

        public:

            DirectoryInfo() = default;
            ~DirectoryInfo();

            inline bool IsEmpty() const { return m_directories.empty() && m_files.empty(); }
            void ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath );

            // Delete all file entries and sub directories
            void Clear();

            void GetAllFiles( TVector<FileInfo const*>& files, bool recurseIntoChildDirectories = false ) const;

            void GetAllResourceOrDataFiles( TVector<FileInfo const*>& files, bool recurseIntoChildDirectories = false ) const;

        private:

            DirectoryInfo( DirectoryInfo&& ) = delete;
            DirectoryInfo( DirectoryInfo const& ) = delete;
            DirectoryInfo& operator=( DirectoryInfo&& ) = delete;
            DirectoryInfo& operator=( DirectoryInfo const& ) = delete;

        public:

            String                                                  m_name;
            FileSystem::Path                                        m_filePath;
            DataPath                                                m_dataPath;
            TVector<DirectoryInfo*>                                 m_directories;
            TVector<FileInfo*>                                      m_files;
        };

    public:

        ~DataFileRegistry();

        inline bool WasInitialized() const { return m_pTypeRegistry != nullptr; }
        void Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, TaskSystem* pTaskSystem, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath );
        void Shutdown();

        // Process any filesystem updates, returns true if any changes were detected!
        bool Update();

        // Database State
        //-------------------------------------------------------------------------

        // Request a full rebuild of the resource DB, this is currently needed only since we somehow fail to track newly created assets
        void RequestRebuild();

        // Get the state of the database
        DatabaseState GetState() const { return m_state; }

        // Did we finish building the file system cache?
        bool IsFileSystemCacheBuilt() const { return m_state > DatabaseState::BuildingFileSystemCache; }

        // Did we finish building the descriptor cache
        bool IsDataFileCacheBuilt() const { return m_state > DatabaseState::BuildingDataFileCache; }

        // Did we finish building the dependency cache
        bool IsDependencyCacheBuilt() const { return m_state > DatabaseState::BuildingDependencyCache; }

        // Are we currently rebuilding the DB?
        bool IsBuildingCaches() const { return m_state > DatabaseState::Empty && m_state < DatabaseState::Ready; }

        // Get the current progress where 0 mean no progress and 1.0f means complete!
        // Progress is per state i.e. progress will reset when we start building the descriptor cache
        float GetCacheBuildProgress() const;

        // Event that fires whenever the file system cache is updated
        TEventHandle<> OnFileSystemCacheUpdated() const { return m_fileCacheUpdatedEvent; }

        // Event that fires whenever the dependency cache is updated
        TEventHandle<> OnDependencyCacheUpdated() const { return m_dependencyCacheUpdatedEvent; }

        // General Info
        //-------------------------------------------------------------------------

        // Get the path for the raw resources
        inline FileSystem::Path const& GetSourceDataDirectoryPath() const { return m_sourceDataDirPath; }

        // Get the path for the compiled resources
        inline FileSystem::Path const& GetCompiledResourceDirectoryPath() const { return m_compiledResourceDirPath; }

        // Get a structured representation of all known raw resources
        inline DirectoryInfo const* GetRawResourceDirectoryEntry() const { return &m_sourceDataDirectoryInfo; }

        // Try to get the directory info for a given path
        inline DirectoryInfo const* FindDirectoryEntry( FileSystem::Path const& path ) const 
        {
            if ( path.IsValid() && path.IsDirectoryPath() )
            {
                return FindDirectory( path );
            }

            return nullptr;
        }

        // Try to get the directory info for a given path
        inline DirectoryInfo const* FindDirectoryEntry( DataPath const& dataPath ) const 
        {
            if ( dataPath.IsValid() && dataPath.IsDirectoryPath() )
            {
                return FindDirectory( dataPath.GetFileSystemPath( m_sourceDataDirPath ) );
            }

            return nullptr;
        }

        // File Info
        //-------------------------------------------------------------------------

        // Try to get an entry for a data path
        FileInfo const* GetFileEntry( DataPath const& dataPath ) const;

        // Try to get an entry for a resource ID
        EE_FORCE_INLINE FileInfo const* GetFileEntry( ResourceID const& resourceID ) const { return GetFileEntry( resourceID.GetDataPath() ); }

        // Get a list of all known resource file entries of the specified type
        TVector<FileInfo const*> GetAllResourceFileEntries( ResourceTypeID resourceTypeID, bool includeDerivedTypes = false ) const;

        // Get a list of all known resource of the specified type
        TVector<FileInfo const*> GetAllResourceFileEntriesFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes = false ) const;

        // Get a list of all known data files for a given extension
        TVector<FileInfo const*> GetAllDataFileEntries() const;

        // Get a list of all known data files for a given extension
        TVector<FileInfo const*> GetAllDataFileEntries( DataFileExtension extension ) const;

        // Check if this is a existing resource
        bool DoesFileExist( DataPath const& path ) const;

        // Check if this is a existing resource
        inline bool DoesFileExist( ResourceID const& resourceID ) const { return DoesFileExist( resourceID.GetDataPath() ); }

        // Gets the list of all known resources
        THashMap<ResourceTypeID, TVector<FileInfo*>> const& GetAllResources() const { return m_resourcesPerType; }

        // Get a list of all known resource of the specified type
        TVector<ResourceID> GetAllResourcesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes = false ) const;

        // Get a list of all known resource of the specified type
        TVector<ResourceID> GetAllResourcesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes = false ) const;

        // Get all resources that depend on this file either for compilation or install - note: this is a very slow function so use sparingly
        void GetAllResourcesThatDependOnFile( DataPath const& sourceFile, TVector<DataPath>& outCompileDependents, TVector<ResourceID>* pOutInstallDependents = nullptr ) const;

        // Get all files that have a reference on this file - this is just checking all data path/resource ID properties - note: this is a very slow function so use sparingly
        void GetAllFilesThatReferenceFile( DataPath const& sourceFile, TVector<DataFileRegistry::FileInfo const*>& outReferencers ) const;

        // Event that fires whenever a resource is deleted
        TEventHandle<DataPath> OnFileDeleted() const { return m_fileDeletedEvent; }

        template<typename T>
        inline T const* GetLoadedDescriptor( ResourceID const& resourceID ) const
        {
            EE_ASSERT( IsDataFileCacheBuilt() );

            auto pFileEntry = GetFileEntry( resourceID );
            if ( pFileEntry != nullptr )
            {
                EE_ASSERT( pFileEntry->m_pDataFile != nullptr );
                return Cast<T>( pFileEntry->m_pDataFile );
            }

            return nullptr;
        }

    private:

        void ClearDatabase();

        void StartFilesystemCacheBuild();

        void StartDataFileCacheBuild();

        // Calling this function will rebuild the dependency cache
        void StartDependencyCacheBuild();

        // Cancel the rebuild of the database
        void CancelDatabaseBuild();

        void HandleMassiveFileSystemChangeDetected();

        // Directory operations
        DirectoryInfo* FindDirectory( FileSystem::Path const& dirPath );
        DirectoryInfo const* FindDirectory( FileSystem::Path const& dirPath ) const { return const_cast<DataFileRegistry*>( this )->FindDirectory( dirPath ); }
        DirectoryInfo* FindOrCreateDirectory( FileSystem::Path const& dirPath );
        void RemoveDirectory( FileSystem::Path const& dirPath );
        void RemoveDirectory( DirectoryInfo* pParentDirectory, int32_t directoryIdx );
        void RemoveAllFilesFromDirectory( DirectoryInfo* pDirectory, bool fireDeletedEvent = true );

        // Add/Remove records
        bool HasFileRecord( FileSystem::Path const& path ) const;
        FileInfo* AddFileRecord( FileSystem::Path const& path, bool shouldLoadDataFile );
        void RemoveFileRecord( FileSystem::Path const& path, bool fireDeletedEvent = true );
        void RemoveFileRecord( DirectoryInfo* pDirectory, int32_t fileIdx, bool fireDeletedEvent = true );

        // File system listener
        void ProcessFileSystemChanges();

    private:

        TypeSystem::TypeRegistry const*                             m_pTypeRegistry = nullptr;
        TaskSystem*                                                 m_pTaskSystem = nullptr;
        FileSystem::Path                                            m_sourceDataDirPath;
        FileSystem::Path                                            m_compiledResourceDirPath;
        int32_t                                                     m_dataDirectoryPathDepth;
        TVector<ResourceTypeID>                                     m_resourceTypesWithDescriptors;

        // File system watcher
        FileSystem::Watcher                                         m_fileSystemWatcher;
        EventBindingID                                              m_massiveFileSystemChangeDetectedEventBinding;

        // Database data
        DirectoryInfo                                               m_sourceDataDirectoryInfo;
        THashMap<ResourceTypeID, TVector<FileInfo*>>                m_resourcesPerType;
        THashMap<DataFileExtension, TVector<FileInfo*>>             m_dataFilesPerExtension;
        THashMap<DataPath, FileInfo*>                               m_filesPerPath;
        mutable TEvent<>                                            m_fileCacheUpdatedEvent;
        mutable TEvent<>                                            m_dependencyCacheUpdatedEvent;
        mutable TEvent<DataPath>                                    m_fileDeletedEvent;

        // Build state
        mutable Threading::ReadWriteMutex                           m_mutex;
        std::atomic<DatabaseState>                                  m_state = DatabaseState::Empty;
        mutable ITaskSet*                                           m_pAsyncTask = nullptr;
        std::atomic<bool>                                           m_cancelActiveTask = false;
        std::atomic<int32_t>                                        m_numItemsProcessed = 0;
        int32_t                                                     m_totalItemsToProcess = 1;
        TVector<FileInfo*>                                          m_dataFilesToLoad;
    };
}
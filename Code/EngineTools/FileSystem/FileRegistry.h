#pragma once
#include "FileSystemWatcher.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Event.h"
#include "Base/Types/HashMap.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Threading/Threading.h"
#include "Base/Types/Function.h"
#include "Base/FileSystem/FileSystemExtension.h"

//-------------------------------------------------------------------------

namespace EE
{
    class TaskSystem;
    namespace TypeSystem { class TypeRegistry; }
    namespace Resource { struct ResourceDescriptor; }
}

//-------------------------------------------------------------------------

namespace EE
{
    // Maintains a DB of all source resources in the source data folder
    //-------------------------------------------------------------------------
    // The way we load descriptors is pretty naive and can definitely be improved

    class EE_ENGINETOOLS_API FileRegistry final
    {
    public:

        enum class DatabaseState
        {
            Empty,
            BuildingFileSystemCache,
            BuildingDataFileCache,
            Ready
        };

        enum class FileType
        {
            Unknown,
            DataFile,
            ResourceDescriptor,
            EntityDescriptor,
        };

        struct FileInfo
        {
            FileInfo() = default;
            ~FileInfo();

            // Custom copying - does not copy datafile ptr!
            FileInfo( FileInfo const& rhs ) { operator=( rhs ); }
            FileInfo& operator=( FileInfo const& rhs );

            // Custom move - transfers datafile ptr!
            FileInfo( FileInfo&& rhs ) { operator=( eastl::move( rhs ) ); }
            FileInfo& operator=( FileInfo&& rhs );

            // Data file
            inline bool IsDataFile() const { return m_fileType == FileType::DataFile; }
            inline bool HasLoadedDataFile() const { return m_pDataFile != nullptr; }
            void LoadDataFile( TypeSystem::TypeRegistry const& typeRegistry );
            void ReloadDataFile( TypeSystem::TypeRegistry const& typeRegistry );

            // Descriptor
            inline bool IsResourceDescriptorFile() const { return m_fileType == FileType::ResourceDescriptor; }
            inline bool HasLoadedDescriptor() const { return HasLoadedDataFile(); }

            // Entity Descriptor
            inline bool IsEntityDescriptorFile() const { return m_fileType == FileType::EntityDescriptor; }

            // Helpers
            inline ResourceTypeID GetResourceTypeID() const
            {
                EE_ASSERT( IsResourceDescriptorFile() || IsEntityDescriptorFile() );
                EE_ASSERT( m_extensionFourCC != 0 );
                return ResourceTypeID( m_extensionFourCC );
            }

            inline ResourceID GetResourceID() const
            {
                EE_ASSERT( IsResourceDescriptorFile() || IsEntityDescriptorFile() );
                return ResourceID( m_dataPath );
            }

        public:

            DataPath                                                m_dataPath;
            FileSystem::Path                                        m_filePath;
            FileSystem::Extension                                   m_extension;
            uint32_t                                                m_extensionFourCC = 0;
            FileType                                                m_fileType = FileType::Unknown;
            IDataFile*                                              m_pDataFile = nullptr;
        };

        struct DirectoryInfo
        {
            inline bool IsEmpty() const { return m_directories.empty() && m_files.empty(); }
            void ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath );

            // Delete all file entries and sub directories
            void Clear();

            void GetAllFiles( TVector<FileInfo const*>& files, bool recurseIntoChildDirectories = false ) const;

        public:

            String                                                  m_name;
            FileSystem::Path                                        m_filePath;
            DataPath                                                m_dataPath;
            TVector<DirectoryInfo>                                  m_directories;
            TVector<FileInfo*>                                      m_files;
        };

    public:

        ~FileRegistry();

        inline bool WasInitialized() const { return m_pTypeRegistry != nullptr; }
        void Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, TaskSystem* pTaskSystem, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath );
        void Shutdown();

        // Process any filesystem updates, returns true if any changes were detected!
        bool Update();

        // Database State
        //-------------------------------------------------------------------------

        // Request a full rebuild of the resource DB, this is currently needed only since we somehow fail to track newly created assets
        // TODO: fix root cause and delete this
        void RequestRebuild();

        // Did we finish building the file system cache?
        bool IsFileSystemCacheBuilt() const { return m_state > DatabaseState::BuildingFileSystemCache; }

        // Event that fires whenever the database is updated
        TEventHandle<> OnFileSystemCacheUpdated() const { return m_fileCacheUpdatedEvent; }

        // Did we finish building the descriptor cache
        bool IsDataFileCacheBuilt() const { return m_state > DatabaseState::BuildingDataFileCache; }

        // Are we currently rebuilding the DB?
        bool IsBuildingCaches() const { return m_state > DatabaseState::Empty && m_state < DatabaseState::Ready; }

        // Get the current progress where 0 mean no progress and 1.0f means complete!
        // Progress is per state i.e. progress will reset when we start building the descriptor cache
        float GetProgress() const;

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
        EE_FORCE_INLINE FileInfo const* GetFileEntry( ResourceID const& resourceID ) const { return GetFileEntry( resourceID.GetResourcePath() ); }

        // Get a list of all known resource file entries of the specified type
        TVector<FileInfo const*> GetAllResourceFileEntries( ResourceTypeID resourceTypeID, bool includeDerivedTypes = false ) const;

        // Get a list of all known resource of the specified type
        TVector<FileInfo const*> GetAllResourceFileEntriesFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes = false ) const;

        // Get a list of all known data files
        TVector<FileInfo const*> GetAllDataFileEntries( uint32_t extensionFourCC ) const;

        // Check if this is a existing resource
        bool DoesFileExist( DataPath const& path ) const;

        // Check if this is a existing resource
        inline bool DoesFileExist( ResourceID const& resourceID ) const { return DoesFileExist( resourceID.GetResourcePath() ); }

        // Gets the list of all known resources
        THashMap<ResourceTypeID, TVector<FileInfo*>> const& GetAllResources() const { return m_resourcesPerType; }

        // Get a list of all known resource of the specified type
        TVector<ResourceID> GetAllResourcesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes = false ) const;

        // Get a list of all known resource of the specified type
        TVector<ResourceID> GetAllResourcesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( Resource::ResourceDescriptor const* )> const& filter, bool includeDerivedTypes = false ) const;

        // Get all dependent resources - note: this is a very slow function so use sparingly
        TVector<DataPath> GetAllDependentResources( DataPath sourceFile ) const;

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

        // Cancel the rebuild of the database
        void CancelDatabaseBuild();

        void HandleMassiveFileSystemChangeDetected();

        // Directory operations
        DirectoryInfo* FindDirectory( FileSystem::Path const& dirPath );
        DirectoryInfo const* FindDirectory( FileSystem::Path const& dirPath ) const { return const_cast<FileRegistry*>( this )->FindDirectory( dirPath ); }
        DirectoryInfo* FindOrCreateDirectory( FileSystem::Path const& dirPath );

        // Add/Remove records
        bool HasFileRecord( FileSystem::Path const& path ) const;
        FileInfo* AddFileRecord( FileSystem::Path const& path, bool shouldLoadDataFile );
        void RemoveFileRecord( FileSystem::Path const& path, bool fireDeletedEvent = true );

        // File system listener
        void ProcessFileSystemChanges();

    private:

        TypeSystem::TypeRegistry const*                             m_pTypeRegistry = nullptr;
        TaskSystem*                                                 m_pTaskSystem = nullptr;
        FileSystem::Path                                            m_sourceDataDirPath;
        FileSystem::Path                                            m_compiledResourceDirPath;
        int32_t                                                     m_dataDirectoryPathDepth;

        // File system watcher
        FileSystem::Watcher                                         m_fileSystemWatcher;
        EventBindingID                                              m_massiveFileSystemChangeDetectedEventBinding;

        // Database data
        DirectoryInfo                                               m_sourceDataDirectoryInfo;
        THashMap<ResourceTypeID, TVector<FileInfo*>>                m_resourcesPerType;
        THashMap<uint32_t, TVector<FileInfo*>>                      m_dataFilesPerExtension;
        THashMap<DataPath, FileInfo*>                               m_filesPerPath;
        mutable TEvent<>                                            m_fileCacheUpdatedEvent;
        mutable TEvent<DataPath>                                    m_fileDeletedEvent;

        // Build state
        std::atomic<DatabaseState>                                  m_state = DatabaseState::Empty;
        ITaskSet*                                                   m_pAsyncTask = nullptr;
        std::atomic<bool>                                           m_cancelActiveTask = false;
        std::atomic<int32_t>                                        m_numItemsProcessed = 0;
        int32_t                                                     m_totalItemsToProcess = 1;
        TVector<FileInfo*>                                          m_dataFilesToLoad;
    };
}
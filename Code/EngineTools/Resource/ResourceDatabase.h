#pragma once
#include "EngineTools/Core/FileSystem/FileSystemWatcher.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Event.h"
#include "Base/Types/HashMap.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Threading/Threading.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class TaskSystem;
    namespace TypeSystem { class TypeRegistry; }
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    // Maintains a DB of all source resources in the source data folder
    //-------------------------------------------------------------------------
    // The way we load descriptors is pretty naive and can definitely be improved

    class EE_ENGINETOOLS_API ResourceDatabase final
    {
    public:

        enum class DatabaseState
        {
            Empty,
            BuildingFileSystemCache,
            BuildingDescriptorCache,
            Ready
        };

        struct FileEntry
        {
            FileEntry() = default;
            ~FileEntry();

            // Disable copying
            FileEntry( FileEntry const& ) = delete;
            FileEntry& operator=( FileEntry& ) = delete;
            FileEntry& operator=( FileEntry const& ) = delete;

            // Descriptor
            inline bool HasDescriptor() const { return m_pDescriptor != nullptr; }
            void LoadDescriptor( TypeSystem::TypeRegistry const& typeRegistry );
            void ReloadDescriptor( TypeSystem::TypeRegistry const& typeRegistry );

        public:

            ResourceID                                              m_resourceID;
            FileSystem::Path                                        m_filePath;
            bool                                                    m_isRegisteredResourceType = false;
            struct ResourceDescriptor*                              m_pDescriptor = nullptr;
        };

        struct DirectoryEntry
        {
            inline bool IsEmpty() const { return m_directories.empty() && m_files.empty(); }
            void ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath );

            // Delete all file entries and sub directories
            void Clear();

        public:

            StringID                                                m_name;
            FileSystem::Path                                        m_filePath;
            ResourcePath                                            m_resourcePath;
            TVector<DirectoryEntry>                                 m_directories;
            TVector<FileEntry*>                                     m_files;
        };

    public:

        ~ResourceDatabase();

        inline bool IsInitialized() const { return m_pTypeRegistry != nullptr; }
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
        TEventHandle<> OnFileSystemCacheUpdated() const { return m_filesystemCacheUpdatedEvent; }

        // Did we finish building the descriptor cache
        bool IsDescriptorCacheBuilt() const { return m_state > DatabaseState::BuildingDescriptorCache; }

        // Are we currently rebuilding the DB?
        bool IsBuildingCaches() const { return m_state > DatabaseState::Empty && m_state < DatabaseState::Ready; }

        // Get the current progress where 0 mean no progress and 1.0f means complete!
        // Progress is per state i.e. progress will reset when we start building the descriptor cache
        inline float GetProgress() const { return float( m_numItemsProcessed ) / m_totalItemsToProcess; }

        // General Resource System Info
        //-------------------------------------------------------------------------

        // Get the path for the raw resources
        inline FileSystem::Path const& GetRawResourceDirectoryPath() const { return m_rawResourceDirPath; }

        // Get the path for the compiled resources
        inline FileSystem::Path const& GetCompiledResourceDirectoryPath() const { return m_compiledResourceDirPath; }

        // Get a structured representation of all known raw resources
        inline DirectoryEntry const* GetRawResourceDirectoryEntry() const { return &m_reflectedDataDirectory; }

        // Resource Info
        //-------------------------------------------------------------------------

        // Try to get an entry for a resource path
        FileEntry const* GetFileEntry( ResourcePath const& resourcePath ) const;

        // Try to get an entry for a resource ID
        EE_FORCE_INLINE FileEntry const* GetFileEntry( ResourceID const& resourceID ) const { return GetFileEntry( resourceID.GetResourcePath() ); }

        // Get a list of all known resource file entries of the specified type
        TVector<FileEntry const*> GetAllFileEntriesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes = false ) const;

        // Get a list of all known resource of the specified type
        TVector<FileEntry const*> GetAllFileEntriesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( ResourceDescriptor const* )> const& filter, bool includeDerivedTypes = false ) const;

        // Check if this is a existing resource
        bool DoesResourceExist( ResourceID const& resourceID ) const;

        // Check if this is a existing resource
        bool DoesResourceExist( ResourcePath const& resourcePath ) const;

        // Gets the list of all known resources
        THashMap<ResourceTypeID, TVector<FileEntry*>> const& GetAllResources() const { return m_resourcesPerType; }

        // Get a list of all known resource of the specified type
        TVector<ResourceID> GetAllResourcesOfType( ResourceTypeID resourceTypeID, bool includeDerivedTypes = false ) const;

        // Get a list of all known resource of the specified type
        TVector<ResourceID> GetAllResourcesOfTypeFiltered( ResourceTypeID resourceTypeID, TFunction<bool( ResourceDescriptor const*)> const& filter, bool includeDerivedTypes = false ) const;

        // Get all dependent resources - note: this is a very slow function so use sparingly
        TVector<ResourcePath> GetAllDependentResources( ResourcePath sourceFile ) const;

        // Event that fires whenever a resource is deleted
        TEventHandle<ResourceID> OnResourceDeleted() const { return m_resourceDeletedEvent; }

        template<typename T>
        inline T const* GetCachedDescriptor( ResourceID const& resourceID ) const
        {
            EE_ASSERT( IsDescriptorCacheBuilt() );

            auto pFileEntry = GetFileEntry( resourceID );
            if ( pFileEntry != nullptr )
            {
                EE_ASSERT( pFileEntry->m_pDescriptor != nullptr );
                return Cast<T>( pFileEntry->m_pDescriptor );
            }

            return nullptr;
        }

    private:

        void ClearDatabase();

        void StartFilesystemCacheBuild();

        void StartDescriptorCacheBuild();

        // Cancel the rebuild of the database
        void CancelDatabaseBuild();

        // Directory operations
        DirectoryEntry* FindDirectory( FileSystem::Path const& dirPath );
        DirectoryEntry* FindOrCreateDirectory( FileSystem::Path const& dirPath );

        // Add/Remove records
        FileEntry* AddFileRecord( FileSystem::Path const& path, bool loadDescriptor );
        void RemoveFileRecord( FileSystem::Path const& path );

        // File system listener
        void ProcessFileSystemChanges();

    private:

        TypeSystem::TypeRegistry const*                             m_pTypeRegistry = nullptr;
        TaskSystem*                                                 m_pTaskSystem = nullptr;
        FileSystem::Path                                            m_rawResourceDirPath;
        FileSystem::Path                                            m_compiledResourceDirPath;
        int32_t                                                     m_dataDirectoryPathDepth;

        // File system watcher
        FileSystem::Watcher                                         m_fileSystemWatcher;

        // Database data
        DirectoryEntry                                              m_reflectedDataDirectory;
        THashMap<ResourceTypeID, TVector<FileEntry*>>               m_resourcesPerType;
        THashMap<ResourcePath, FileEntry*>                          m_resourcesPerPath;
        mutable TEvent<>                                            m_filesystemCacheUpdatedEvent;
        mutable TEvent<ResourceID>                                  m_resourceDeletedEvent;

        // Build state
        std::atomic<DatabaseState>                                  m_state = DatabaseState::Empty;
        ITaskSet*                                                   m_pAsyncTask = nullptr;
        std::atomic<bool>                                           m_cancelActiveTask = false;
        std::atomic<int32_t>                                        m_numItemsProcessed = 0;
        int32_t                                                     m_totalItemsToProcess = 1;
        TVector<FileEntry*>                                         m_descriptorsToLoad;
    };
}
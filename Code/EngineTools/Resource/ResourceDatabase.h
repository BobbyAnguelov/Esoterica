#pragma once
#include "EngineTools/Core/FileSystem/FileSystemWatcher.h"
#include "System/Resource/ResourceID.h"
#include "System/Types/StringID.h"
#include "System/Types/Event.h"
#include "System/Types/HashMap.h"
#include "System/Threading/TaskSystem.h"
#include "System/Threading/Threading.h"
#include "System/Types/Function.h"

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

    class EE_ENGINETOOLS_API ResourceDatabase final : public FileSystem::IFileSystemChangeListener
    {
    public:

        enum class Status
        {
            Idle,
            BuildingDB,
        };

        struct FileEntry
        {
            ~FileEntry();

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

        // Get the path for the raw resources
        inline FileSystem::Path const& GetRawResourceDirectoryPath() const { return m_rawResourceDirPath; }

        // Get the path for the compiled resources
        inline FileSystem::Path const& GetCompiledResourceDirectoryPath() const { return m_compiledResourceDirPath; }

        // Get a structured representation of all known raw resources
        inline DirectoryEntry const* GetRawResourceDirectoryEntry() const { return &m_reflectedDataDirectory; }

        // Try to get an entry for a resource path
        FileEntry const* GetFileEntry( ResourcePath const& resourcePath ) const;

        // Try to get an entry for a resource ID
        EE_FORCE_INLINE FileEntry const* GetFileEntry( ResourceID const& resourceID ) const { return GetFileEntry( resourceID.GetResourcePath() ); }

        //-------------------------------------------------------------------------

        // Are we currently rebuilding the DB?
        bool IsRebuilding() const { return m_pAsyncTask != nullptr; }

        // Get the rebuild progress where 0 mean no progress and 1.0f means complete!
        inline float GetRebuildProgress() const { return m_rebuildProgress; }

        // Process any filesystem updates, returns true if any changes were detected!
        bool Update();

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

        // Event that fires whenever the database is updated
        TEventHandle<> OnDatabaseUpdated() const { return m_databaseUpdatedEvent; }

        // Event that fires whenever a resource is deleted
        TEventHandle<ResourceID> OnResourceDeleted() const { return m_resourceDeletedEvent; }

    private:

        void ClearDatabase();

        // Trigger a full rebuild of the database, this is done async
        void RequestDatabaseRebuild();

        // The actual rebuild task
        void RebuildDatabase();

        // Cancel the rebuild of the database
        void CancelDatabaseRebuild();

        // Directory operations
        DirectoryEntry* FindDirectory( FileSystem::Path const& dirPath );
        DirectoryEntry* FindOrCreateDirectory( FileSystem::Path const& dirPath );

        // Add/Remove records
        void AddFileRecord( FileSystem::Path const& path );
        void RemoveFileRecord( FileSystem::Path const& path );

        // File system listener
        virtual void OnFileCreated( FileSystem::Path const& path ) override final;
        virtual void OnFileDeleted( FileSystem::Path const& path ) override final;
        virtual void OnFileRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath ) override final;
        virtual void OnFileModified( FileSystem::Path const& path ) override final;
        virtual void OnDirectoryCreated( FileSystem::Path const& path ) override final;
        virtual void OnDirectoryDeleted( FileSystem::Path const& path ) override final;
        virtual void OnDirectoryRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath ) override final;

    private:

        TypeSystem::TypeRegistry const*                             m_pTypeRegistry = nullptr;
        TaskSystem*                                                 m_pTaskSystem = nullptr;
        FileSystem::Path                                            m_rawResourceDirPath;
        FileSystem::Path                                            m_compiledResourceDirPath;
        int32_t                                                     m_dataDirectoryPathDepth;
        FileSystem::FileSystemWatcher                               m_fileSystemWatcher;

        Status                                                      m_status = Status::Idle;
        ITaskSet*                                                   m_pAsyncTask = nullptr;
        bool                                                        m_cancelActiveTask = false;
        std::atomic<float>                                          m_rebuildProgress = 1.0f;

        DirectoryEntry                                              m_reflectedDataDirectory;
        THashMap<ResourceTypeID, TVector<FileEntry*>>               m_resourcesPerType;
        THashMap<ResourcePath, FileEntry*>                          m_resourcesPerPath;
        mutable TEvent<>                                            m_databaseUpdatedEvent;
        mutable TEvent<ResourceID>                                  m_resourceDeletedEvent;
    };
}
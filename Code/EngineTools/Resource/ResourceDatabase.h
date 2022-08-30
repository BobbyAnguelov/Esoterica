#pragma once
#include "EngineTools/Core/FileSystem/FileSystemWatcher.h"
#include "System/Resource/ResourceID.h"
#include "System/Types/StringID.h"
#include "System/Types/Event.h"
#include "System/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API ResourceDatabase : public FileSystem::IFileSystemChangeListener
    {
        struct ResourceEntry
        {
            ResourceID                                              m_resourceID;
            FileSystem::Path                                        m_filePath;
        };

    private:

        struct Directory
        {
            void ChangePath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& newPath );
            void Clear();
            bool IsEmpty() const { return m_directories.empty() && m_files.empty(); }

        public:

            StringID                                                m_name;
            FileSystem::Path                                        m_filePath;
            TVector<Directory>                                      m_directories;
            TVector<ResourceEntry*>                                 m_files;
        };

    public:

        ~ResourceDatabase();

        inline bool IsInitialized() const { return m_pTypeRegistry != nullptr; }
        void Initialize( TypeSystem::TypeRegistry const* pTypeRegistry, FileSystem::Path const& rawResourceDirPath, FileSystem::Path const& compiledResourceDirPath );
        void Shutdown();

        inline FileSystem::Path const& GetRawResourceDirectoryPath() const { return m_rawResourceDirPath; }
        inline FileSystem::Path const& GetCompiledResourceDirectoryPath() const { return m_compiledResourceDirPath; }

        //-------------------------------------------------------------------------

        // Process any filesystem updates, returns true if any changes were detected!
        bool Update();

        // Check if this is a existing resource
        bool DoesResourceExist( ResourceID const& resourceID ) const;

        // Gets the list of all found resources
        THashMap<ResourceTypeID, TVector<ResourceEntry*>> const& GetAllResources() const { return m_resourcesPerType; }

        // Get a list of all known resource of the specified type
        TVector<ResourceEntry*> const& GetAllResourcesOfType( ResourceTypeID typeID ) const;

        // Event that fires whenever the database is updated
        TEventHandle<> OnDatabaseUpdated() const { return m_databaseUpdatedEvent; }

        // Event that fires whenever a resource is deleted
        TEventHandle<ResourceID> OnResourceDeleted() const { return m_resourceDeletedEvent; }

    private:

        void RebuildDatabase();
        void ClearDatabase();

        // Directory operations
        Directory* FindDirectory( FileSystem::Path const& dirPath );
        Directory* FindOrCreateDirectory( FileSystem::Path const& dirPath );

        // Add/Remove records
        void AddFileRecord( FileSystem::Path const& path );
        void RemoveFileRecord( FileSystem::Path const& path );

        // File system listener
        virtual void OnFileCreated( FileSystem::Path const& path ) override final;
        virtual void OnFileDeleted( FileSystem::Path const& path ) override final;
        virtual void OnFileRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath ) override final;
        virtual void OnDirectoryCreated( FileSystem::Path const& path ) override final;
        virtual void OnDirectoryDeleted( FileSystem::Path const& path ) override final;
        virtual void OnDirectoryRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath ) override final;

    private:

        TypeSystem::TypeRegistry const*                             m_pTypeRegistry;
        FileSystem::Path                                            m_rawResourceDirPath;
        FileSystem::Path                                            m_compiledResourceDirPath;
        int32_t                                                     m_dataDirectoryPathDepth;
        FileSystem::FileSystemWatcher                               m_fileSystemWatcher;

        Directory                                                   m_rootDir;
        THashMap<ResourceTypeID, TVector<ResourceEntry*>>           m_resourcesPerType;
        mutable TEvent<>                                            m_databaseUpdatedEvent;
        mutable TEvent<ResourceID>                                  m_resourceDeletedEvent;
    };
}
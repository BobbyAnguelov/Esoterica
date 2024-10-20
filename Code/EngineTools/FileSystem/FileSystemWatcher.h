#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Time/Time.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------
// Basic Win32 File System Watcher
//-------------------------------------------------------------------------
// Implementation will try to batch file modification notification to prevent sending multiple events for the same operation
// The OS level function (ReadDirectoryChangesW) will trigger a modification event for multiple operations that are part of a logical operation
//-------------------------------------------------------------------------

#if _WIN32
namespace EE::FileSystem
{
    class EE_ENGINETOOLS_API Watcher final
    {

    public:

        struct Event
        {
            enum Type
            {
                Unknown = 0,

                FileCreated,
                FileDeleted,
                FileRenamed,
                FileModified,

                DirectoryCreated,
                DirectoryDeleted,
                DirectoryRenamed,
                DirectoryModified,
            };

            inline bool IsFileEvent() const { return m_type >= Type::FileCreated && m_type <= Type::FileModified; }
            inline bool IsDirectoryEvent() const { return m_type >= Type::DirectoryCreated; }

        public:

            Type                    m_type = Unknown;
            FileSystem::Path        m_path;
            FileSystem::Path        m_oldPath; // Only set for rename events
        };

    public:

        Watcher();
        ~Watcher();

        bool StartWatching( FileSystem::Path const& directoryToWatch );
        bool IsWatching() const { return m_pDirectoryHandle != nullptr; }
        void StopWatching();

        // Returns true if any filesystem changes detected!
        bool Update();

        // Did any filesystem changes occur?
        inline bool HasFilesystemChangeEvents() const { return !m_unhandledEvents.empty(); }

        // Get all the file system changes since the last update - the event buffer is cleared on each update
        TInlineVector<Event, 50> const& GetFileSystemChangeEvents() const { return m_unhandledEvents; }

        // Event that fires whenever we've detected too many changes to fit in our allocated buffer - user is expected to rescan the watched folder manually!
        TEventHandle<> OnMassiveChangeDetected() { return m_massiveChangeDetectedEvent; }

    private:

        void RequestListOfDirectoryChanges();
        void ProcessListOfDirectoryChanges();

    private:

        FileSystem::Path                                m_directoryToWatch;
        void*                                           m_pDirectoryHandle = nullptr;

        // Request Data
        void*                                           m_pOverlappedEvent = nullptr;
        uint8_t*                                        m_pResultBuffer = nullptr;
        unsigned long                                   m_numBytesReturned = 0;
        bool                                            m_requestPending = false;

        // File Modification buffers
        TInlineVector<Event, 50>                        m_unhandledEvents;

        // OVerflow event
        TEvent<>                                        m_massiveChangeDetectedEvent;
    };
}
#endif
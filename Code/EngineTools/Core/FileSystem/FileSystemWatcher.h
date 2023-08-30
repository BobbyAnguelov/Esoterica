#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------
// Basic Win32 File System Watcher
//-------------------------------------------------------------------------
// Implementation will try to batch file modification notification to prevent sending multiple events for the same operation
// The OS level function (ReadDirectoryChangesW) will trigger a modification event for multiple operations that are part of a logical operation
//-------------------------------------------------------------------------

// TODO: API to retrieve events can process and concatenate events together (i.e. a create, move, rename into a create with the final name) 

#if _WIN32
namespace EE::FileSystem
{
    class EE_ENGINETOOLS_API Watcher final
    {
        static constexpr uint32_t const s_resultBufferSize = 32768;

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

    private:

        void RequestListOfDirectoryChanges();
        void ProcessListOfDirectoryChanges();

    private:

        FileSystem::Path                                m_directoryToWatch;
        void*                                           m_pDirectoryHandle = nullptr;

        // Request Data
        void*                                           m_pOverlappedEvent = nullptr;
        uint8_t                                         m_resultBuffer[s_resultBufferSize] = { 0 };
        unsigned long                                   m_numBytesReturned = 0;
        bool                                            m_requestPending = false;

        // File Modification buffers
        TInlineVector<Event, 50>                        m_unhandledEvents;
    };
}
#endif
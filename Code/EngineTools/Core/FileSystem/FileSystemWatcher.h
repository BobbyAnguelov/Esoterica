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

// TODO: change this to collect all events and provide them to the user in a batch, this will allow users to defer events to specific points in time
// TODO: API to retrieve events can process and concatenate events together (i.e. a create, move, rename into a create with the final name) 

#if _WIN32
namespace EE::FileSystem
{
    class EE_ENGINETOOLS_API IFileSystemChangeListener
    {
    public:

        virtual ~IFileSystemChangeListener() = default;

        virtual void OnFileCreated( FileSystem::Path const& path ) {};
        virtual void OnFileDeleted( FileSystem::Path const& path ) {};
        virtual void OnFileRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath ) {};

        virtual void OnFileModified( FileSystem::Path const& path ) {};

        virtual void OnDirectoryCreated( FileSystem::Path const& path ) {};
        virtual void OnDirectoryDeleted( FileSystem::Path const& path ) {};
        virtual void OnDirectoryRenamed( FileSystem::Path const& oldPath, FileSystem::Path const& newPath ) {};
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API FileSystemWatcher final
    {
        static constexpr uint32_t const ResultBufferSize = 16384;
        static constexpr float const FileModificationBatchTimeout = 250; // How long to wait between the start of the first modification event and notifying the event listeners

        struct FileModificationEvent
        {
            FileModificationEvent( FileSystem::Path const& path ) : m_path( path ) { EE_ASSERT( path.IsValid() && path.IsFilePath() ); }

            FileSystem::Path                            m_path;
            Milliseconds                                m_startTime = PlatformClock::GetTimeInMilliseconds();
        };

    public:

        FileSystemWatcher();
        ~FileSystemWatcher();

        void RegisterChangeListener( IFileSystemChangeListener* pListener );
        void UnregisterChangeListener( IFileSystemChangeListener* pListener );

        bool StartWatching( FileSystem::Path const& directoryToWatch );
        bool IsWatching() const { return m_pDirectoryHandle != nullptr; }
        void StopWatching();

        // Returns true if any filesystem changes detected!
        bool Update();

    private:

        void ProcessResults();
        void ProcessPendingModificationEvents();

    private:

        FileSystem::Path                                m_directoryToWatch;
        void*                                           m_pDirectoryHandle = nullptr;

        // Listeners
        TInlineVector<IFileSystemChangeListener*, 5>    m_changeListeners;

        // Request Data
        void*                                           m_pOverlappedEvent = nullptr;
        uint8_t                                         m_resultBuffer[ResultBufferSize] = { 0 };
        unsigned long                                   m_numBytesReturned = 0;
        bool                                            m_requestPending = false;

        // File Modification buffers
        TVector<FileModificationEvent>                  m_pendingFileModificationEvents;
    };
}
#endif
#include "FileSystemWatcher.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log.h"
#include "System/Platform/PlatformHelpers_Win32.h"

//-------------------------------------------------------------------------

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

//-------------------------------------------------------------------------

#if _WIN32
namespace EE::FileSystem
{
    FileSystemWatcher::FileSystemWatcher()
    {
        // Create overlapped event
        OVERLAPPED* pOverlappedEvent = EE::New<OVERLAPPED>();
        Memory::MemsetZero( pOverlappedEvent, sizeof( OVERLAPPED ) );
        m_pOverlappedEvent = pOverlappedEvent;
    }

    FileSystemWatcher::~FileSystemWatcher()
    {
        EE_ASSERT( m_changeListeners.empty() );
        StopWatching();

        // Delete overlapped event
        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );
        EE::Delete( pOverlappedEvent );
    }

    void FileSystemWatcher::RegisterChangeListener( IFileSystemChangeListener* pListener )
    {
        EE_ASSERT( pListener != nullptr && !VectorContains( m_changeListeners, pListener ) );
        m_changeListeners.emplace_back( pListener );
    }

    void FileSystemWatcher::UnregisterChangeListener( IFileSystemChangeListener* pListener )
    {
        EE_ASSERT( pListener != nullptr && VectorContains( m_changeListeners, pListener ) );
        m_changeListeners.erase_first_unsorted( pListener );
    }

    bool FileSystemWatcher::StartWatching( Path const& directoryToWatch )
    {
        EE_ASSERT( !IsWatching() );
        EE_ASSERT( directoryToWatch.IsValid() && directoryToWatch.IsDirectoryPath() );

        // Get directory handle
        //-------------------------------------------------------------------------

        m_pDirectoryHandle = CreateFile( directoryToWatch.c_str(), GENERIC_READ | FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL );

        if ( m_pDirectoryHandle == INVALID_HANDLE_VALUE )
        {
            EE_LOG_ERROR( "FileSystem", "Failed to open handle to directory (%s), error: %s", m_directoryToWatch.c_str(), Platform::Win32::GetLastErrorMessage().c_str() );
            m_pDirectoryHandle = nullptr;
            return false;
        }

        // Create event
        //-------------------------------------------------------------------------

        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );
        pOverlappedEvent->hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( pOverlappedEvent->hEvent == nullptr )
        {
            EE_LOG_ERROR( "FileSystem", "Failed to create overlapped event: %s", Platform::Win32::GetLastErrorMessage().c_str() );
            CloseHandle( m_pDirectoryHandle );
            m_pDirectoryHandle = nullptr;
            return false;
        }

        m_directoryToWatch = directoryToWatch;
        return true;
    }

    void FileSystemWatcher::StopWatching()
    {
        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );

        // If we are still waiting for an IO request, cancel it
        if ( m_requestPending )
        {
            CancelIo( m_pDirectoryHandle );
            GetOverlappedResult( m_pDirectoryHandle, pOverlappedEvent, &m_numBytesReturned, TRUE);
        }

        // Send all pending notifications
        for ( auto& event : m_pendingFileModificationEvents )
        {
            for ( auto pChangeHandler : m_changeListeners )
            {
                pChangeHandler->OnFileModified( event.m_path );
            }
        }

        m_pendingFileModificationEvents.clear();

        //-------------------------------------------------------------------------

        CloseHandle( pOverlappedEvent->hEvent );
        CloseHandle( m_pDirectoryHandle );
        m_directoryToWatch = Path();
        pOverlappedEvent->hEvent = nullptr;
        m_pDirectoryHandle = nullptr;
    }

    bool FileSystemWatcher::Update()
    {
        EE_ASSERT( IsWatching() );

        auto pOverlappedEvent = reinterpret_cast<OVERLAPPED*>( m_pOverlappedEvent );

        bool changeDetected = false;

        if ( !m_requestPending )
        {
            m_requestPending = ReadDirectoryChangesExW
            (
                m_pDirectoryHandle,
                m_resultBuffer,
                ResultBufferSize,
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                &m_numBytesReturned,
                pOverlappedEvent,
                NULL,
                ReadDirectoryNotifyExtendedInformation
            );

            EE_ASSERT( m_requestPending );
        }
        else // Wait for request to complete
        {
            if ( GetOverlappedResult( m_pDirectoryHandle, pOverlappedEvent, &m_numBytesReturned, FALSE ) )
            {
                if ( m_numBytesReturned != 0 )
                {
                    ProcessResults();
                }

                m_requestPending = false;
                changeDetected = true;
            }
            else // Error occurred or request not complete
            {
                if ( GetLastError() != ERROR_IO_INCOMPLETE )
                {
                    EE_LOG_FATAL_ERROR( "FileSystem", "FileSystemWatcher failed to get overlapped results: %s", Platform::Win32::GetLastErrorMessage().c_str() );
                    EE_HALT();
                }
            }
        }

        //-------------------------------------------------------------------------

        ProcessPendingModificationEvents();
        return changeDetected;
    }

    //-------------------------------------------------------------------------

    namespace
    {
        static Path GetFileSystemPath( Path const& dirPath, _FILE_NOTIFY_EXTENDED_INFORMATION* pNotifyInformation )
        {
            EE_ASSERT( pNotifyInformation != nullptr );

            char strBuffer[256] = { 0 };
            wcstombs( strBuffer, pNotifyInformation->FileName, pNotifyInformation->FileNameLength / 2 );

            Path filePath = dirPath;
            filePath.Append( strBuffer );

            if ( pNotifyInformation->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                filePath.MakeIntoDirectoryPath();
            }
            return filePath;
        }
    }

    void FileSystemWatcher::ProcessResults()
    {
        Path path, secondPath;

        _FILE_NOTIFY_EXTENDED_INFORMATION* pNotify = nullptr;
        size_t offset = 0;

        do
        {
            pNotify = reinterpret_cast<_FILE_NOTIFY_EXTENDED_INFORMATION*>( m_resultBuffer + offset );

            switch ( pNotify->Action )
            {
                case FILE_ACTION_ADDED:
                {
                    path = GetFileSystemPath( m_directoryToWatch, pNotify );

                    if ( path.IsDirectoryPath() )
                    {
                        for ( auto pChangeHandler : m_changeListeners )
                        {
                            pChangeHandler->OnDirectoryCreated( path );
                        }
                    }
                    else
                    {
                        for ( auto pChangeHandler : m_changeListeners )
                        {
                            pChangeHandler->OnFileCreated( path );
                        }
                    }
                }
                break;

                case FILE_ACTION_REMOVED:
                {
                    path = GetFileSystemPath( m_directoryToWatch, pNotify );

                    if ( path.IsDirectoryPath() )
                    {
                        for ( auto pChangeHandler : m_changeListeners )
                        {
                            pChangeHandler->OnDirectoryDeleted( path );
                        }
                    }
                    else
                    {
                        for ( auto pChangeHandler : m_changeListeners )
                        {
                            pChangeHandler->OnFileDeleted( path );
                        }
                    }
                }
                break;

                case FILE_ACTION_MODIFIED:
                {
                    path = GetFileSystemPath( m_directoryToWatch, pNotify );

                    if ( path.IsFilePath() )
                    {
                        auto predicate = [] ( FileModificationEvent const& event, Path const& path )
                        {
                            return event.m_path == path;
                        };

                        if ( !VectorContains( m_pendingFileModificationEvents, path, predicate ) )
                        {
                            m_pendingFileModificationEvents.emplace_back( FileModificationEvent( path ) );
                        }
                    }
                }
                break;

                case FILE_ACTION_RENAMED_OLD_NAME:
                {
                    // Get old name
                    path = GetFileSystemPath( m_directoryToWatch, pNotify );

                    // Get new name
                    EE_ASSERT( pNotify->NextEntryOffset != 0 );
                    offset += pNotify->NextEntryOffset;
                    pNotify = reinterpret_cast<_FILE_NOTIFY_EXTENDED_INFORMATION*>( m_resultBuffer + offset );
                    EE_ASSERT( pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME );

                    secondPath = GetFileSystemPath( m_directoryToWatch, pNotify );

                    //-------------------------------------------------------------------------

                    if ( secondPath.IsDirectoryPath() )
                    {
                        for ( auto pChangeHandler : m_changeListeners )
                        {
                            pChangeHandler->OnDirectoryRenamed( path, secondPath );
                        }
                    }
                    else
                    {
                        for ( auto pChangeHandler : m_changeListeners )
                        {
                            pChangeHandler->OnFileRenamed( path, secondPath );
                        }
                    }
                }
                break;
            }

            offset += pNotify->NextEntryOffset;

        } while ( pNotify->NextEntryOffset != 0 );

        // Clear the result buffer
        Memory::MemsetZero( m_resultBuffer, ResultBufferSize );
    }

    void FileSystemWatcher::ProcessPendingModificationEvents()
    {
        for ( int32_t i = (int32_t) m_pendingFileModificationEvents.size() - 1; i >= 0; i-- )
        {
            auto& pendingEvent = m_pendingFileModificationEvents[i];

            Milliseconds const elapsedTime = PlatformClock::GetTimeInMilliseconds() - pendingEvent.m_startTime;
            if ( elapsedTime > FileModificationBatchTimeout )
            {
                for ( auto pChangeHandler : m_changeListeners )
                {
                    pChangeHandler->OnFileModified( pendingEvent.m_path );
                }

                m_pendingFileModificationEvents.erase_unsorted( m_pendingFileModificationEvents.begin() + i );
            }
        }
    }
}
#endif
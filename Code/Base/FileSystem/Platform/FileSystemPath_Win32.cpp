#ifdef _WIN32
#include "../FileSystemPath.h"
#include <windows.h>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    char const Path::s_pathDelimiter = '\\';
    constexpr static DWORD const g_maxPathBufferLength = 1024;

    //-------------------------------------------------------------------------

    void Path::EnsureCorrectPathStringFormat()
    {
        DWORD dwAttrib = GetFileAttributes( m_fullpath.c_str() );
        if ( dwAttrib == INVALID_FILE_ATTRIBUTES )
        {
            return;
        }

        bool const isPathADirectory = ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY );

        //-------------------------------------------------------------------------

        // Add trailing delimiter for directories
        if ( isPathADirectory && !IsDirectoryPath() )
        {
            m_fullpath += s_pathDelimiter;
            UpdatePathInternals();
        }

        // Remove trailing delimiter for files
        else if ( !isPathADirectory && IsDirectoryPath() )
        {
            m_fullpath.pop_back();
            UpdatePathInternals();
        }
    }

    bool Path::GetFullPathString( char const* pPath, String& outPath )
    {
        InlineString workingBuffer;

        if ( pPath != nullptr && pPath[0] != 0 )
        {
            // Warning: this function is slow, so use sparingly
            workingBuffer.reserve( g_maxPathBufferLength );
            DWORD const length = GetFullPathNameA( pPath, g_maxPathBufferLength, workingBuffer.data(), nullptr );
            EE_ASSERT( length != 0 && length != g_maxPathBufferLength );
            workingBuffer.force_size( length );

            // Ensure directory paths have the final slash appended
            DWORD const result = GetFileAttributesA( workingBuffer.c_str() );
            if ( result != INVALID_FILE_ATTRIBUTES && ( result & FILE_ATTRIBUTE_DIRECTORY ) && workingBuffer[length - 1] != s_pathDelimiter )
            {
                workingBuffer += s_pathDelimiter;
            }

            outPath = workingBuffer;
            return true;
        }

        outPath.clear();
        return false;
    }

    bool Path::GetCorrectCaseForPath( char const* pPath, String& outPath )
    {
        bool failed = false;

        HANDLE hFile = CreateFile( pPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            char buffer[g_maxPathBufferLength];
            DWORD dwRet = GetFinalPathNameByHandle( hFile, buffer, g_maxPathBufferLength, FILE_NAME_NORMALIZED );
            if ( dwRet < g_maxPathBufferLength )
            {
                outPath = buffer + 4;
            }
            else
            {
                failed = true;
            }

            CloseHandle( hFile );
        }
        else // Invalid handle i.e. file doesnt exist
        {
            failed = true;
        }

        //-------------------------------------------------------------------------

        if ( failed )
        {
            outPath = pPath;
        }

        //-------------------------------------------------------------------------

        return !failed;
    }
}
#endif
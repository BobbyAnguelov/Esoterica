#ifdef _WIN32
#include "../FileSystemPath.h"
#include <windows.h>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
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
            m_fullpath += Settings::s_pathDelimiter;
            UpdatePathInternals();
        }

        // Remove trailing delimiter for files
        else if ( !isPathADirectory && IsDirectoryPath() )
        {
            m_fullpath.pop_back();
            UpdatePathInternals();
        }
    }
}
#endif
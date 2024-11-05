#include "FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    bool EnsureDirectoryExists( char const* path )
    {
        if ( !IsExistingDirectory( path ) )
        {
            return CreateDir( path );
        }

        return true;
    }
}
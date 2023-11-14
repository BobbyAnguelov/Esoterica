#include "FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    size_t FindExtensionStartIdx( String const& path, char const pathDelimiter, bool supportMultiExtensions )
    {
        // Try to find the last period in the path
        //-------------------------------------------------------------------------

        size_t extensionIdx = path.rfind( '.' );
        if ( extensionIdx == String::npos )
        {
            return extensionIdx;
        }

        // If we have the path delimiter and it is after our last period, then there's no extension
        //-------------------------------------------------------------------------

        size_t const pathDelimiterIdx = path.find_last_of( pathDelimiter );
        if ( pathDelimiterIdx > extensionIdx )
        {
            extensionIdx = String::npos;
            return extensionIdx;
        }

        // Try to handle multi-extensions like ".tar.gzip"
        //-------------------------------------------------------------------------

        size_t prevIdx = extensionIdx;

        if ( supportMultiExtensions )
        {
            while ( extensionIdx != String::npos && extensionIdx > pathDelimiterIdx )
            {
                prevIdx = extensionIdx;
                extensionIdx = path.rfind( '.', extensionIdx - 1 );
            }

            EE_ASSERT( prevIdx != String::npos );
        }

        //-------------------------------------------------------------------------

        prevIdx++;
        extensionIdx = prevIdx;
        return extensionIdx;
    }
}
#include "FileStreams.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    InputFileStream::InputFileStream( Path const& filePath )
    {
        EE_ASSERT( filePath.IsFilePath() );
        if ( !FileSystem::Exists( filePath ) )
        {
            return;
        }

        m_filestream.open( filePath, std::ios::in | std::ios::binary );

        if ( !m_filestream.is_open() )
        {
            String const msg = "Error opening file ( " + (String) filePath + " ) for reading - " + strerror( errno );
            EE_LOG_WARNING( "FileSystem", "Input File", msg.c_str() );
        }
    }

    OutputFileStream::OutputFileStream( Path const& filePath )
    {
        EE_ASSERT( filePath.IsFilePath() );
        filePath.EnsureDirectoryExists();
        m_filestream.open( filePath, std::ios::out | std::ios::trunc | std::ios::binary );

        if ( !m_filestream.is_open() )
        {
            String const msg = "Error opening file ( " + (String) filePath + " ) for writing- " + strerror( errno );
            EE_LOG_WARNING( "FileSystem", "Output File", msg.c_str() );
        }
    }
}
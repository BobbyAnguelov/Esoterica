#include "JsonSerialization.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/ThirdParty/rapidjson/error/en.h"

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    char const* GetJsonErrorMessage( rapidjson::ParseErrorCode code )
    {
        return rapidjson::GetParseError_En( code );
    }

    //-------------------------------------------------------------------------

    JsonArchiveReader::~JsonArchiveReader()
    {
        if ( m_pStringBuffer != nullptr )
        {
            EE::Free( m_pStringBuffer );
        }
    }

    bool JsonArchiveReader::ReadFromFile( FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsFilePath() );
        Reset();

        //-------------------------------------------------------------------------

        if ( FileSystem::Exists( filePath ) )
        {
            FILE* pFile = fopen( filePath, "r" );

            if ( pFile == nullptr )
            {
                return false;
            }

            fseek( pFile, 0, SEEK_END );
            size_t filesize = (size_t) ftell( pFile );
            fseek( pFile, 0, SEEK_SET );

            m_pStringBuffer = (uint8_t*) EE::Alloc( filesize + 1 );
            size_t readLength = fread( m_pStringBuffer, 1, filesize, pFile );
            m_pStringBuffer[readLength] = '\0';
            fclose( pFile );

            m_document.ParseInsitu( (char*) m_pStringBuffer );

            bool const isValidJsonFile = ( m_document.GetParseError() == rapidjson::kParseErrorNone );
            if ( isValidJsonFile )
            {
                OnFileReadSuccess();
            }

            return isValidJsonFile;
        }

        return false;
    }

    bool JsonArchiveReader::ReadFromString( char const* pString )
    {
        EE_ASSERT( pString != nullptr );
        Reset();

        // Copy string data
        //-------------------------------------------------------------------------

        size_t const stringLength = strlen( pString );
        size_t const requiredMemory = sizeof( char ) * ( stringLength + 1 );
        m_pStringBuffer = (uint8_t*) EE::Alloc( requiredMemory );
        m_pStringBuffer[stringLength] = '\0';

        memcpy( m_pStringBuffer, pString, requiredMemory - sizeof( char ) );

        // Parse json data
        //-------------------------------------------------------------------------

        m_document.ParseInsitu( (char*) m_pStringBuffer );

        bool const isValidJsonFile = ( m_document.GetParseError() == rapidjson::kParseErrorNone );
        if ( isValidJsonFile )
        {
            OnFileReadSuccess();
        }

        return isValidJsonFile;
    }

    void JsonArchiveReader::Reset()
    {
        m_document.Clear();

        if ( m_pStringBuffer != nullptr )
        {
            EE::Free( m_pStringBuffer );
        }
    }

    //-------------------------------------------------------------------------

    bool JsonArchiveWriter::WriteToFile( FileSystem::Path const& outPath )
    {
        EE_ASSERT( outPath.IsFilePath() );

        FinalizeSerializedData();

        //-------------------------------------------------------------------------

        if ( !outPath.EnsureDirectoryExists() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        FILE* pFile = fopen( outPath, "w" );
        if ( pFile == nullptr )
        {
            Reset();
            return false;
        }

        fwrite( m_stringBuffer.GetString(), m_stringBuffer.GetSize(), 1, pFile );
        fclose( pFile );

        Reset();
        return true;
    }

    void JsonArchiveWriter::Reset()
    {
        m_stringBuffer.Clear();
    }
}
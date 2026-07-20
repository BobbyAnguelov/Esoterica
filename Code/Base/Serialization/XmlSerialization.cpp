#include "XmlSerialization.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    struct XmlStringWriter : pugi::xml_writer
    {
    public:

        XmlStringWriter( String& outStr ) : m_result( outStr ) { outStr.clear(); }

        virtual void write( const void* data, size_t size )
        {
            m_result.append( static_cast<const char*>( data ), size );
        }

    public:

        String &m_result;
    };

    //-------------------------------------------------------------------------

    bool ReadXmlFromFile( FileSystem::Path const& path, xml_document& outDoc )
    {
        if ( !path.IsValid() )
        {
            return false;
        }

        pugi::xml_parse_result const result = outDoc.load_file( path.c_str() );

        if ( !result )
        {
            EE_LOG_ERROR( LogCategory::Serialization, "XML", "Failed to load xml file (%s): %s", path.c_str(), result.description() );
        }

        return result;
    }

    bool ReadXmlFromBuffer( void const* pBuffer, size_t bufferSize, xml_document& outDoc, bool suppressLogging )
    {
        EE_ASSERT( pBuffer != nullptr && bufferSize > 0 );

        pugi::xml_parse_result const result = outDoc.load_buffer( pBuffer, bufferSize );

        if ( !result && !suppressLogging )
        {
            EE_LOG_ERROR( LogCategory::Serialization, "XML", "Failed to load xml from str: %s", result.description() );
        }

        return result;
    }

    bool ReadXmlFromBufferInPlace( void* pBuffer, size_t bufferSize, xml_document& outDoc, bool suppressLogging )
    {
        EE_ASSERT( pBuffer != nullptr && bufferSize > 0 );

        pugi::xml_parse_result const result = outDoc.load_buffer_inplace( pBuffer, bufferSize );

        if ( !result && !suppressLogging )
        {
            EE_LOG_ERROR( LogCategory::Serialization, "XML", "Failed to load xml from str: %s", result.description() );
        }

        return result;
    }

    bool WriteXmlToFile( xml_document const& doc, FileSystem::Path const& path, bool onlyWriteFileIfContentsChanged /*= false */ )
    {
        if ( !path.IsValid() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        String xmlStr;
        XmlStringWriter writer( xmlStr );
        doc.print( writer, "  " );

        if ( writer.m_result.empty() )
        {
            return false;
        }

        if ( onlyWriteFileIfContentsChanged )
        {
            return FileSystem::UpdateTextFile( path, xmlStr );
        }
        else
        {
            return FileSystem::WriteTextFile( path, xmlStr );
        }
    }

    bool WriteXmlToString( xml_document const& doc, String& outStr )
    {
        XmlStringWriter writer( outStr );
        doc.print( writer );
        return !outStr.empty();
    }
}
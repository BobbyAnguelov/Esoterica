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
            EE_LOG_ERROR( "Serialization", "XML", "Failed to load xml file (%s): %s", path.c_str(), result.description() );
        }

        return result;
    }

    bool ReadXmlFromString( String const& str, xml_document& outDoc, bool suppressLogging )
    {
        if ( str.empty() )
        {
            return false;
        }

        pugi::xml_parse_result const result = outDoc.load_buffer( str.c_str(), str.size() );

        if ( !result && !suppressLogging )
        {
            EE_LOG_ERROR( "Serialization", "XML", "Failed to load xml from str: %s", result.description() );
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
#include "IniFile.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/ThirdParty/iniparser/iniparser.h"

//-------------------------------------------------------------------------

namespace EE
{
    IniFile::IniFile( FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsValid() );
        m_pDictionary = iniparser_load( filePath.c_str() );
    }

    IniFile::IniFile()
    {
        m_pDictionary = dictionary_new( 0 );
    }

    IniFile::~IniFile()
    {
        if ( m_pDictionary != nullptr )
        {
            iniparser_freedict( m_pDictionary );
        }
    }

    bool IniFile::HasEntry( char const* key ) const
    {
        EE_ASSERT( IsValid() );
        return iniparser_find_entry( m_pDictionary, key ) > 0;
    }

    void IniFile::SaveToFile( FileSystem::Path const& filePath ) const
    {
        EE_ASSERT( IsValid() );

        FILE* pFile;
        pFile = fopen( filePath.c_str(), "w" );
        iniparser_dump_ini( m_pDictionary, pFile );
        fclose( pFile );
    }

    bool IniFile::TryGetBool( char const* key, bool& outValue ) const
    {
        EE_ASSERT( IsValid() );

        if ( HasEntry( key ) )
        {
            outValue = (bool) iniparser_getboolean( m_pDictionary, key, false );
            return true;
        }

        return false;
    }

    bool IniFile::TryGetInt( char const* key, int32_t& outValue ) const
    {
        EE_ASSERT( IsValid() );

        if ( HasEntry( key ) )
        {
            outValue = iniparser_getint( m_pDictionary, key, 0 );
            return true;
        }

        return false;
    }

    bool IniFile::TryGetUInt( char const* key, uint32_t& outValue ) const
    {
        EE_ASSERT( IsValid() );

        if ( HasEntry( key ) )
        {
            outValue = ( uint32_t) iniparser_getint( m_pDictionary, key, 0 );
            return true;
        }

        return false;
    }

    bool IniFile::TryGetString( char const* key, String& outValue ) const
    {
        EE_ASSERT( IsValid() );

        if ( HasEntry( key ) )
        {
            outValue = iniparser_getstring( m_pDictionary, key, "" );
            return true;
        }

        return false;
    }

    bool IniFile::TryGetFloat( char const* key, float& outValue ) const
    {
        EE_ASSERT( IsValid() );

        if ( HasEntry( key ) )
        {
            outValue = ( float) iniparser_getdouble( m_pDictionary, key, 0.0f );
            return true;
        }

        return false;
    }

    bool IniFile::GetBoolOrDefault( char const* key, bool defaultValue ) const
    {
        EE_ASSERT( IsValid() );
        return (bool) iniparser_getboolean( m_pDictionary, key, defaultValue );
    }

    int32_t IniFile::GetIntOrDefault( char const* key, int32_t defaultValue ) const
    {
        EE_ASSERT( IsValid() );
        return (int32_t) iniparser_getint( m_pDictionary, key, defaultValue );
    }

    uint32_t IniFile::GetUIntOrDefault( char const* key, uint32_t defaultValue ) const
    {
        EE_ASSERT( IsValid() );
        return (uint32_t) iniparser_getint( m_pDictionary, key, defaultValue );
    }

    String IniFile::GetStringOrDefault( char const* key, String const& defaultValue ) const
    {
        EE_ASSERT( IsValid() );
        return iniparser_getstring( m_pDictionary, key, defaultValue.c_str() );
    }

    String IniFile::GetStringOrDefault( char const* key, char const* pDefaultValue ) const
    {
        EE_ASSERT( IsValid() );
        return iniparser_getstring( m_pDictionary, key, pDefaultValue );
    }

    float IniFile::GetFloatOrDefault( char const* key, float defaultValue ) const
    {
        EE_ASSERT( IsValid() );
        return (float) iniparser_getdouble( m_pDictionary, key, defaultValue );
    }

    void IniFile::CreateSection( char const* section )
    {
        EE_ASSERT( IsValid() );
        iniparser_set( m_pDictionary, section, nullptr );
    }

    void IniFile::SetBool( char const* key, bool value )
    {
        EE_ASSERT( IsValid() );
        iniparser_set( m_pDictionary, key, value ? "True" : "False" );
    }

    void IniFile::SetInt( char const* key, int32_t value )
    {
        EE_ASSERT( IsValid() );
        char buffer[255];
        Printf( buffer, 255, "%d", value );
        iniparser_set( m_pDictionary, key, buffer );
    }

    void IniFile::SetUInt( char const* key, uint32_t value )
    {
        EE_ASSERT( IsValid() );
        char buffer[255];
        Printf( buffer, 255, "%d", value );
        iniparser_set( m_pDictionary, key, buffer );
    }

    void IniFile::SetFloat( char const* key, float value )
    {
        EE_ASSERT( IsValid() );
        char buffer[255];
        Printf( buffer, 255, "%f", value );
        iniparser_set( m_pDictionary, key, buffer );
    }

    void IniFile::SetString( char const* key, char const* pString )
    {
        EE_ASSERT( IsValid() );
        iniparser_set( m_pDictionary, key, pString );
    }

    void IniFile::SetString( char const* key, String const& string )
    {
        EE_ASSERT( IsValid() );
        iniparser_set( m_pDictionary, key, string.c_str() );
    }
}
#include "IniFile.h"
#include "Base/FileSystem/FileSystemPath.h"

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4866 )
#include "Base/ThirdParty/mINI/ini.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool IniFile::Load( FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsValid() );
        EE_ASSERT( filePath.IsFilePath() );

        m_sections.clear();

        // Read File
        //-------------------------------------------------------------------------

        mINI::INIFile file( filePath.c_str() );
        mINI::INIStructure ini;

        if ( !file.read( ini ) )
        {
            return false;
        }

        // Reflect Loaded INI
        //-------------------------------------------------------------------------

        for ( auto& section : ini )
        {
            auto pSection = FindOrCreateSection( section.first.c_str() );

            for ( auto& kv : section.second )
            {
                auto pKey = pSection->FindOrCreateKeyValue( kv.first.c_str() );
                pKey->m_value = kv.second.c_str();
            }
        }

        return true;
    }

    bool IniFile::Save( FileSystem::Path const& filePath ) const
    {
        mINI::INIFile file( filePath.c_str() );
        mINI::INIStructure ini;

        //-------------------------------------------------------------------------

        for ( auto const& section : m_sections )
        {
            for ( auto const& kv : section.m_kv )
            {
                ini[section.m_name.c_str()][kv.m_key.c_str()] = kv.m_value.c_str();
            }
        }

        //-------------------------------------------------------------------------

        return file.write( ini, true );
    }

    bool IniFile::HasEntry( char const* pSectionName, char const* pKey ) const
    {
        EE_ASSERT( pSectionName != nullptr );
        EE_ASSERT( pKey != nullptr );

        Section const* pSection = GetSection( pSectionName );
        if ( pSection == nullptr )
        {
            return false;
        }

        return pSection->HasKey( pKey );
    }

    //-------------------------------------------------------------------------

    bool IniFile::GetBool( char const* pSectionName, char const* pKeyID, bool defaultValue ) const
    {
        auto pKey = GetKeyValue( pSectionName, pKeyID );
        if ( pKey == nullptr )
        {
            return defaultValue;
        }

        InlineString lowerString = pKey->m_value.c_str();
        lowerString.make_lower();
        return ( lowerString == "true" );
    }

    int64_t IniFile::GetInt( char const* pSectionName, char const* pKeyID, int64_t defaultValue ) const
    {
        auto pKey = GetKeyValue( pSectionName, pKeyID );
        if ( pKey == nullptr )
        {
            return defaultValue;
        }

        return std::strtol( pKey->m_value.c_str(), nullptr, 0 );
    }

    float IniFile::GetFloat( char const* pSectionName, char const* pKeyID, float defaultValue ) const
    {
        auto pKey = GetKeyValue( pSectionName, pKeyID );
        if ( pKey == nullptr )
        {
            return defaultValue;
        }

        return std::strtof( pKey->m_value.c_str(), nullptr );
    }

    char const* IniFile::GetString( char const* pSectionName, char const* pKeyID, char const* pDefaultStr ) const
    {
        auto pKey = GetKeyValue( pSectionName, pKeyID );
        return ( pKey != nullptr ) ? pKey->m_value.c_str() : pDefaultStr;
    }

    //-------------------------------------------------------------------------

    void IniFile::SetBool( char const* pSectionName, char const* pKeyID, bool value )
    {
        auto pKV = FindOrCreateKeyValue( pSectionName, pKeyID );
        pKV->m_value = value ? "true" : "false";
    }

    void IniFile::SetInt( char const* pSectionName, char const* pKeyID, int64_t value )
    {
        auto pKV = FindOrCreateKeyValue( pSectionName, pKeyID );
        pKV->m_value = eastl::to_string( value );
    }

    void IniFile::SetFloat( char const* pSectionName, char const* pKeyID, float value )
    {
        auto pKV = FindOrCreateKeyValue( pSectionName, pKeyID );
        pKV->m_value = eastl::to_string( value );
    }

    void IniFile::SetString( char const* pSectionName, char const* pKeyID, char const* pString )
    {
        auto pKV = FindOrCreateKeyValue( pSectionName, pKeyID );
        pKV->m_value = pString;
    }
}

#pragma warning( pop )
#endif
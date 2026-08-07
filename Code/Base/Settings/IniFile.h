#pragma once
#include "Base/_Module/API.h"
#include "Base/Types/String.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------
// INI File Reader/Writer
//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API IniFile
    {
        struct KeyValue
        {
            inline bool IsValid() const { return !m_key.empty(); }

            inline bool operator==( char const* pKey ) const
            {
                return StringUtils::Stricmp( pKey, m_key.c_str() ) == 0;
            }

        public:

            String              m_key;
            String              m_value;
        };

        //-------------------------------------------------------------------------

        struct Section
        {
            inline bool IsValid() const { return !m_name.empty(); }

            inline KeyValue* GetKey( char const* pKey )
            {
                EE_ASSERT( pKey != nullptr );
                auto iter = eastl::find( m_kv.begin(), m_kv.end(), pKey, [] ( KeyValue const& kv, char const* pKey ) { return StringUtils::Stricmp( pKey, kv.m_key.c_str() ) == 0; } );
                return ( iter != m_kv.end() ) ? iter : nullptr;
            }

            inline KeyValue const* GetKey( char const* pKeyID ) const { return const_cast<Section*>( this )->GetKey( pKeyID ); }

            inline bool HasKey( char const* pKey ) const { return GetKey( pKey ) != nullptr; }

            inline KeyValue* FindOrCreateKeyValue( char const* pKey )
            {
                EE_ASSERT( pKey != nullptr );
                auto iter = eastl::find( m_kv.begin(), m_kv.end(), pKey, [] ( KeyValue const& kv, char const* pKey ) { return StringUtils::Stricmp( pKey, kv.m_key.c_str() ) == 0; } );
                if ( iter != m_kv.end() )
                {
                    return iter;
                }

                auto pKeyValue = &m_kv.emplace_back();
                pKeyValue->m_key = pKey;
                return pKeyValue;
            }

        public:

            String              m_name;
            TVector<KeyValue>   m_kv;
        };

    public:

        IniFile() = default;

        bool Load( FileSystem::Path const& filePath );
        bool Save( FileSystem::Path const& filePath ) const;

        //-------------------------------------------------------------------------

        bool HasEntry( char const* pSectionName, char const* pKeyID ) const;

        //-------------------------------------------------------------------------

        bool GetBool( char const* pSectionName, char const* pKeyID, bool defaultValue ) const;
        int64_t GetInt( char const* pSectionName, char const* pKeyID, int64_t defaultValue ) const;
        char const* GetString( char const* pSectionName, char const* pKeyID, char const* pDefaultStr = "" ) const;
        float GetFloat( char const* pSectionName, char const* pKeyID, float defaultValue ) const;

        //-------------------------------------------------------------------------

        void SetBool( char const* pSectionName, char const* pKeyID, bool value );
        void SetInt( char const* pSectionName, char const* pKeyID, int64_t value );
        void SetFloat( char const* pSectionName, char const* pKeyID, float value );
        void SetString( char const* pSectionName, char const* pKeyID, char const* pString );

    private:

        IniFile( IniFile const& ) = delete;
        IniFile& operator=( IniFile const& ) = delete;

        inline Section* GetSection( char const* pSectionName )
        {
            EE_ASSERT( pSectionName != nullptr );
            auto iter = eastl::find( m_sections.begin(), m_sections.end(), pSectionName, [] ( Section const& s, char const* pSectionName ) { return StringUtils::Stricmp( pSectionName, s.m_name.c_str() ) == 0; } );
            return ( iter != m_sections.end() ) ? iter : nullptr;
        }

        inline Section const* GetSection( char const* pSectionName ) const { return const_cast<IniFile*>( this )->GetSection( pSectionName ); }

        inline Section* FindOrCreateSection( char const* pSectionName )
        {
            EE_ASSERT( pSectionName != nullptr );
            auto iter = eastl::find( m_sections.begin(), m_sections.end(), pSectionName, [] ( Section const& s, char const* pSectionName ) { return StringUtils::Stricmp( pSectionName, s.m_name.c_str() ) == 0; } );
            if ( iter != m_sections.end() )
            {
                return iter;
            }

            auto pSection = &m_sections.emplace_back();
            pSection->m_name = pSectionName;
            return pSection;
        }

        inline KeyValue* FindOrCreateKeyValue( char const* pSectionName, char const* pKeyID )
        {
            return FindOrCreateSection( pSectionName )->FindOrCreateKeyValue( pKeyID );
        }

        inline KeyValue* GetKeyValue( char const* pSectionName, char const* pKeyID )
        {
            Section* pSection = GetSection( pSectionName );
            if ( pSection == nullptr )
            {
                return nullptr;
            }

            return pSection->GetKey( pKeyID );
        }

        inline KeyValue const* GetKeyValue( char const* pSectionName, char const* pKeyID ) const { return const_cast<IniFile*>( this )->GetKeyValue( pSectionName, pKeyID ); }

        inline bool HasSection( char const* pSectionName ) const { return GetSection( pSectionName ) != nullptr; }

    private:

        TVector<Section>    m_sections;
    };
}
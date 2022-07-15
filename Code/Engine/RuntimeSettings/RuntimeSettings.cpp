#include "RuntimeSettings.h"
#include "System/Algorithm/Hash.h"

//-------------------------------------------------------------------------

namespace EE
{
    RuntimeSetting* RuntimeSetting::s_pHead = nullptr;
    RuntimeSetting* RuntimeSetting::s_pTail = nullptr;

    //-------------------------------------------------------------------------

    RuntimeSetting::RuntimeSetting( char const* pName, char const* pCategory, char const* pDescription, Type type )
        : m_type( type )
    {
        // Name Validation
        //-------------------------------------------------------------------------

        size_t const nameLength = strlen( pName );
        EE_ASSERT( nameLength > 0 && nameLength < 100 );

        for ( auto i = 0; i < nameLength - 1; i++ )
        {
            EE_ASSERT( isalnum( pName[i] ) || pName[i] == ' ' );
        }

        // Basic Category Validation
        //-------------------------------------------------------------------------
        // Derived types are expected to provide more detailed validation

        size_t const categoryLength = strlen( pCategory );
        for ( auto i = 0; i < categoryLength - 1; i++ )
        {
            EE_ASSERT( isalnum( pCategory[i] ) || pCategory[i] == ' ' || pCategory[i] == s_categorySeparatorToken );
        }

        // Description Validation
        //-------------------------------------------------------------------------

        size_t const descriptionLength = ( pDescription == nullptr ) ? 0 : strlen( pDescription );
        EE_ASSERT( descriptionLength < 256 );

        if ( pDescription != nullptr )
        {
            strcpy( const_cast<char*>( m_description ), pDescription );
        }

        // Set Internal State
        //-------------------------------------------------------------------------

        // Copy strings
        strcpy( const_cast<char*>( m_name ), pName );
        strcpy( const_cast<char*>( m_category ), pCategory );

        // Generate name hash
        char fullNameBuffer[256] = { 0 };
        memcpy( fullNameBuffer, pCategory, categoryLength );
        memcpy( fullNameBuffer + categoryLength, pName, nameLength );
        m_nameHash = Hash::GetHash32( fullNameBuffer );

        // Add to global list
        if ( s_pHead != nullptr )
        {
            s_pTail->m_pNext = this;
            s_pTail = this;
        }
        else
        {
            s_pHead = this;
            s_pTail = this;
        }
    }

    RuntimeSetting::~RuntimeSetting()
    {
        // Remove from global list
        //-------------------------------------------------------------------------

        // If we are the head of the list, just change the head to our next sibling
        if ( s_pHead == this )
        {
            s_pHead = m_pNext;

            // If we are also the tail, then empty the list
            if ( s_pTail == this )
            {
                s_pTail = nullptr;
            }
        }
        else // Find our previous sibling
        {
            auto pPrevious = s_pHead;
            while ( pPrevious != nullptr )
            {
                if ( pPrevious->m_pNext == this )
                {
                    break;
                }

                pPrevious = pPrevious->m_pNext;
            }

            EE_ASSERT( pPrevious != nullptr );

            // Remove ourselves from the list
            pPrevious->m_pNext = m_pNext;

            // Update the tail of the list if needed
            if ( s_pTail == this )
            {
                s_pTail = pPrevious;
            }
        }
    }

    //-------------------------------------------------------------------------

    RuntimeSettingBool::RuntimeSettingBool( char const* pName, char const* pCategory, char const* pDescription, bool initialValue )
        : RuntimeSetting( pName, pCategory, pDescription, Type::Bool )
        , m_value( initialValue )
    {}

    //-------------------------------------------------------------------------

    RuntimeSettingInt::RuntimeSettingInt( char const* pName, char const* pCategory, char const* pDescription, int32_t initialValue )
        : RuntimeSetting( pName, pCategory, pDescription, Type::Int )
        , m_value( initialValue )
    {}

    RuntimeSettingInt::RuntimeSettingInt( char const* pName, char const* pCategory, char const* pDescription, int32_t initialValue, int32_t min, int32_t max )
        : RuntimeSetting( pName, pCategory, pDescription, Type::Int )
        , m_value( initialValue )
        , m_min( min )
        , m_max( max )
    {}

    //-------------------------------------------------------------------------

    RuntimeSettingFloat::RuntimeSettingFloat( char const* pName, char const* pCategory, char const* pDescription, float initialValue )
        : RuntimeSetting( pName, pCategory, pDescription, Type::Float )
        , m_value( initialValue )
    {}

    RuntimeSettingFloat::RuntimeSettingFloat( char const* pName, char const* pCategory, char const* pDescription, float initialValue, float min, float max )
        : RuntimeSetting( pName, pCategory, pDescription, Type::Float )
        , m_value( initialValue )
        , m_min( min )
        , m_max( max )
    {}
}

//-------------------------------------------------------------------------

namespace EE
{
    SettingsRegistry::SettingsRegistry()
    {
        GenerateSettingsMap();
    }

    RuntimeSetting const* SettingsRegistry::TryGetSetting( char const* pCategoryName, char const* pSettingName ) const
    {
        EE_ASSERT( pCategoryName != nullptr && pSettingName != nullptr );
        InlineString combinedName( pCategoryName );
        combinedName += pSettingName;

        uint32_t const combinedNameHash = Hash::GetHash32( combinedName.c_str() );

        auto settingsIter = m_settings.find( combinedNameHash );
        if ( settingsIter != m_settings.end() )
        {
            return settingsIter->second;
        }

        return nullptr;
    }

    void SettingsRegistry::GenerateSettingsMap()
    {
        m_settings.clear();

        //-------------------------------------------------------------------------

        RuntimeSetting* pSetting = RuntimeSetting::s_pHead;
        while ( pSetting != nullptr )
        {
            // Add to global list - settings are expected to be unique!
            EE_ASSERT( m_settings.find( pSetting->m_nameHash ) == m_settings.end() );
            m_settings.insert( TPair<uint32_t, RuntimeSetting*>( pSetting->m_nameHash, pSetting ) );

            // Move onto next setting
            pSetting = pSetting->m_pNext;
        }
    }
}
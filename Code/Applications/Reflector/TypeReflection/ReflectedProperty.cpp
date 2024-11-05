#include "ReflectedProperty.h"
#include "Base/Utils/StringKeyValueParser.h"
#include "ReflectionSettings.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static String GenerateFriendlyName( String const& name )
    {
        String friendlyName = name;

        if ( name.length() <= 1 )
        {
            return friendlyName;
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "m_", "" );

        if ( friendlyName.length() > 1 && friendlyName[0] == 'p' && isupper( friendlyName[1] ) )
        {
            friendlyName = friendlyName.substr( 1, friendlyName.length() - 1 );
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "_", " " );

        friendlyName[0] = (char) toupper( friendlyName[0] );

        int32_t i = 1;
        while ( i < friendlyName.length() )
        {
            // Only insert a space before a Capital letter, if it isnt the last letter and if it isnt followed or preceded by a capital letter
            bool const shouldInsertSpace = isupper( friendlyName[i] ) && i != friendlyName.length() - 1 && ( friendlyName[i - 1] != '/' ) && !isupper( friendlyName[i - 1] ) && !isupper( friendlyName[i + 1] );
            if ( shouldInsertSpace )
            {
                friendlyName.insert( friendlyName.begin() + i, 1, ' ' );
                i++;
            }

            i++;
        }

        return friendlyName;
    }

    //-------------------------------------------------------------------------

    void ReflectedProperty::GenerateMetaData( InlineString& outWarnings )
    {
        outWarnings.clear();

        m_metaData.m_flags.ClearAllFlags();
        m_metaData.m_keyValues.clear();

        // Generate friendly name
        //-------------------------------------------------------------------------

        m_reflectedFriendlyName = GenerateFriendlyName( m_name );

        // Create entries for name and category
        //-------------------------------------------------------------------------

        {
            PropertyMetadata::KV& nameKV = m_metaData.m_keyValues.emplace_back();
            nameKV.m_key = PropertyMetadata::Flag::FriendlyName;
            nameKV.m_value = m_reflectedFriendlyName;
            m_metaData.m_flags.SetFlag( nameKV.m_key );

            PropertyMetadata::KV& descKV = m_metaData.m_keyValues.emplace_back();
            descKV.m_key = PropertyMetadata::Flag::Description;
            descKV.m_value = m_reflectedDescription;
            m_metaData.m_flags.SetFlag( descKV.m_key );
        }

        // Parse the supplied meta data string
        //-------------------------------------------------------------------------

        KeyValueParser const parser( m_rawMetaDataStr.data() );
        for ( KeyValueParser::KV const& rawKV : parser.m_keyValues )
        {
            bool foundCoreMetaFlag = false;

            for ( int32_t i = 0; i < PropertyMetadata::s_numFlags; i++ )
            {
                PropertyMetadata::Flag const flag = (PropertyMetadata::Flag) i;

                // If we detect a key with a known value
                if ( rawKV.m_key.comparei( PropertyMetadata::s_flagStrings[i] ) == 0 )
                {
                    foundCoreMetaFlag = true;

                    PropertyMetadata::KV* pMetaKV = nullptr;

                    if ( flag == PropertyMetadata::Flag::FriendlyName )
                    {
                        pMetaKV = &m_metaData.m_keyValues[0];
                    }
                    else if ( flag == PropertyMetadata::Flag::Description )
                    {
                        pMetaKV = &m_metaData.m_keyValues[1];
                    }
                    else
                    {
                        pMetaKV = &m_metaData.m_keyValues.emplace_back();
                    }

                    // Set data
                    //-------------------------------------------------------------------------

                    pMetaKV->m_key = (PropertyMetadata::Flag) i;
                    pMetaKV->m_value = rawKV.m_value;

                    // Validation
                    //-------------------------------------------------------------------------\

                    switch ( flag )
                    {
                        case PropertyMetadata::Flag::Min:
                        case PropertyMetadata::Flag::Max:
                        {
                            if ( pMetaKV->m_value.empty() )
                            {
                                outWarnings.append( "    * Min/Max meta flags require a value to be set!\n" );
                            }
                            else
                            {
                                char *pEnd = nullptr;
                                std::strtof( rawKV.m_value.c_str(), &pEnd );
                                if ( pEnd == rawKV.m_value.c_str() )
                                {
                                    outWarnings.append( "    * Min/Max meta flags require a numeric value to be set!\n" );
                                }
                            }
                        }
                        break;

                        default:
                        {}
                        break;
                    }

                    //-------------------------------------------------------------------------

                    m_metaData.m_flags.SetFlag( flag );
                }
            }

            // Handle unknown meta data
            //-------------------------------------------------------------------------

            if ( !foundCoreMetaFlag )
            {
                // Set data
                //-------------------------------------------------------------------------

                PropertyMetadata::KV& customMetaKV = m_metaData.m_keyValues.emplace_back();
                customMetaKV.m_key = PropertyMetadata::Flag::Unknown;
                customMetaKV.m_keyValue = rawKV.m_key;
                customMetaKV.m_value = rawKV.m_value;
            }
        }

        // Escape description str
        //-------------------------------------------------------------------------

        PropertyMetadata::KV& descKV = m_metaData.m_keyValues[PropertyMetadata::Flag::Description];
        StringUtils::ReplaceAllOccurrencesInPlace( descKV.m_value, "\"", "\\\"" );
    }
}
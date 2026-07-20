#include "PropertyMetadata.h"
#include "Base/Utils/StringKeyValueParser.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    PropertyMetadata::PropertyMetadata( String const& metaDataStr, Log* pLog )
        : PropertyMetadata()
    {
        AppendMetaDataFromString( metaDataStr, pLog );
    }

    void PropertyMetadata::AppendMetaDataFromString( String const& metaDataStr, Log* pLog )
    {
        // Parse the supplied meta data string
        //-------------------------------------------------------------------------

        KeyValueParser const parser( metaDataStr.data() );
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

                    // Set data
                    //-------------------------------------------------------------------------

                    PropertyMetadata::KV& metaKV = m_keyValues.emplace_back();
                    metaKV.m_key = (PropertyMetadata::Flag) i;
                    metaKV.m_value = rawKV.m_value;

                    // Validation
                    //-------------------------------------------------------------------------\

                    switch ( flag )
                    {
                        case PropertyMetadata::Flag::Min:
                        case PropertyMetadata::Flag::Max:
                        {
                            if ( metaKV.m_value.empty() )
                            {
                                if ( pLog != nullptr )
                                {
                                    pLog->LogWarning( "    * Min/Max meta flags require a value to be set!\n" );
                                }
                            }
                            else
                            {
                                char *pEnd = nullptr;
                                std::strtof( rawKV.m_value.c_str(), &pEnd );
                                if ( pEnd == rawKV.m_value.c_str() )
                                {
                                    if ( pLog != nullptr )
                                    {
                                        pLog->LogWarning( "    * Min/Max meta flags require a numeric value to be set!\n" );
                                    }
                                }
                            }
                        }
                        break;

                        default:
                        break;
                    }

                    //-------------------------------------------------------------------------

                    m_flags.SetFlag( flag );
                }
            }

            // Handle unknown meta data
            //-------------------------------------------------------------------------

            if ( !foundCoreMetaFlag )
            {
                // Set data
                //-------------------------------------------------------------------------

                PropertyMetadata::KV& customMetaKV = m_keyValues.emplace_back();
                customMetaKV.m_key = PropertyMetadata::Flag::Unknown;
                customMetaKV.m_keyValue = rawKV.m_key;
                customMetaKV.m_value = rawKV.m_value;
            }
        }
    }
}
#include "ReflectionDataTypes.h"
#include "Base/Utils/StringKeyValueParser.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static void GenerateFriendlyName( String& name )
    {
        if ( name.size() <= 1 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( name, "_", " " );

        name[0] = (char) toupper( name[0] );

        int32_t i = 1;
        while ( i < name.length() )
        {
            // Only insert a space before a Capital letter, if it isnt the last letter and if it isnt followed or preceded by a capital letter
            bool const shouldInsertSpace = isupper( name[i] ) && i != name.length() - 1 && ( name[i - 1] != '/') && !isupper(name[i - 1]) && !isupper(name[i + 1]);
            if ( shouldInsertSpace )
            {
                name.insert( name.begin() + i, 1, ' ' );
                i++;
            }

            i++;
        }
    }

    //-------------------------------------------------------------------------

    ReflectedProperty const* ReflectedType::GetPropertyDescriptor( StringID propertyID ) const
    {
        EE_ASSERT( m_ID.IsValid() && !IsAbstract() && !IsEnum() );
        for ( auto const& prop : m_properties )
        {
            if ( prop.m_propertyID == propertyID )
            {
                return &prop;
            }
        }

        return nullptr;
    }

    void ReflectedProperty::GenerateMetaData( InlineString& outWarnings )
    {
        outWarnings.clear();

        m_metaData.m_flags.ClearAllFlags();
        m_metaData.m_keyValues.clear();

        // Generate friendly name
        //-------------------------------------------------------------------------

        m_reflectedFriendlyName = m_name;
        StringUtils::ReplaceAllOccurrencesInPlace( m_reflectedFriendlyName, "m_", "" );

        if ( !m_reflectedFriendlyName.empty() )
        {
            if ( m_reflectedFriendlyName.length() > 1 && m_reflectedFriendlyName[0] == 'p' && isupper( m_reflectedFriendlyName[1] ) )
            {
                m_reflectedFriendlyName = m_reflectedFriendlyName.substr( 1, m_reflectedFriendlyName.length() - 1 );
            }

            GenerateFriendlyName( m_reflectedFriendlyName );
        }
        else
        {
            m_reflectedFriendlyName = m_name;
        }

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

    //-------------------------------------------------------------------------

    void ReflectedType::AddEnumConstant( ReflectedEnumConstant const& constant )
    {
        EE_ASSERT( m_ID.IsValid() && IsEnum() );
        EE_ASSERT( constant.m_ID.IsValid() );
        EE_ASSERT( !IsValidEnumLabelID( constant.m_ID ) );
        m_enumConstants.emplace_back( constant );
    }

    bool ReflectedType::GetValueFromEnumLabel( StringID labelID, uint32_t& value ) const
    {
        EE_ASSERT( m_ID.IsValid() && IsEnum() );

        for ( auto const& constant : m_enumConstants )
        {
            if ( constant.m_ID == labelID )
            {
                value = constant.m_value;
                return true;
            }
        }

        return false;
    }

    bool ReflectedType::IsValidEnumLabelID( StringID labelID ) const
    {
        for ( auto const& constant : m_enumConstants )
        {
            if ( constant.m_ID == labelID )
            {
                return true;
            }
        }

        return false;
    }

    String ReflectedType::GetFriendlyName() const
    {
        String friendlyName = m_name;
        GenerateFriendlyName( friendlyName );
        return friendlyName;
    }

    String ReflectedType::GetInternalNamespace() const
    {
        String ns = m_namespace.c_str() + 4;

        if ( StringUtils::EndsWith( ns, "::" ) )
        {
            ns = ns.substr( 0, ns.length() - 2 );
        }

        return ns;
    }

    String ReflectedType::GetCategory() const
    {
        String category = m_namespace;
        StringUtils::ReplaceAllOccurrencesInPlace( category, Settings::g_engineNamespacePlusDelimiter, "" );
        StringUtils::ReplaceAllOccurrencesInPlace( category, "::", "/" );

        // Remove trailing slash
        if ( !category.empty() && category.back() == '/' )
        {
            category.pop_back();
        }

        GenerateFriendlyName( category );

        return category;
    }

    bool ReflectedType::HasArrayProperties() const
    {
        for ( auto& propertyDesc : m_properties )
        {
            if ( propertyDesc.IsArrayProperty() )
            {
                return true;
            }
        }

        return false;
    }

    bool ReflectedType::HasDynamicArrayProperties() const
    {
        for ( auto& propertyDesc : m_properties )
        {
            if ( propertyDesc.IsDynamicArrayProperty() )
            {
                return true;
            }
        }

        return false;
    }

    bool ReflectedType::HasResourcePtrProperties() const
    {
        for ( auto& propertyDesc : m_properties )
        {
            if ( propertyDesc.IsResourcePtrProperty() )
            {
                return true;
            }
        }

        return false;
    }

    bool ReflectedType::HasResourcePtrOrStructProperties() const
    {
        for ( auto& propertyDesc : m_properties )
        {
            if ( propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
            {
                return true;
            }

            if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr )
            {
                return true;
            }

            if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    bool ReflectedResourceType::TryParseResourceRegistrationMacroString( String const& registrationStr )
    {
        // Generate type ID string and get friendly name
        size_t const resourceIDStartIdx = registrationStr.find( '\'', 0 );
        if ( resourceIDStartIdx == String::npos )
        {
            return false;
        }

        size_t const resourceIDEndIdx = registrationStr.find( '\'', resourceIDStartIdx + 1 );
        if ( resourceIDEndIdx == String::npos )
        {
            return false;
        }

        size_t const resourceFriendlyNameStartIdx = registrationStr.find( '"', resourceIDEndIdx + 1 );
        if ( resourceFriendlyNameStartIdx == String::npos )
        {
            return false;
        }

        size_t const resourceFriendlyNameEndIdx = registrationStr.find( '"', resourceFriendlyNameStartIdx + 1 );
        if ( resourceFriendlyNameEndIdx == String::npos )
        {
            return false;
        }

        EE_ASSERT( resourceIDStartIdx != resourceIDEndIdx );
        EE_ASSERT( resourceFriendlyNameStartIdx != resourceFriendlyNameEndIdx );

        //-------------------------------------------------------------------------

        String const resourceTypeIDString = registrationStr.substr( resourceIDStartIdx + 1, resourceIDEndIdx - resourceIDStartIdx - 1 );

        m_resourceTypeID = ResourceTypeID( resourceTypeIDString );
        if ( !m_resourceTypeID.IsValid() )
        {
            return false;
        }
        
        m_friendlyName = registrationStr.substr( resourceFriendlyNameStartIdx + 1, resourceFriendlyNameEndIdx - resourceFriendlyNameStartIdx - 1 );

        return true;
    }

    //-------------------------------------------------------------------------

    bool ReflectedDataFileType::TryParseDataFileRegistrationMacroString( String const& registrationStr )
    {
        // Skip the first part of the string since this is the typename for data file
        size_t searchStartIdx = 0;
        searchStartIdx = registrationStr.find( ',', searchStartIdx );

        // Generate type ID string and get friendly name
        size_t const extensionStartIdx = registrationStr.find( '\'', searchStartIdx );
        if ( extensionStartIdx == String::npos )
        {
            return false;
        }

        size_t const extensionEndIdx = registrationStr.find( '\'', extensionStartIdx + 1 );
        if ( extensionEndIdx == String::npos )
        {
            return false;
        }

        size_t const resourceFriendlyNameStartIdx = registrationStr.find( '"', extensionEndIdx + 1 );
        if ( resourceFriendlyNameStartIdx == String::npos )
        {
            return false;
        }

        size_t const resourceFriendlyNameEndIdx = registrationStr.find( '"', resourceFriendlyNameStartIdx + 1 );
        if ( resourceFriendlyNameEndIdx == String::npos )
        {
            return false;
        }

        EE_ASSERT( extensionStartIdx != extensionEndIdx );
        EE_ASSERT( resourceFriendlyNameStartIdx != resourceFriendlyNameEndIdx );

        //-------------------------------------------------------------------------

        String const extensionString = registrationStr.substr( extensionStartIdx + 1, extensionEndIdx - extensionStartIdx - 1 );
        if ( FourCC::IsValidLowercase( extensionString.c_str() ) )
        {
            m_extensionFourCC = FourCC::FromString( extensionString.c_str() );
        }
        else
        {
            return false;
        }

        m_friendlyName = registrationStr.substr( resourceFriendlyNameStartIdx + 1, resourceFriendlyNameEndIdx - resourceFriendlyNameStartIdx - 1 );

        return true;
    }
}
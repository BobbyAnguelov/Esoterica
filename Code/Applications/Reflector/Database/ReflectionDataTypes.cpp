#include "ReflectionDataTypes.h"

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

    String ReflectedProperty::GetFriendlyName() const
    {
        String name = m_name;
        StringUtils::ReplaceAllOccurrencesInPlace( name, "m_", "" );

        if ( name.empty() )
        {
            return m_name;
        }

        if ( name.length() > 1 && name[0] == 'p' && isupper( name[1] ) )
        {
            name = name.substr( 1, name.length() - 1 );
        }

        GenerateFriendlyName( name );

        return name;
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
            if ( propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
            {
                return true;
            }

            if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr )
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
}
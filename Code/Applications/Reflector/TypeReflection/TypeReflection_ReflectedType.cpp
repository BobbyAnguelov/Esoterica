#include "TypeReflection_ReflectedType.h"
#include "Applications/Reflector/ReflectorSettings.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    static String GenerateFriendlyName( String const& name )
    {
        String friendlyName = name;

        if ( name.size() <= 1 )
        {
            return friendlyName;
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "_", " " );
        friendlyName[0] = (char) toupper( friendlyName[0] );
        StringUtils::InsertSpacesAccordingToCapitalization( friendlyName );

        return friendlyName;
    }

    //-------------------------------------------------------------------------

    ReflectedType::ReflectedType( TypeSystem::TypeID ID, String const& name )
        : m_ID( ID )
        , m_name( name )
    {}

    ReflectedProperty const* ReflectedType::GetPropertyDescriptor( StringID propertyID ) const
    {
        EE_ASSERT( m_ID.IsValid() && !IsAbstract() && !IsEnum() );
        for ( auto const& prop : m_properties )
        {
            if ( prop.m_ID == propertyID )
            {
                return &prop;
            }
        }

        return nullptr;
    }

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
        String friendlyName = GenerateFriendlyName( m_name );
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

        category = GenerateFriendlyName( category );

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
            if ( propertyDesc.m_typeID == TypeSystem::CoreTypeID::ResourcePtr )
            {
                return true;
            }

            if ( propertyDesc.m_typeID == TypeSystem::CoreTypeID::TResourcePtr )
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
}
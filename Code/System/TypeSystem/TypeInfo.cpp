#include "TypeInfo.h"
#include "TypeDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    bool TypeInfo::IsDerivedFrom( TypeID const potentialParentTypeID ) const
    {
        if ( potentialParentTypeID == m_ID )
        {
            return true;
        }

        for ( auto& actualParentTypeInfo : m_parentTypes )
        {
            if ( actualParentTypeInfo->m_ID == potentialParentTypeID )
            {
                return true;
            }

            // Check inheritance hierarchy
            if ( actualParentTypeInfo->IsDerivedFrom( potentialParentTypeID ) )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    PropertyInfo const* TypeInfo::GetPropertyInfo( StringID propertyID ) const
    {
        PropertyInfo const* pProperty = nullptr;

        auto propertyIter = m_propertyMap.find( propertyID );
        if ( propertyIter != m_propertyMap.end() )
        {
            int32_t const propertyIdx = propertyIter->second;
            pProperty = &m_properties[propertyIdx];
        }

        return pProperty;
    }
}
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

        if ( m_pParentTypeInfo != nullptr )
        {
            if ( m_pParentTypeInfo->m_ID == potentialParentTypeID )
            {
                return true;
            }

            // Check inheritance hierarchy
            if ( m_pParentTypeInfo->IsDerivedFrom( potentialParentTypeID ) )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    PropertyInfo* TypeInfo::GetPropertyInfo( StringID propertyID )
    {
        PropertyInfo* pProperty = nullptr;

        auto propertyIter = m_propertyMap.find( propertyID );
        if ( propertyIter != m_propertyMap.end() )
        {
            int32_t const propertyIdx = propertyIter->second;
            pProperty = &m_properties[propertyIdx];
        }

        return pProperty;
    }
}
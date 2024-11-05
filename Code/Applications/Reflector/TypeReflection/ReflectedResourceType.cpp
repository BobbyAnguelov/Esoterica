#include "ReflectedResourceType.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
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

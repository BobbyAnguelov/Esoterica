#include "ReflectedDataFileType.h"
#include "Base/Encoding/FourCC.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
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
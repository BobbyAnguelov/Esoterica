#pragma once
#include "Base/TypeSystem/TypeID.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    struct ReflectedDataFileType
    {
        // Fill the data type ID and the friendly name from the macro registration string
        bool TryParseDataFileRegistrationMacroString( String const& registrationStr );

    public:

        TypeID                                          m_typeID;
        uint32_t                                        m_extensionFourCC = 0;
        String                                          m_friendlyName;
        StringID                                        m_headerID;
        String                                          m_className;
        String                                          m_namespace;
        bool                                            m_isDevOnly = true;
    };
}
#pragma once
#include "Base/TypeSystem/TypeID.h"
#include "Base/Types/String.h"
#include "Base/FileSystem/DataFileExtension.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    struct ReflectedDataFileType
    {
        // Fill the data type ID and the friendly name from the macro registration string
        bool TryParseDataFileRegistrationMacroString( String const& registrationStr );

    public:

        TypeSystem::TypeID                              m_typeID;
        DataFileExtension                               m_dataFileExtension;
        String                                          m_friendlyName;
        StringID                                        m_headerID;
        String                                          m_className;
        String                                          m_namespace;
        bool                                            m_isDevOnly = true;
    };
}
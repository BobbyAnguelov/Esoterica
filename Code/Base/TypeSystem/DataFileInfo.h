#pragma once
#include "TypeID.h"
#include "Base/FileSystem/DataFileExtension.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    #if EE_DEVELOPMENT_TOOLS
    struct EE_BASE_API DataFileInfo
    {
        inline bool IsValid() const
        {
            if ( !m_typeID.IsValid() || !m_extension.IsValid() )
            {
                return false;
            }

            if ( !StringUtils::IsLowercaseAlphaNumeric( m_extension.ToString() ) )
            {
                return false;
            }

            return true;
        }

        inline FileSystem::Extension GetFileSystemExtension() const { return m_extension.ToFileSystemExtension(); }

    public:

        TypeID                      m_typeID;
        DataFileExtension           m_extension;
        String                      m_friendlyName;
    };
    #endif
}
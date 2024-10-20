#pragma once
#include "TypeID.h"
#include "Base/Encoding/FourCC.h"
#include "Base/FileSystem/FileSystemExtension.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    #if EE_DEVELOPMENT_TOOLS
    struct EE_BASE_API DataFileInfo
    {
        inline bool IsValid() const { return m_typeID.IsValid() && m_extensionFourCC != 0; }
        FileSystem::Extension GetExtension() const { return FourCC::ToLowercaseString( m_extensionFourCC ); }

    public:

        TypeID                      m_typeID;
        uint32_t                    m_extensionFourCC = 0;
        String                      m_friendlyName;
    };
    #endif
}
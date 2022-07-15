#pragma once
#include "TypeID.h"
#include "System/Resource/ResourceTypeID.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    struct EE_SYSTEM_API ResourceInfo
    {
        inline bool IsValid() const { return m_typeID.IsValid() && m_resourceTypeID.IsValid(); }

    public:

        TypeID                  m_typeID;
        ResourceTypeID          m_resourceTypeID;
        bool                    m_isVirtualResource = false;

        #if EE_DEVELOPMENT_TOOLS
        String                  m_friendlyName;
        #endif
    };
}
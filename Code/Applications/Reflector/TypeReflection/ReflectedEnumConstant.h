#pragma once
#include "Base/Types/StringID.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    struct ReflectedEnumConstant
    {
        StringID                                        m_ID;
        String                                          m_label;
        int32_t                                         m_value;
        String                                          m_description;
    };
}
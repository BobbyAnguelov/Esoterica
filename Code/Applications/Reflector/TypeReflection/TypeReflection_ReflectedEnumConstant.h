#pragma once
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    struct ReflectedEnumConstant
    {
        StringID                                        m_ID;
        String                                          m_label;
        int32_t                                         m_value;
        String                                          m_description;
        bool                                            m_isHiddenInTools = false;
    };
}
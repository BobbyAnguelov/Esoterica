#include "ResourceServerContext.h"
#include "Base/Resource/ResourceSettings.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    bool ResourceServerContext::IsValid() const
    {
        if ( m_pCompilerRegistry == nullptr || m_pTypeRegistry == nullptr )
        {
            return false;
        }

        return true;
    }
}
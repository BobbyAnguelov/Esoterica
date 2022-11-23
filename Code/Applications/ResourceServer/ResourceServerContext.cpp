#include "ResourceServerContext.h"
#include "System/Resource/ResourceSettings.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    bool ResourceServerContext::IsValid() const
    {
        if ( !m_rawResourcePath.IsValid() || !m_rawResourcePath.Exists() )
        {
            return false;
        }

        if ( !m_compiledResourcePath.IsValid() || !m_compiledResourcePath.Exists() )
        {
            return false;
        }

        if ( m_pCompilerRegistry == nullptr || m_pTypeRegistry == nullptr )
        {
            return false;
        }

        if ( m_pCompiledResourceDB == nullptr || !m_pCompiledResourceDB->IsConnected() )
        {
            return false;
        }

        return true;
    }
}
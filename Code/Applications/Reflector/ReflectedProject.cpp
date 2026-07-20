#include "ReflectedProject.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    ReflectedHeader const* ReflectedProject::GetModuleHeader() const
    {
        EE_ASSERT( m_moduleHeaderID.IsValid() );
        for ( auto const& header : m_headerFiles )
        {
            if ( header.m_ID == m_moduleHeaderID )
            {
                return &header;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }
}
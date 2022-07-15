#include "Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    SystemRegistry::~SystemRegistry()
    {
        EE_ASSERT( m_registeredSystems.empty() );
    }

    void SystemRegistry::RegisterSystem( ISystem* pSystem )
    {
        EE_ASSERT( pSystem != nullptr && !VectorContains( m_registeredSystems, pSystem ) );
        m_registeredSystems.emplace_back( pSystem );
    }

    void SystemRegistry::UnregisterSystem( ISystem* pSystem )
    {
        auto iter = VectorFind( m_registeredSystems, pSystem );
        EE_ASSERT( iter != m_registeredSystems.end() );
        m_registeredSystems.erase_unsorted( iter );
    }
}
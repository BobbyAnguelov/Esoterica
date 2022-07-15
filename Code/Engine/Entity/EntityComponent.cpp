#include "EntityComponent.h"

//-------------------------------------------------------------------------

namespace EE
{
    EntityComponent::~EntityComponent()
    {
        EE_ASSERT( m_status == Status::Unloaded );
        EE_ASSERT( m_isRegisteredWithEntity == false && m_isRegisteredWithWorld == false );
    }
}
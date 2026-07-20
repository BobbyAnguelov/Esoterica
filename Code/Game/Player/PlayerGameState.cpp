#include "PlayerGameState.h"

//-------------------------------------------------------------------------

namespace EE
{
    static float g_energyRefillSpeed = 0.5f;

    //-------------------------------------------------------------------------

    void PlayerGameState::Update( float deltaTime )
    {
        m_energy += g_energyRefillSpeed * deltaTime;
        m_energy = Math::Clamp( m_energy, 0.0f, m_maxEnergy );
    }
}

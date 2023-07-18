#include "Component_MainPlayer.h"
#include "Base\Math\Math.h"

namespace EE::Player
{
    static float g_energyRefillSpeed = 0.5f;
    static float g_maxEnergyLevel = 3.0f;

    //-------------------------------------------------------------------------

    void MainPlayerComponent::UpdateState( float deltaTime )
    {
        m_energyLevel += g_energyRefillSpeed * deltaTime;
        m_energyLevel = Math::Clamp( m_energyLevel, 0.0f, g_maxEnergyLevel );
    }
}
#include "Component_MainPlayer.h"
#include "Game\Player\Input\PlayerInput.h"
#include "Base\Math\Math.h"

namespace EE::Player
{
    static float g_energyRefillSpeed = 0.5f;
    static float g_maxEnergyLevel = 3.0f;

    //-------------------------------------------------------------------------

    MainPlayerComponent::MainPlayerComponent()
        : PlayerComponent()
    {
        m_pInputMap = EE::New<GameInputMap>();
        m_pInputMap->RegisterInputs();
    }

    MainPlayerComponent::~MainPlayerComponent()
    {
        EE::Delete( m_pInputMap );
    }

    void MainPlayerComponent::UpdateState( float deltaTime )
    {
        m_energyLevel += g_energyRefillSpeed * deltaTime;
        m_energyLevel = Math::Clamp( m_energyLevel, 0.0f, g_maxEnergyLevel );
    }
}
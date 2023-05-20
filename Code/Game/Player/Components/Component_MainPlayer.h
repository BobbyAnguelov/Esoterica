#pragma once

#include "Game/_Module/API.h"
#include "Engine/Player/Components/Component_Player.h"
#include "System/Math/FloatCurve.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphVariation;
}

//-------------------------------------------------------------------------

namespace EE::Player
{
    class EnergyController;

    //-------------------------------------------------------------------------

    class EE_GAME_API MainPlayerComponent : public PlayerComponent
    {
        friend EnergyController;
        EE_ENTITY_COMPONENT( MainPlayerComponent );

    public:

        float GetEnergyLevel() const { return m_energyLevel; }
        inline bool HasEnoughEnergy( float requiredEnergy ) const { return m_energyLevel >= requiredEnergy; }
        inline void ConsumeEnergy( float ConsumedEnergy ) { EE_ASSERT( m_energyLevel >= ConsumedEnergy ); m_energyLevel -= ConsumedEnergy; }

        inline float GetAngularVelocityLimit( float speed ) { return m_angularVelocityLimitCurve.Evaluate( speed ); }

        void UpdateState( float deltaTime );

    public:

        bool                                    m_sprintFlag = false;
        bool                                    m_crouchFlag = false;
        EE_REFLECT() FloatCurve                    m_jumpCurve;
        EE_REFLECT() FloatCurve                    m_angularVelocityLimitCurve;

        // HACK!!! - Just to test out the external graph feature for now
        Animation::GraphVariation const*        m_pAvailableInteraction = nullptr;

    private:

        float                                   m_energyLevel = 3.0f;
    };
}
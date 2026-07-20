#pragma once

#include "Game/Base/GameState.h"
#include "Base/Math/FloatCurve.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerGameState final : public GameState
    {
        EE_REFLECT_TYPE( PlayerGameState );

    public:

        inline float GetEnergy() const { return m_energy; }
        inline float GetMaxEnergy() const { return m_maxEnergy; }
        inline bool HasRequiredEnergy( float energyRequirements ) const { return m_energy >= energyRequirements; }
        inline Percentage GetPercentageEnergyLeft() const { return m_energy / m_maxEnergy; }
        inline void ResetEnergy() { m_energy = m_maxEnergy; }
        inline void ConsumeEnergy( float consumedEnergy ) { m_energy = Math::Max( 0.0f, m_energy - consumedEnergy ); }

        inline float GetAngularVelocityLimit( float speed ) const { return m_angularVelocityLimitCurve.Evaluate( speed ); }

        virtual void Update( float deltaTime ) override;

    public:

        EE_REFLECT();
        FloatCurve                              m_angularVelocityLimitCurve;

        bool                                    m_sprintFlag = false;
        bool                                    m_crouchFlag = false;
        bool                                    m_isInTimeDilation = false;

    private:

        float                                   m_energy = 5.0f;
        float                                   m_maxEnergy = 5.0f;
    };
}
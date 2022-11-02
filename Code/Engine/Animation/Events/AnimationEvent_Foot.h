#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct Color;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API FootEvent final : public Event
    {
        EE_REGISTER_TYPE( FootEvent );

    public:

        // The actual foot phase
        enum class Phase : uint8_t
        {
            EE_REGISTER_ENUM

            LeftFootDown = 0,
            RightFootPassing = 1,
            RightFootDown = 2,
            LeftFootPassing = 3,
        };

        // Used wherever we need to specify a phase (or phase group)
        enum class PhaseCondition : uint8_t
        {
            EE_REGISTER_ENUM

            LeftFootDown = 0,
            LeftFootPassing = 1,
            LeftPhase = 4, // The whole phase for the right foot down (right foot down + left foot passing)

            RightFootDown = 2,
            RightFootPassing = 3,
            RightPhase = 5, // The whole phase for the right foot down (right foot down + left foot passing)
        };

        #if EE_DEVELOPMENT_TOOLS
        static char const* GetPhaseName( Phase phase );
        static char const* GetPhaseConditionName( PhaseCondition phaseCondition );
        static Color GetPhaseColor( Phase phase );
        #endif

    public:

        inline Phase GetFootPhase() const { return m_phase; }
        virtual StringID GetSyncEventID() const override;

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return GetPhaseName( m_phase ); }
        #endif

    private:

        EE_EXPOSE Phase     m_phase = Phase::LeftFootDown;
    };
}
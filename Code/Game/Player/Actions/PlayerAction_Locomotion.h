#pragma once

#include "PlayerAction.h"
#include "System\Time\Timers.h"
#include "System\Math\Vector.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class LocomotionAction final : public Action
    {
        enum class LocomotionState
        {
            Idle,
            Moving,
            TurnOnSpot,
            PlantedTurn,
            Stopping
        };

    public:

        EE_PLAYER_ACTION_ID( LocomotionAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

    private:

        void UpdateIdle( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );
        void UpdateTurnOnSpot( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );
        void UpdateMoving( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );
        void UpdateStopping( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

    private:

        ManualCountdownTimer        m_sprintTimer;
        ManualCountdownTimer        m_crouchTimer;
        ManualTimer                 m_slideTimer;
        ManualCountdownTimer        m_generalTimer;
        bool                        m_crouchToggled = false;

        Vector                      m_desiredHeading = Vector::Zero;
        Vector                      m_desiredFacing = Vector::WorldForward;
        Vector                      m_cachedFacing = Vector::Zero;
        Vector                      m_desiredTurnDirection = Vector::Zero;
        LocomotionState             m_state = LocomotionState::Idle;
    };
}
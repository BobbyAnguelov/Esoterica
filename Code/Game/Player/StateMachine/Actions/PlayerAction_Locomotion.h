#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "System/Time/Timers.h"
#include "System/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class LocomotionAction final : public Action
    {
        enum class LocomotionState
        {
            Idle,
            Starting,
            Moving,
            TurningOnSpot,
            PlantingAndTurning,
            Stopping
        };

    public:

        EE_PLAYER_ACTION_ID( LocomotionAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

    private:

        void RequestIdle( ActionContext const& ctx );
        void UpdateIdle( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestStart( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );
        void UpdateStarting( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestTurnOnSpot( ActionContext const& ctx, Vector const& desiredFacingDirection );
        void UpdateTurnOnSpot( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestMoving( ActionContext const& ctx, Vector const& initialVelocity );
        void UpdateMoving( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestStop( ActionContext const& ctx );
        void UpdateStopping( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebugUI() override;
        #endif

    private:

        ManualTimer                 m_slideTimer;
        ManualCountdownTimer        m_generalTimer;
        ManualCountdownTimer        m_turnOnSpotTimer;

        Vector                      m_desiredHeading = Vector::Zero;
        Vector                      m_desiredFacing = Vector::WorldForward;
        Vector                      m_cachedFacing = Vector::Zero;
        Vector                      m_desiredTurnDirection = Vector::Zero;
        LocomotionState             m_state = LocomotionState::Idle;

        #if EE_DEVELOPMENT_TOOLS
        bool                        m_enableVisualizations = true;
        #endif
    };
}
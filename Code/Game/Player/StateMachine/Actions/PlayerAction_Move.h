#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerAction_Move final : public PlayerAction
    {
        enum class LocomotionState
        {
            Idle,
            TurningOnSpot,
            Starting,
            Moving,
            PlantingAndTurning,
            Stopping
        };

    public:

        EE_PLAYER_ACTION_ID( PlayerAction_Move );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

    private:

        void SetCrouchState( PlayerActionContext const& ctx, bool isCrouchEnabled );

        void RequestIdle( PlayerActionContext const& ctx );
        void UpdateIdle( PlayerActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestStart( PlayerActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );
        void UpdateStarting( PlayerActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestTurnOnSpot( PlayerActionContext const& ctx, Vector const& desiredFacingDirection );
        void UpdateTurnOnSpot( PlayerActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestMoving( PlayerActionContext const& ctx, Vector const& initialVelocity );
        void UpdateMoving( PlayerActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        void RequestStop( PlayerActionContext const& ctx );
        void UpdateStopping( PlayerActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude );

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebugUI() override;
        #endif

    private:

        ManualTimer                 m_unnavigableSurfaceSlideTimer;
        ManualCountdownTimer        m_startDetectionTimer;
        ManualCountdownTimer        m_stopDetectionTimer;

        Vector                      m_desiredMovementVelocity = Vector::Zero;
        Vector                      m_desiredFacing = Vector::WorldForward;
        Vector                      m_cachedFacing = Vector::Zero;
        Vector                      m_desiredTurnDirection = Vector::Zero;
        LocomotionState             m_state = LocomotionState::Idle;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        float                       m_debug_desiredSpeed = 0.0f;
        float                       m_debug_moveAngularVelocityLimit = 0.0f;
        bool                        m_areVisualizationsEnabled = false;
        #endif
    };
}
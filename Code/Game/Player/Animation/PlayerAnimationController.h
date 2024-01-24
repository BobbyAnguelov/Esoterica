#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"
#include "Engine/Animation/Events/AnimationEvent_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class AnimationController final : public Animation::GraphController
    {

    public:

        enum class CharacterState : uint8_t
        {
            Locomotion = 0,
            InAir,
            Ability,
            Interaction,
            GhostMode,

            NumStates,
        };

        enum class LocomotionState : int8_t
        {
            Unknown = -1,
            Idle = 0,
            TurnOnSpot,
            Start,
            Move,
            PlantedTurn,
            Stop,

            NumStates,
        };

        enum class InAirState : int8_t
        {
            None = -1,
            Falling = 0,
            SoftLanding,
            HardLanding,

            NumStates,
        };

        enum class WeaponState : uint8_t
        {
            None = 0,
            Drawing,
            Drawn,
            Aimed,
            Holstering,
            MeleeAttack,

            NumStates,
        };

    private:

        static StringID const s_characterStateIDs[ (uint8_t) CharacterState::NumStates];
        static StringID const s_locomotionStateIDs[ (uint8_t) LocomotionState::NumStates];
        static StringID const s_inAirStateParameterIDs[(uint8_t) LocomotionState::NumStates];
        static StringID const s_weaponStateIDs[ (uint8_t) WeaponState::NumStates];

    public:

        AnimationController( Animation::GraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent );

        virtual void PostGraphUpdate( Seconds deltaTime ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetName() const override { return "Player Graph Controller"; }
        #endif

        // General Player State
        //-------------------------------------------------------------------------

        void SetCharacterState( CharacterState state );

        // Generic Transition Info
        //-------------------------------------------------------------------------

        inline bool HasTransitionEvent() const { return !m_transitionEvents.empty(); }

        bool IsAnyTransitionAllowed( StringID specificID ) const { return IsTransitionFullyAllowed( specificID ) || IsTransitionConditionallyAllowed( specificID ); }
        bool IsTransitionFullyAllowed( StringID specificID ) const;
        bool IsTransitionConditionallyAllowed( StringID specificID ) const;

        // Locomotion
        //-------------------------------------------------------------------------

        void RequestIdle();
        void RequestTurnOnSpot( Vector const& directionWS );
        void RequestStart( Vector const& movementVelocityWS );
        void RequestMove( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );
        void RequestPlantedTurn( Vector const& directionWS );
        void RequestStop( Transform const& target );

        void SetCrouch( bool isCrouch );
        void SetSliding( bool isSliding );

        bool IsLocomotionTransitionFullyAllowed() const;
        bool IsLocomotionTransitionConditionallyAllowed() const;
        inline bool IsLocomotionTransitionAllowed() const { return IsLocomotionTransitionFullyAllowed() || IsLocomotionTransitionConditionallyAllowed(); }

        // State check
        inline bool IsTransitionAllowed() const { return m_isAllowedToTransition; }
        inline bool IsIdle() const { return m_graphState == LocomotionState::Idle; }
        inline bool IsTurningOnSpot() const { return m_graphState == LocomotionState::TurnOnSpot; }
        inline bool IsStarting() const { return m_graphState == LocomotionState::Start; }
        inline bool IsMoving() const { return m_graphState == LocomotionState::Move; }
        inline bool IsPlantingAndTurning() const { return m_graphState == LocomotionState::PlantedTurn; }
        inline bool IsStopping() const { return m_graphState == LocomotionState::Stop; }

        // In-Air
        //-------------------------------------------------------------------------

        // Set the air control movement desires
        void SetInAirDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );

        // Weapons
        //-------------------------------------------------------------------------

        void DrawWeapon();
        void HolsterWeapon();

        bool IsWeaponDrawn() const { return m_isWeaponDrawn; }

        void AimWeapon( Vector const& targetWS );
        void FireWeapon();

        inline bool IsInMeleeAttack() const { return m_isInMeleeAttack; }

        void StartMeleeAttack();

        // Abilities
        //-------------------------------------------------------------------------

        void StartJump();
        void StartDash();
        void StartSlide();

        void SetAbilityDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS );

        // Hit Reactions
        //-------------------------------------------------------------------------

        void TriggerHitReaction(); // HACK
        void ClearHitReaction(); // HACK
        bool IsHitReactionComplete() const { return m_isHitReactionComplete; }

    private:

        ControlParameter<StringID>                          m_characterState = "CharacterState";

        // Locomotion
        //-------------------------------------------------------------------------

        ControlParameter<StringID>                          m_locomotionState = "Locomotion_State";
        ControlParameter<float>                             m_locomotionSpeed = "Locomotion_Speed";
        ControlParameter<Vector>                            m_locomotionMoveDir = "Locomotion_MoveDirection";
        ControlParameter<Vector>                            m_locomotionMoveVelocity = "Locomotion_MoveVelocity";
        ControlParameter<Vector>                            m_locomotionFacing = "Locomotion_Facing";
        ControlParameter<bool>                              m_locomotionCrouchState = "Locomotion_IsCrouched";
        ControlParameter<bool>                              m_locomotionAutoSlidingState = "Locomotion_IsAutoSliding";

        LocomotionState                                     m_graphState = LocomotionState::Unknown;
        bool                                                m_isAllowedToTransition = false;

        // In-Air
        //-------------------------------------------------------------------------

        ControlParameter<StringID>                          m_inAirState = "InAir_State";
        ControlParameter<float>                             m_inAirSpeed = "InAir_Speed";
        ControlParameter<Vector>                            m_inAirMoveDir = "InAir_MoveDirection";
        ControlParameter<Vector>                            m_inAirMoveVelocity = "InAir_MoveVelocity";
        ControlParameter<Vector>                            m_inAirFacing = "InAir_Facing";

        // Weapons
        //-------------------------------------------------------------------------

        ControlParameter<StringID>                          m_weaponState = "Weapon_State";
        ControlParameter<bool>                              m_weaponFireSignal = "Weapon_Fire";
        ControlParameter<float>                             m_weaponAimAngleHorizontal = "Weapon_AimAngleHorizontal";
        ControlParameter<float>                             m_weaponAimAngleVertical = "Weapon_AimAngleVertical";

        bool                                                m_isWeaponDrawn;
        bool                                                m_isInMeleeAttack = false;

        // Abilities
        //-------------------------------------------------------------------------

        ControlParameter<StringID>                          m_abilityState = "Ability_State";
        ControlParameter<float>                             m_abilitySpeed = "Ability_Speed";
        ControlParameter<Vector>                            m_abilityMoveDir = "Ability_MoveDirection";
        ControlParameter<Vector>                            m_abilityMoveVelocity = "Ability_MoveVelocity";
        ControlParameter<Vector>                            m_abilityFacing = "Ability_Facing";

        //-------------------------------------------------------------------------

        ControlParameter<StringID>                          m_hitReactionState = "HitReaction_State";
        bool                                                m_isHitReactionComplete = false;

        //-------------------------------------------------------------------------

        TVector<Animation::TransitionEvent const*>          m_transitionEvents;
    };
}
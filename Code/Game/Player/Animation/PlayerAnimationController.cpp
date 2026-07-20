#include "PlayerAnimationController.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE
{
    StaticStringID const PlayerAnimationController::s_characterStateIDs[] =
    {
        StaticStringID( "CS_Locomotion" ),
        StaticStringID( "CS_InAir" ),
        StaticStringID( "CS_Ability" ),
        StaticStringID( "CS_Interaction" ),
        StaticStringID( "CS_GhostMode" ),
    };

    StaticStringID const PlayerAnimationController::s_locomotionStateIDs[] =
    {
        StaticStringID( "Locomotion_Idle" ),
        StaticStringID( "Locomotion_TurnOnSpot" ),
        StaticStringID( "Locomotion_Start" ),
        StaticStringID( "Locomotion_Move" ),
        StaticStringID( "Locomotion_PlantedTurn" ),
        StaticStringID( "Locomotion_Stop" ),
    };

    StaticStringID const PlayerAnimationController::s_inAirStateParameterIDs[] =
    {
        StaticStringID( "InAir_Falling" ),
        StaticStringID( "InAir_SoftLanding" ),
        StaticStringID( "InAir_HardLanding" )
    };

    StaticStringID const PlayerAnimationController::s_weaponStateIDs[] =
    {
        StaticStringID( "Weapon_None" ),
        StaticStringID( "Weapon_Draw" ),
        StaticStringID( "Weapon_Drawn" ),
        StaticStringID( "Weapon_Aimed" ),
        StaticStringID( "Weapon_Holster" ),
        StaticStringID( "Weapon_WeaponAttack" ),
    };

    namespace AbilityIDs
    {
        static StaticStringID g_jumpID( "Ability_Jump" );
        static StaticStringID g_dashID( "Ability_Dash" );
        static StaticStringID g_slideID( "Ability_Slide" );
    }

    StaticStringID const PlayerAnimationController::s_locomotionTransitionMarkerID( "Locomotion" );

    //-------------------------------------------------------------------------

    PlayerAnimationController::PlayerAnimationController( Animation::GraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::GraphController( pGraphComponent, pMeshComponent )
    {
        m_characterState.TryBind( this );

        //-------------------------------------------------------------------------

        m_locomotionState.TryBind( this );
        m_locomotionSpeed.TryBind( this );
        m_locomotionMoveVelocity.TryBind( this );
        m_locomotionMoveDir.TryBind( this );
        m_locomotionFacing.TryBind( this );
        m_locomotionCrouchState.TryBind( this );
        m_locomotionAutoSlidingState.TryBind( this );

        //-------------------------------------------------------------------------

        m_inAirState.TryBind( this );
        m_inAirSpeed.TryBind( this );
        m_inAirMoveDir.TryBind( this );
        m_inAirMoveVelocity.TryBind( this );
        m_inAirFacing.TryBind( this );

        //-------------------------------------------------------------------------

        m_weaponState.TryBind( this );
        m_weaponFireSignal.TryBind( this );
        m_weaponAimAngleHorizontal.TryBind( this );
        m_weaponAimAngleVertical.TryBind( this );

        //-------------------------------------------------------------------------

        m_abilityState.TryBind( this );
        m_abilitySpeed.TryBind( this );
        m_abilityMoveDir.TryBind( this );
        m_abilityMoveVelocity.TryBind( this );
        m_abilityFacing.TryBind( this );

        //-------------------------------------------------------------------------

        m_hitReactionState.TryBind( this );
    }

    //-------------------------------------------------------------------------

    bool PlayerAnimationController::IsTransitionFullyAllowed( StringID specificID ) const
    {
        for ( auto const& pSampledEvent : m_transitionEvents )
        {
            if ( pSampledEvent->GetOptionalID() == specificID && pSampledEvent->GetRule() == Animation::TransitionRule::AllowTransition )
            {
                return true;
            }
        }

        return false;
    }

    bool PlayerAnimationController::IsTransitionConditionallyAllowed( StringID specificID ) const
    {
        for ( auto const& pSampledEvent : m_transitionEvents )
        {
            if ( pSampledEvent->GetOptionalID() == specificID && pSampledEvent->GetRule() == Animation::TransitionRule::ConditionallyAllowTransition )
            {
                return true;
            }
        }

        return false;
    }

    void PlayerAnimationController::PostGraphUpdate( Seconds deltaTime )
    {
        Animation::GraphController::PostGraphUpdate( deltaTime );

        //-------------------------------------------------------------------------

        m_transitionEvents.clear();

        //-------------------------------------------------------------------------

        m_graphState = LocomotionState::Unknown;
        m_isAllowedToTransition = false;

        m_weaponFireSignal.Set( false );
        m_isWeaponDrawn = false;
        m_isInMeleeAttack = false;
        m_isHitReactionComplete = false;

        //-------------------------------------------------------------------------

        for ( auto const& sampledEvent : GetSampledEvents() )
        {
            if ( sampledEvent.IsIgnored() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( sampledEvent.IsAnimationEvent() )
            {
                if ( sampledEvent.IsFromActiveBranch() )
                {
                    if ( auto pTransitionEvent = sampledEvent.TryGetEvent<Animation::TransitionEvent>() )
                    {
                        m_transitionEvents.emplace_back( pTransitionEvent );
                    }
                }
            }
            else // Graph events
            {
                StringID const stateID = sampledEvent.GetGraphEventID();

                // Locomotion
                if ( sampledEvent.IsEntryEvent() || sampledEvent.IsFullyInStateEvent() )
                {
                    if ( stateID == s_locomotionStateIDs[(int8_t) LocomotionState::Idle] )
                    {
                        m_graphState = LocomotionState::Idle;
                    }
                    else if ( stateID == s_locomotionStateIDs[(int8_t) LocomotionState::TurnOnSpot] )
                    {
                        m_graphState = LocomotionState::TurnOnSpot;
                    }
                    else if ( stateID == s_locomotionStateIDs[(int8_t) LocomotionState::Start] )
                    {
                        m_graphState = LocomotionState::Start;
                    }
                    else if ( stateID == s_locomotionStateIDs[(int8_t) LocomotionState::Move] )
                    {
                        m_graphState = LocomotionState::Move;
                    }
                    else if ( stateID == s_locomotionStateIDs[(int8_t) LocomotionState::PlantedTurn] )
                    {
                        m_graphState = LocomotionState::PlantedTurn;
                    }
                    else if ( stateID == s_locomotionStateIDs[(int8_t) LocomotionState::Stop] )
                    {
                        m_graphState = LocomotionState::Stop;
                    }
                }

                // Weapon
                //-------------------------------------------------------------------------

                if ( stateID == s_weaponStateIDs[(uint8_t) WeaponState::Drawn] || stateID == s_weaponStateIDs[(uint8_t) WeaponState::Aimed] )
                {
                    m_isWeaponDrawn = true;
                }

                if ( sampledEvent.IsFullyInStateEvent() && stateID == s_weaponStateIDs[(uint8_t) WeaponState::MeleeAttack] )
                {
                    m_isInMeleeAttack = true;
                }

                // Hit Reactions
                //-------------------------------------------------------------------------

                static StringID const hitReactionCompleteEventID( "HR_HitReactionComplete" );
                if ( sampledEvent.IsTimedEvent() && stateID == hitReactionCompleteEventID )
                {
                    m_isHitReactionComplete = true;
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    // General
    //-------------------------------------------------------------------------

    void PlayerAnimationController::SetCharacterState( CharacterState state )
    {
        EE_ASSERT( state < CharacterState::NumStates );
        m_characterState.Set( s_characterStateIDs[(uint8_t) state] );
    }

    //-------------------------------------------------------------------------
    // Locomotion
    //-------------------------------------------------------------------------

    void PlayerAnimationController::RequestIdle()
    {
        m_locomotionState.Set( s_locomotionStateIDs[(int8_t) LocomotionState::Idle] );
        m_locomotionMoveVelocity.Set( Vector::Zero );
        m_locomotionMoveDir.Set( Vector::Zero );
        m_locomotionFacing.Set( Vector::WorldForward );
        m_locomotionSpeed.Set( 0.0f );
    }

    void PlayerAnimationController::RequestTurnOnSpot( Vector const& directionWS )
    {
        m_locomotionState.Set( s_locomotionStateIDs[(int8_t) LocomotionState::TurnOnSpot] );
        m_locomotionMoveVelocity.Set( Vector::Zero );
        m_locomotionMoveDir.Set( Vector::Zero );
        m_locomotionFacing.Set( ConvertWorldSpaceVectorToCharacterSpace( directionWS ) );
        m_locomotionSpeed.Set( 0.0f );
    }

    void PlayerAnimationController::RequestStart( Vector const& movementVelocityWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );

        float speed;
        Vector movementDirCS;
        movementVelocityCS.ToDirectionAndLength3( movementDirCS, speed );

        //-------------------------------------------------------------------------

        m_locomotionState.Set( s_locomotionStateIDs[(int8_t) LocomotionState::Start] );
        m_locomotionMoveVelocity.Set( movementVelocityCS );
        m_locomotionMoveDir.Set( movementDirCS );
        m_locomotionFacing.Set( Vector::WorldForward );
        m_locomotionSpeed.Set( speed );
    }

    void PlayerAnimationController::RequestMove( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );

        float speed;
        Vector movementDirCS;
        movementVelocityCS.ToDirectionAndLength3( movementDirCS, speed );

        //-------------------------------------------------------------------------

        Vector facingCS = Vector::WorldForward;
        if ( !facingDirectionWS.IsZero3() )
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            facingCS = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
        }

        //-------------------------------------------------------------------------

        m_locomotionState.Set( s_locomotionStateIDs[(int8_t) LocomotionState::Move] );
        m_locomotionMoveVelocity.Set( movementVelocityCS );
        m_locomotionMoveDir.Set( movementDirCS );
        m_locomotionFacing.Set( facingCS );
        m_locomotionSpeed.Set( speed );
    }

    void PlayerAnimationController::RequestPlantedTurn( Vector const& directionWS )
    {
        EE_ASSERT( directionWS.IsNormalized3() );
        m_locomotionState.Set( s_locomotionStateIDs[(int8_t) LocomotionState::PlantedTurn] );
        m_locomotionMoveVelocity.Set( directionWS );
        m_locomotionMoveDir.Set( directionWS );
    }

    void PlayerAnimationController::RequestStop( Transform const& target )
    {
        m_locomotionState.Set( s_locomotionStateIDs[(int8_t) LocomotionState::Stop] );
        m_locomotionMoveVelocity.Set( Vector::Zero );
        m_locomotionMoveDir.Set( Vector::Zero );
        m_locomotionFacing.Set( Vector::WorldForward );
        m_locomotionSpeed.Set( 0.0f );
    }

    void PlayerAnimationController::SetCrouch( bool isCrouch )
    {
        m_locomotionCrouchState.Set( isCrouch );
    }

    void PlayerAnimationController::SetSliding( bool isSliding )
    {
        m_locomotionAutoSlidingState.Set( isSliding );
    }

    bool PlayerAnimationController::IsLocomotionTransitionFullyAllowed() const
    {
        return IsTransitionFullyAllowed( s_locomotionTransitionMarkerID );
    }

    bool PlayerAnimationController::IsLocomotionTransitionConditionallyAllowed() const
    {
        return IsTransitionFullyAllowed( s_locomotionTransitionMarkerID );
    }

    //-------------------------------------------------------------------------
    // In Air
    //-------------------------------------------------------------------------

    void PlayerAnimationController::SetInAirDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );

        if ( movementVelocityCS.IsNearZero3() )
        {
            m_inAirMoveDir.Set( Vector::WorldForward );
            m_inAirMoveVelocity.Set( Vector::Zero );
            m_inAirSpeed.Set( 0.0f );
        }
        else
        {
            float speed;
            Vector movementDirCS;
            movementVelocityCS.ToDirectionAndLength3( movementDirCS, speed );
            m_inAirMoveDir.Set( movementDirCS );
            m_inAirMoveVelocity.Set( movementVelocityCS );
            m_inAirSpeed.Set( speed );
        }

        //-------------------------------------------------------------------------

        if ( facingDirectionWS.IsZero3() )
        {
            m_inAirFacing.Set( Vector::WorldForward );
        }
        else
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            Vector const characterSpaceFacing = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
            m_inAirFacing.Set( characterSpaceFacing );
        }
    }

    //-------------------------------------------------------------------------
    // Weapons
    //-------------------------------------------------------------------------

    void PlayerAnimationController::FireWeapon()
    {
        m_weaponFireSignal.Set( true );
    }

    void PlayerAnimationController::DrawWeapon()
    {
        m_weaponState.Set( s_weaponStateIDs[(uint8_t) WeaponState::Drawing] );
    }

    void PlayerAnimationController::HolsterWeapon()
    {
        m_weaponState.Set( s_weaponStateIDs[(uint8_t) WeaponState::Holstering] );
    }

    void PlayerAnimationController::AimWeapon( Vector const& targetWS )
    {
        float angleH = 0.0f;
        float angleV = 0.0f;
        m_weaponState.Set( s_weaponStateIDs[(uint8_t) WeaponState::Aimed] );

        // This is stupid but it's a demo
        Animation::Pose const* pPose = GetCurrentPose();
        int32_t const headIdx = pPose->GetSkeleton()->GetBoneIndex( StringID( "head" ) );
        if ( headIdx != InvalidIndex )
        {
            Vector headPos = pPose->GetModelSpaceTransform( headIdx ).GetTranslation();
            Vector targetPosCS = ConvertWorldSpacePointToCharacterSpace( targetWS );
            Vector aimDir = ( targetPosCS - headPos ).GetNormalized3();

            angleH = Math::CalculateYawAngleBetweenVectors( Vector::WorldForward, aimDir ).ToDegrees().ToFloat();
            angleV = Math::CalculatePitchAngleBetweenUnitVectors( Vector::WorldForward, aimDir ).ToDegrees().ToFloat();
        }

        m_weaponAimAngleHorizontal.Set( angleH );
        m_weaponAimAngleVertical.Set( angleV );
    }

    void PlayerAnimationController::StartMeleeAttack()
    {
        static StringID const meleeAttackID( "MeleeAttack" );
        m_weaponState.Set( meleeAttackID );
    }

    //-------------------------------------------------------------------------
    // Abilities
    //-------------------------------------------------------------------------

    void PlayerAnimationController::StartJump()
    {
        m_abilityState.Set( AbilityIDs::g_jumpID );
    }

    void PlayerAnimationController::StartDash()
    {
        m_abilityState.Set( AbilityIDs::g_dashID );
    }

    void PlayerAnimationController::StartSlide()
    {
        m_abilityState.Set( AbilityIDs::g_slideID );
    }

    void PlayerAnimationController::SetAbilityDesiredMovement( Seconds const deltaTime, Vector const& movementVelocityWS, Vector const& facingDirectionWS )
    {
        Vector const movementVelocityCS = ConvertWorldSpaceVectorToCharacterSpace( movementVelocityWS );
        
        float speed;
        Vector movementDirCS;
        movementVelocityCS.ToDirectionAndLength3( movementDirCS, speed );

        m_abilityMoveVelocity.Set( movementVelocityCS );
        m_abilityMoveDir.Set( movementDirCS );
        m_abilitySpeed.Set( speed );

        //-------------------------------------------------------------------------

        if ( facingDirectionWS.IsZero3() )
        {
            m_abilityFacing.Set( Vector::WorldForward );
        }
        else
        {
            EE_ASSERT( facingDirectionWS.IsNormalized3() );
            Vector const characterSpaceFacing = ConvertWorldSpaceVectorToCharacterSpace( facingDirectionWS ).GetNormalized2();
            m_abilityFacing.Set( characterSpaceFacing );
        }
    }

    //-------------------------------------------------------------------------
    // Hit Reactions
    //-------------------------------------------------------------------------

    void PlayerAnimationController::TriggerHitReaction()
    {
        m_hitReactionState.Set( StringID( "HR_Flinch" ) );
    }

    void PlayerAnimationController::ClearHitReaction()
    {
        m_hitReactionState.Set( StringID() );
    }
}
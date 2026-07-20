#include "Behavior_HackMove.h"
#include "Base/Math/MathRandom.h"

//-------------------------------------------------------------------------

namespace EE
{
    void HackPrototypeBehavior::StartInternal( BehaviorContext const& ctx )
    {
        ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Locomotion );

        m_waitTimer.Start( 0.1f );
        m_isWaiting = true;
    }

    Behavior::Status HackPrototypeBehavior::UpdateInternal( BehaviorContext const& ctx )
    {
        if ( m_waitTimer.IsRunning() )
        {
            ctx.m_pAnimationController->HACK_SetIdle( true );

            // Wait for the timer to elapse and start a move
            if ( m_waitTimer.Update( ctx.GetDeltaTime() ) )
            {
                FloatRange const validRange( -40, 40 );

                Vector startPos = ctx.m_pNPC->GetWorldTransform().GetTranslation();
                startPos = Vector::Clamp( startPos, Vector( validRange.m_begin + 1 ), Vector( validRange.m_end - 1 ) );

                Vector goalPos = startPos;

                do 
                {
                    Vector const deltaPos( Math::GetRandomFloat( -15.0f, 15.0f ), Math::GetRandomFloat( -15.0f, 15.0f ), 0.0f );
                    goalPos = startPos + deltaPos;

                } while ( !validRange.ContainsInclusive( goalPos.GetX() ) || !validRange.ContainsInclusive( goalPos.GetY() ) );

                //-------------------------------------------------------------------------

                m_path = LineSegment( startPos, goalPos );
                m_distanceAlongPath = 0;
                m_isWaiting = false;
            }
        }
        #if 0
        else
        {
            ctx.m_pAnimationController->HACK_SetIdle( true );
        }
        #else
        else // We're moving
        {
            ctx.m_pAnimationController->HACK_SetIdle( false );

            float deltaDistance = ctx.GetDeltaTime() * 5.0f;
            m_distanceAlongPath += deltaDistance;

            Transform characterTransform = ctx.m_pNPC->GetWorldTransform();
            Vector const currentPos = characterTransform.GetTranslation();
            Vector const characterPos = m_path.GetPointAtDistance( m_distanceAlongPath );

            Vector delta( characterPos - currentPos );
            delta.SetZ( 0 );

            if ( !delta.IsNearZero3() )
            {
                delta.Normalize3();
                characterTransform.SetTranslation( characterPos );
                characterTransform.SetRotation( Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, delta ) );
                ctx.m_pNPC->TeleportCharacter( characterTransform );
            }

            if ( m_distanceAlongPath >= m_path.GetLength() )
            {
                m_waitTimer.Start( Math::GetRandomFloat( 2.0f, 5.0f ) );
                m_isWaiting = true;
            }
        }
        #endif

        //-------------------------------------------------------------------------

        return Status::Running;
    }

    void HackPrototypeBehavior::StopInternal( BehaviorContext const& ctx, StopReason reason )
    {

    }
}
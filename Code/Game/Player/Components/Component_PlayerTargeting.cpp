#include "Component_PlayerTargeting.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Hitbox/Components/Component_Hitbox.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Intersection.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PlayerTargetingComponent::Target::ResetScoreState()
    {
        m_angularDistance = 0;
        m_distance = 0;
        m_angleScore = 0;
        m_distanceScore = 0;
        m_score = 0;
    }

    void PlayerTargetingComponent::LockOn()
    {
        if ( m_trackedTargetEntityID.IsValid() )
        {
            m_lockedOnTargetEntityID = m_trackedTargetEntityID;
        }
    }

    void PlayerTargetingComponent::CancelLockOn()
    {
        m_lockedOnTargetEntityID.Clear();
    }

    PlayerTargetingComponent::Target* PlayerTargetingComponent::GetTrackedTarget()
    {
        EE_ASSERT( HasTrackedTarget() );

        for ( auto& target : m_reflectedTargets )
        {
            if ( target.m_entityID == m_trackedTargetEntityID )
            {
                return &target;
            }
        }

        return nullptr;
    }

    void PlayerTargetingComponent::ReflectTargets( Seconds deltaTime, Seconds unscaledDeltaTime, Math::ViewVolume const& viewVolume, TInlineVector<HitboxComponent const*, 20> const& targetHitboxes )
    {
        m_viewPos = viewVolume.GetViewPosition();
        m_viewForward = viewVolume.GetViewForwardVector();
        m_viewRight = viewVolume.GetViewRightVector();
        m_deltaTime = unscaledDeltaTime;

        m_maxDistance = -FLT_MAX;
        m_lowestAngularDistance = FLT_MAX;
        m_targetBounds.Reset();

        //-------------------------------------------------------------------------
        // Reflect Targets
        //-------------------------------------------------------------------------

        // Remove invalid targets
        for ( int32_t i = int32_t( m_reflectedTargets.size() ) - 1; i >= 0; i-- )
        {
            // Delete targets that are not in the new list
            auto foundIter = VectorFind( targetHitboxes, m_reflectedTargets[i].m_entityID, [] ( HitboxComponent const* pComponent, EntityID const& ID ) { return pComponent->GetEntityID() == ID; } );
            if ( foundIter == targetHitboxes.end() )
            {
                m_reflectedTargets.erase_unsorted( m_reflectedTargets.begin() + i );
            }
        }

        // Update and add new targets
        for ( auto const& pHitboxComponent : targetHitboxes )
        {
            Target* pTarget = nullptr;

            auto foundIter = VectorFind( m_reflectedTargets, pHitboxComponent->GetEntityID(), [] ( Target const& target, EntityID const& ID ) { return target.m_entityID == ID; } );
            if ( foundIter == m_reflectedTargets.end() )
            {
                pTarget = &m_reflectedTargets.emplace_back();
                pTarget->m_entityID = pHitboxComponent->GetEntityID();
                pTarget->m_headTrackingWeight.SetDesiredBlendTime( m_headFocusTime );
            }
            else
            {
                pTarget = foundIter;
                pTarget->ResetScoreState();
            }

            // Update basic target data
            Hitbox const* pHitbox = pHitboxComponent->GetHitbox();
            EE_ASSERT( pTarget != nullptr && pTarget->m_entityID == pHitboxComponent->GetEntityID() );
            pTarget->m_pHitbox = pHitbox;
            pTarget->m_bounds = pHitbox->GetBounds();
        }

        // Delete target outside of the view
        for ( int32_t i = int32_t( m_reflectedTargets.size() ) - 1; i >= 0; i-- )
        {
            if ( viewVolume.Contains( m_reflectedTargets[i].m_bounds ) )
            {
                m_reflectedTargets[i].m_timeOffScreen = 0.0f;
                continue;
            }

            if ( m_lockedOnTargetEntityID == m_reflectedTargets[i].m_entityID )
            {
                // If we've been offscreen for too long, disable the lock on
                if ( m_reflectedTargets[i].m_timeOffScreen > m_lockOnOffscreenGracePeriod )
                {
                    m_lockedOnTargetEntityID.Clear();
                    m_reflectedTargets.erase_unsorted( m_reflectedTargets.begin() + i );
                }
                else
                {
                    m_reflectedTargets[i].m_timeOffScreen += m_deltaTime;
                }
            }
            else // Delete out of view targets
            {
                m_reflectedTargets.erase_unsorted( m_reflectedTargets.begin() + i );
            }
        }

        //-------------------------------------------------------------------------
        // Update Targeting
        //-------------------------------------------------------------------------

        if ( PickBestTargetEntity() )
        {
            UpdateTargetedZone();
            ApplyVelocityModifiersToTrackedTarget();
        }

        //-------------------------------------------------------------------------

        for ( auto& target : m_reflectedTargets )
        {
            target.m_pHitbox = nullptr;
        }
    }

    bool PlayerTargetingComponent::PickBestTargetEntity()
    {
        EntityID previousLockedTargetID = m_lockedOnTargetEntityID;
        m_lockedOnTargetEntityID.Clear();

        float highestScore = -1.0f;
        int32_t highestScoreTargetIdx = InvalidIndex;

        int32_t const numTargets = (int32_t) m_reflectedTargets.size();
        for ( int32_t i = 0; i < numTargets; i++ )
        {
            auto& target = m_reflectedTargets[i];

            // Calculate distances
            //-------------------------------------------------------------------------

            Vector const viewToTarget = Vector( target.m_bounds.GetCenter() - m_viewPos );

            target.m_distance = viewToTarget.GetLength3();
            target.m_angularDistance = Math::CalculateAngleBetweenUnitVectors( m_viewForward, viewToTarget.GetNormalized3() );

            m_maxDistance = Math::Max( m_maxDistance, target.m_distance );
            m_lowestAngularDistance = Math::Min( m_lowestAngularDistance, target.m_angularDistance );

            // Try to keep previous target
            //-------------------------------------------------------------------------

            if ( !m_lockedOnTargetEntityID.IsValid() && previousLockedTargetID.IsValid() && target.m_entityID == previousLockedTargetID )
            {
                m_lockedOnTargetEntityID = previousLockedTargetID;
            }
        }

        // Calculate scores
        //-------------------------------------------------------------------------
        // Needs to run as a second pass as we need the max distance

        for ( int32_t i = 0; i < numTargets; i++ )
        {
            auto& target = m_reflectedTargets[i];

            target.m_angleScore = 1.0f - Math::Min( target.m_angularDistance.ToFloat() / 90, 1.0f );
            target.m_distanceScore = 1.0f - ( target.m_distance / m_maxDistance );
            target.m_score = ( target.m_angleScore + ( target.m_distanceScore * m_distanceBias ) ) / 2;

            if ( target.m_entityID == m_lockedOnTargetEntityID )
            {
                target.m_score += 1.0f;
            }

            //-------------------------------------------------------------------------

            if ( target.m_score >= highestScore )
            {
                highestScore = target.m_score;
                highestScoreTargetIdx = i;
            }
        }

        // Final Selection
        //-------------------------------------------------------------------------

        m_trackedTargetEntityID.Clear();

        if ( highestScoreTargetIdx != InvalidIndex )
        {
            for ( int32_t t = 0; t < numTargets; t++ )
            {
                if ( t == highestScoreTargetIdx )
                {
                    m_reflectedTargets[t].m_timeTracked += m_deltaTime;
                    m_trackedTargetEntityID = m_reflectedTargets[t].m_entityID;
                }
                else
                {
                    m_reflectedTargets[t].m_timeTracked = 0;
                }
            }
        }

        return m_trackedTargetEntityID.IsValid();
    }

    void PlayerTargetingComponent::UpdateTargetedZone()
    {
        Target* pTrackedTarget = GetTrackedTarget();

        // Check for headshot
        //-------------------------------------------------------------------------

        AABB headBounds;
        Vector headVelocity = Vector::Zero;

        auto const matchingShapes = pTrackedTarget->m_pHitbox->FindMatchingShapes( m_headTag );
        if ( !matchingShapes.empty() && matchingShapes[0]->m_pShapeDef->m_type == HitboxShape::Type::Sphere )
        {
            auto pHeadShape = ( Hitbox::Sphere const* ) matchingShapes[0];
            headBounds = pHeadShape->m_bounds;
            headVelocity = pHeadShape->m_velocity;

            #if EE_DEVELOPMENT_TOOLS
            m_headTrackingSpherePos = pHeadShape->m_center;
            #endif

            Vector const offset = ( m_viewRight * m_headTrackingShapeHalfLength );
            Vector const capsulePoint0 = pHeadShape->m_center - offset;
            Vector const capsulePoint1 = pHeadShape->m_center + offset;
            Math::RayCapsuleResult const result = Math::IntersectRayCapsule( Ray( m_viewPos, m_viewForward ), capsulePoint0, capsulePoint1, m_headTrackingShapeRadius );
            if ( result )
            {
                pTrackedTarget->m_headTrackingWeight.Update( m_deltaTime, true );
                pTrackedTarget->m_headTrackingGraceTime = 0.0f;
            }
            else
            {
                // Update grace period timer
                if ( pTrackedTarget->m_headTrackingGraceTime < m_headTrackingGracePeriod && pTrackedTarget->m_headTrackingWeight.GetWeight() == 1.0f )
                {
                    pTrackedTarget->m_headTrackingGraceTime += m_deltaTime;
                }
                else // Just blend out
                {
                    pTrackedTarget->m_headTrackingGraceTime = 0.0f;
                    pTrackedTarget->m_headTrackingWeight.Update( m_deltaTime, false );
                }
            }
        }

        // Check tracked time
        //-------------------------------------------------------------------------

        if ( pTrackedTarget->m_headTrackingWeight.GetWeight() >= 1.0f )
        {
            m_targetBounds = headBounds;
            m_targetVelocity = headVelocity;
            return;
        }

        // Calculate body target
        //-------------------------------------------------------------------------

        if ( pTrackedTarget->m_timeTracked > m_torsoFocusTime )
        {
            m_targetBounds = pTrackedTarget->m_pHitbox->GetBoundsForMatchingTag( m_torsoTag );
            m_targetVelocity = pTrackedTarget->m_pHitbox->GeAverageVelocityForMatchingTag( m_torsoTag );
        }
        else if ( pTrackedTarget->m_timeTracked > m_torsoStartFocusTime )
        {
            AABB const torsoBounds = pTrackedTarget->m_pHitbox->GetBoundsForMatchingTag( m_torsoTag );
            m_targetVelocity = pTrackedTarget->m_pHitbox->GeAverageVelocityForMatchingTag( m_torsoTag );
            EE_ASSERT( torsoBounds.IsValid() );

            m_targetBounds = AABB::Lerp( pTrackedTarget->m_bounds, torsoBounds, pTrackedTarget->m_timeTracked / m_torsoFocusTime );
        }
        else // Full body focus
        {
            m_targetBounds = pTrackedTarget->m_bounds;
            m_targetVelocity = pTrackedTarget->m_pHitbox->GeAverageVelocityForMatchingTag( m_torsoTag );
        }

        // Lerp towards the head target
        //-------------------------------------------------------------------------

        if ( pTrackedTarget->m_headTrackingWeight.GetWeight() > 0 )
        {
            m_targetBounds = AABB::Lerp( m_targetBounds, headBounds, pTrackedTarget->m_headTrackingWeight.GetWeight() );
            m_targetVelocity = Vector::Lerp( m_targetVelocity, headVelocity, pTrackedTarget->m_headTrackingWeight.GetWeight() );
        }
    }

    void PlayerTargetingComponent::ApplyVelocityModifiersToTrackedTarget()
    {
        Target const* pTrackedTarget = GetTrackedTarget();

        if ( m_targetVelocity.IsNearZero3() )
        {
            return;
        }

        Vector targetDir;
        float targetSpeed = 0.0f;
        m_targetVelocity.ToDirectionAndLength3( targetDir, targetSpeed );

        if ( targetSpeed < m_velocityModifierRange.m_begin )
        {
            return;
        }

        Percentage const modifierWeight = m_velocityModifierRange.GetPercentageThroughClamped( targetSpeed );
        float const offset = modifierWeight.ToFloat() * m_maxOffsetDistanceDueToVelocity;
        m_targetBounds.Expand( Vector( offset / 2, offset / 2, 0 ) );
        m_targetBounds.AddTranslationOffset( targetDir * -offset );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PlayerTargetingComponent::DrawDebug( DebugDrawContext& ctx )
    {
        int32_t const numTargets = (int32_t) m_reflectedTargets.size();
        for ( int32_t i = 0; i < numTargets; i++ )
        {
            auto const& target = m_reflectedTargets[i];

            InlineString str;
            str.sprintf( "%d", i );

            if ( target.m_entityID == m_trackedTargetEntityID )
            {
                Color const color = Color::EvaluateRedGreenGradient( 1.0f - target.m_headTrackingWeight.GetWeight(), false );

                // Draw head tracking shape
                Vector const offset = ( m_viewRight * m_headTrackingShapeHalfLength );
                Vector const capsulePoint0 = m_headTrackingSpherePos - offset;
                Vector const capsulePoint1 = m_headTrackingSpherePos + offset;
                ctx.DrawWireCapsule( capsulePoint0, capsulePoint1, m_headTrackingShapeRadius, Colors::LightPink.GetAlphaVersion( 0.5f ) );

                // Draw target box
                ctx.DrawText3D( target.m_bounds.GetBottomCenter() + Vector( 0, 0, 0.25f ), str.c_str(), color );
                ctx.DrawWireBox( m_targetBounds, color );
            }
            else
            {
                ctx.DrawWireBox( target.m_bounds, Colors::Gray.GetAlphaVersion( 0.35f ) );
                ctx.DrawText3D( target.m_bounds.GetBottomCenter() + Vector( 0, 0, 0.25f ), str.c_str(), Colors::Gray.GetAlphaVersion( 0.35f ) );
            }

            if ( target.m_entityID == m_lockedOnTargetEntityID )
            {
                ctx.DrawSphere( target.m_bounds.GetCenter() + Vector( 0, 0, target.m_bounds.GetExtents().GetZ() + 0.2f ), 0.1f, Colors::Red );
            }
        }
    }
    #endif
}
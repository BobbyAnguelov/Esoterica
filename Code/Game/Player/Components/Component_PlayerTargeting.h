#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/Math/Vector.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Math/BlendWeight.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerDebugView;
    class DebugDrawContext;
    class HitboxComponent;
    class Hitbox;
    namespace Math { class ViewVolume; }

    //-------------------------------------------------------------------------

    class EE_GAME_API PlayerTargetingComponent : public EntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( PlayerTargetingComponent );

        friend PlayerDebugView;
        friend class PlayerTargetingSystem;

        struct Target
        {
            void ResetScoreState();

        public:

            EntityID                            m_entityID;
            Hitbox const*                       m_pHitbox = nullptr; // Only set for the duration of the "ReflectTargets" call
            AABB                                m_bounds;
            Seconds                             m_timeTracked = 0;
            Seconds                             m_timeOffScreen = 0;

            Degrees                             m_angularDistance = 0;
            float                               m_distance = 0;
            float                               m_angleScore = 0;
            float                               m_distanceScore = 0;
            float                               m_score = 0;

            Math::BlendWeight                   m_headTrackingWeight;
            Seconds                             m_headTrackingGraceTime = 0;

            bool                                m_isInViewVolume = false;
        };

    public:

        bool HasTrackedTarget() const { return m_trackedTargetEntityID.IsValid(); }
        AABB GetTrackedTargetBounds() const { EE_ASSERT( HasTrackedTarget() ); return m_targetBounds; }

        // Lock on system
        //-------------------------------------------------------------------------
        // Allows you to lock onto a target without having to aim at them

        void LockOn();
        void CancelLockOn();
        bool IsLockedOn() const { return m_lockedOnTargetEntityID.IsValid(); }

    private:

        Target* GetTrackedTarget();
        Target const* GetTrackedTarget() const { return const_cast<PlayerTargetingComponent*>( this )->GetTrackedTarget(); }

        void ReflectTargets( Seconds deltaTime, Seconds unscaledDeltaTime, Math::ViewVolume const& viewVolume, TInlineVector<HitboxComponent const*, 20> const& targetHitboxes );

        // Pick the best overall target
        bool PickBestTargetEntity();

        // Calculates the overall zone for targeting (full body, chest or head)
        void UpdateTargetedZone();

        // Shrink/grows the targeted zone to reflect overall accuracy
        void ApplyVelocityModifiersToTrackedTarget();

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx );
        #endif

    private:

        // Settings
        StringID                                m_headTag = StringID( "Headshot" );
        StringID                                m_torsoTag = StringID( "Torso" );
        float                                   m_distanceBias = 0.15f;
        Seconds                                 m_lockOnOffscreenGracePeriod = 1.0f;
        Seconds                                 m_torsoFocusTime = 1.0f; // The time needed to focus on the torso
        Seconds                                 m_torsoStartFocusTime = 0.2f; // How long before we start focusing on the torso

        float                                   m_headTrackingShapeHalfLength = 0.1f; // The radius of the head tracking detection zone
        float                                   m_headTrackingShapeRadius = 0.3f; // The radius of the head tracking detection zone
        Seconds                                 m_headFocusTime = 0.4f; // The time needed to focus on the head
        Seconds                                 m_headTrackingGracePeriod = 0.5f; // The time that we focus on the head once we lose it

        FloatRange                              m_velocityModifierRange = FloatRange( 2.5f, 7.5f );
        float                                   m_maxOffsetDistanceDueToVelocity = 0.1f;

        //-------------------------------------------------------------------------

        EntityID                                m_trackedTargetEntityID;
        EntityID                                m_lockedOnTargetEntityID;
        TInlineVector<Target, 10>               m_reflectedTargets;

        AABB                                    m_targetBounds;
        Vector                                  m_targetVelocity = Vector::Zero;

        // Transient Data needed for target update
        Vector                                  m_viewPos;
        Vector                                  m_viewForward;
        Vector                                  m_viewRight;
        Seconds                                 m_deltaTime = 0;
        Degrees                                 m_lowestAngularDistance = FLT_MAX;
        float                                   m_maxDistance = 0;

        #if EE_DEVELOPMENT_TOOLS
        Vector                                  m_headTrackingSpherePos;
        #endif
    };
}
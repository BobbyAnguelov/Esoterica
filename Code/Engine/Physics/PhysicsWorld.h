#pragma once

#include "Engine/Physics/PhysicsQuery.h"
#include "Base/Time/Time.h"
#include "Base/Math/Transform.h"
#include <atomic>

//-------------------------------------------------------------------------

namespace EE { struct AABB; }

namespace physx 
{
    class PxScene;
    class PxGeometry;
    class PxRigidActor;
    class PxShape;
    class PxRenderBuffer;
    class PxControllerManager;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct ActorUserData;
    class CharacterComponent;
    class PhysicsShapeComponent;
    class MaterialRegistry;
    class Ragdoll;
    struct RagdollDefinition;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsWorld final
    {
        friend class PhysicsWorldSystem;

    public:

        // The distance that the shape is pushed away from a detected collision after a sweep - currently set to 5mm as that is a relatively standard value
        static constexpr float const s_sweepSeperationDistance = 0.005f;

    public:

        PhysicsWorld( MaterialRegistry const* pRegistry, bool isGameWorld );
        ~PhysicsWorld();

        // Locks
        //-------------------------------------------------------------------------

        void AcquireReadLock();
        void ReleaseReadLock();
        void AcquireWriteLock();
        void ReleaseWriteLock();

        // Ragdolls
        //-------------------------------------------------------------------------

        Ragdoll* CreateRagdoll( RagdollDefinition const* pDefinition, StringID const& profileID, uint64_t userID );
        void DestroyRagdoll( Ragdoll*& pRagdoll );

        // Queries
        //-------------------------------------------------------------------------

        inline bool RayCast( Vector const& start, Vector const& end, QueryRules const& rules, RayCastResults& outResults ) 
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance);
            return RayCastInternal( start, direction, distance, rules, outResults );
        }

        inline bool RayCast( Vector const& start, Vector const& unitDirection, float distance, QueryRules const& rules, RayCastResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return RayCastInternal( start, unitDirection, distance, rules, outResults );
        }

        bool SphereSweep( float radius, Vector const& start, Vector const& end, QueryRules const& rules, SweepResults& outResults )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return SphereSweepInternal( radius, start, direction, distance, rules, outResults );
        }

        bool SphereSweep( float radius, Vector const& start, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return SphereSweepInternal( radius, start, unitDirection, distance, rules, outResults );
        }

        bool SphereSweep( float radius, Transform const& startTransform, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return SphereSweepInternal( radius, startTransform.GetTranslation(), unitDirection, distance, rules, outResults );
        }

        bool CapsuleSweep( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, QueryRules const& rules, SweepResults& outResults )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return CapsuleSweepInternal( radius, cylinderPortionHalfHeight, orientation, start, direction, distance, rules, outResults );
        }

        bool CapsuleSweep( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return CapsuleSweepInternal( radius, cylinderPortionHalfHeight, orientation, start, unitDirection, distance, rules, outResults );
        }

        bool CapsuleSweep( float radius, float cylinderPortionHalfHeight, Transform const& startTransform, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return CapsuleSweepInternal( radius, cylinderPortionHalfHeight, startTransform.GetRotation(), startTransform.GetTranslation(), unitDirection, distance, rules, outResults );
        }

        bool CylinderSweep( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, QueryRules const& rules, SweepResults& outResults )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return CylinderSweepInternal( radius, cylinderPortionHalfHeight, orientation, start, direction, distance, rules, outResults );
        }

        bool CylinderSweep( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return CylinderSweepInternal( radius, cylinderPortionHalfHeight, orientation, start, unitDirection, distance, rules, outResults );
        }

        bool CylinderSweep( float radius, float cylinderPortionHalfHeight, Transform const& startTransform, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return CylinderSweepInternal( radius, cylinderPortionHalfHeight, startTransform.GetRotation(), startTransform.GetTranslation(), unitDirection, distance, rules, outResults );
        }

        bool BoxSweep( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& end, QueryRules const& rules, SweepResults& outResults )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return BoxSweepInternal( halfExtents, orientation, start, direction, distance, rules, outResults );
        }

        bool BoxSweep( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return BoxSweepInternal( halfExtents, orientation, start, unitDirection, distance, rules, outResults );
        }

        bool BoxSweep( Vector halfExtents, Transform const& startTransform, Vector const& unitDirection, float distance, QueryRules const& rules, SweepResults& outResults )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            return BoxSweepInternal( halfExtents, startTransform.GetRotation(), startTransform.GetTranslation(), unitDirection, distance, rules, outResults );
        }

        bool SphereOverlap( float radius, Vector const& position, QueryRules const& rules, OverlapResults& outResults );

        bool CapsuleOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, QueryRules const& rules, OverlapResults& outResults );

        inline bool CapsuleOverlap( float radius, float cylinderPortionHalfHeight, Transform const& shapeTransform, QueryRules const& rules, OverlapResults& outResults )
        {
            return CapsuleOverlap( cylinderPortionHalfHeight, radius, shapeTransform.GetRotation(), shapeTransform.GetTranslation(), rules, outResults );
        }

        bool CylinderOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, QueryRules const& rules, OverlapResults& outResults );

        inline bool CylinderOverlap( float radius, float cylinderPortionHalfHeight, Transform const& shapeTransform, QueryRules const& rules, OverlapResults& outResults )
        {
            return CylinderOverlap( cylinderPortionHalfHeight, radius, shapeTransform.GetRotation(), shapeTransform.GetTranslation(), rules, outResults );
        }

        bool BoxOverlap( Vector halfExtents, Quaternion const& orientation, Vector const& position, QueryRules const& rules, OverlapResults& outResults );

        inline bool BoxOverlap( Vector halfExtents, Transform const& shapeTransform, QueryRules const& rules, OverlapResults& outResults )
        {
            return BoxOverlap( halfExtents, shapeTransform.GetRotation(), shapeTransform.GetTranslation(), rules, outResults );
        }

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline uint32_t GetDebugFlags() const { return m_sceneDebugFlags; }
        void SetDebugFlags( uint32_t debugFlags );

        inline bool IsDebugDrawingEnabled() const;
        void SetDebugDrawingEnabled( bool enableDrawing );
        inline float GetDebugDrawDistance() const { return m_debugDrawDistance; }
        inline void SetDebugDrawDistance( float drawDistance ) { m_debugDrawDistance = Math::Max( drawDistance, 0.0f ); }

        void SetDebugCullingBox( AABB const& cullingBox );
        physx::PxRenderBuffer const& GetRenderBuffer() const;
        #endif

    private:

        PhysicsWorld( PhysicsWorld const& ) = delete;
        PhysicsWorld( PhysicsWorld&& ) = delete;
        PhysicsWorld& operator=( PhysicsWorld const& ) = delete;
        PhysicsWorld& operator=( PhysicsWorld&& ) = delete;

        // Simulation
        //-------------------------------------------------------------------------

        void Simulate( Seconds deltaTime );

        // Queries
        //-------------------------------------------------------------------------

        bool RayCastInternal( Vector const& start, Vector const& directionAndDistance, float distance, QueryRules const& rules, RayCastResults& outResults );
        bool SphereSweepInternal( float radius, Vector const& start, Vector const& directionAndDistance, float distance, QueryRules const& rules, SweepResults& outResults );
        bool CapsuleSweepInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& directionAndDistance, float distance, QueryRules const& rules, SweepResults& outResults );
        bool CylinderSweepInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& directionAndDistance, float distance, QueryRules const& rules, SweepResults& outResults );
        bool BoxSweepInternal( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults );
        bool SweepInternal( physx::PxGeometry const& geo, Transform const& startTransform, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults );
        bool OverlapInternal( physx::PxGeometry const& geo, Transform const& transform, QueryRules const& rules, OverlapResults& outResults );

        // Actors and Shapes
        //-------------------------------------------------------------------------

        bool CreateActor( PhysicsShapeComponent* pComponent ) const;
        void DestroyActor( PhysicsShapeComponent* pComponent ) const;

        bool CreateCharacterController( CharacterComponent* pComponent ) const;
        void DestroyCharacterController( CharacterComponent* pComponent ) const;

    private:

        MaterialRegistry const*                                 m_pMaterialRegistry = nullptr;
        physx::PxScene*                                         m_pScene = nullptr;
        physx::PxControllerManager*                             m_pControllerManager = nullptr;
        bool                                                    m_isGameWorld = false;

        #if EE_DEVELOPMENT_TOOLS
        uint32_t                                                m_sceneDebugFlags = 0;
        float                                                   m_debugDrawDistance = 10.0f;
        
        std::atomic<int32_t>                                    m_readLockCount = false;        // Assertion helper
        std::atomic<bool>                                       m_writeLockAcquired = false;    // Assertion helper
        #endif
    };
}
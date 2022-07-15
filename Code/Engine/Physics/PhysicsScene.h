#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Physics/PhysicsQuery.h"
#include "Engine/Physics/PhysX.h"
#include <atomic>

//-------------------------------------------------------------------------

namespace EE
{
    class StringID;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class Ragdoll;
    struct RagdollDefinition;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Scene final
    {
        friend class PhysicsWorldSystem;

    public:

        // The distance that the shape is pushed away from a detected collision after a sweep - currently set to 5mm as that is a relatively standard value
        static constexpr float const s_sweepSeperationDistance = 0.005f;

    public:

        Scene( physx::PxScene* pScene );
        ~Scene();

        // Locks
        //-------------------------------------------------------------------------

        void AcquireReadLock();
        void ReleaseReadLock();
        void AcquireWriteLock();
        void ReleaseWriteLock();

        // Ragdoll Factory
        //-------------------------------------------------------------------------

        Ragdoll* CreateRagdoll( RagdollDefinition const* pDefinition, StringID const& profileID, uint64_t userID );

        // Queries
        //-------------------------------------------------------------------------
        // The  versions of the queries allow you to provide your own result container, generally only useful if you hit the 32 hit limit the default results provides
        // NOTE!!! Make sure to always use the Physics world transform for the shape and not the component transforms directly!!!!
        // WARNING!!! None of these function acquire read locks, the user is expected to manually lock/unlock the scene, since they are often doing more than one query

        template<int N>
        bool RayCast( Vector const& start, Vector const& end, QueryFilter& filter, RayCastResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_start = start;
            outResults.m_end = end;

            Vector unitDirection; float distance;
            ( end - start ).ToDirectionAndLength3( unitDirection, distance );
            EE_ASSERT( !unitDirection.IsNearZero3() );
            return RayCast( start, unitDirection, distance, filter, outResults );
        }

        template<int N>
        bool RayCast( Vector const& start, Vector const& unitDirection, float distance, QueryFilter& filter, RayCastResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            EE_ASSERT( unitDirection.IsNormalized3() && distance > 0 );

            outResults.m_start = start;
            outResults.m_end = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );

            bool const result = m_pScene->raycast( ToPx( start ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            return result;
        }

        //-------------------------------------------------------------------------

        // Spheres
        template<int N>
        bool SphereSweep( float radius, Vector const& start, Vector const& end, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = end;

            Vector unitDirection; float distance;
            ( end - start ).ToDirectionAndLength3( unitDirection, distance );
            EE_ASSERT( !unitDirection.IsNearZero3() );

            physx::PxSphereGeometry const sphereGeo( radius );
            bool const result = m_pScene->sweep( sphereGeo, physx::PxTransform( ToPx( start ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        template<int N>
        bool SphereSweep( float radius, Vector const& start, Vector const& unitDirection, float distance, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            EE_ASSERT( unitDirection.IsNormalized3() && distance > 0 );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );

            physx::PxSphereGeometry const sphereGeo( radius );
            bool const result = m_pScene->sweep( sphereGeo, physx::PxTransform( ToPx( start ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        // Perform an overlap query. NOTE: This overlap may not agree with the results of a sweep query with regards to the initially overlapping state
        // Note: Overlap results will never have the block hit set!
        template<int N>
        bool SphereOverlap( float radius, Vector const& position, QueryFilter& filter, OverlapResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_position = position;

            // Set the no block value for overlaps
            TScopedGuardValue guard( filter.m_filterData.flags, filter.m_filterData.flags | physx::PxQueryFlag::eNO_BLOCK );
            physx::PxSphereGeometry const sphereGeo( radius );
            bool const result = m_pScene->overlap( sphereGeo, physx::PxTransform( ToPx( position ) ), outResults, filter.m_filterData, &filter );
            return result;
        }

        //-------------------------------------------------------------------------

        // Sweep a capsule
        // Note: the capsule half-height is along the X-axis
        template<int N>
        bool CapsuleSweep( float cylinderPortionHalfHeight, float radius, Quaternion const& orientation, Vector const& start, Vector const& end, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = end;
            outResults.m_orientation = orientation;

            Vector unitDirection;
            float distance;
            ( end - start ).ToDirectionAndLength3( unitDirection, distance );
            EE_ASSERT( !unitDirection.IsNearZero3() );

            physx::PxCapsuleGeometry const capsuleGeo( radius, cylinderPortionHalfHeight );
            physx::PxTransform Test( ToPx( start ), ToPx( orientation ) );
            EE_ASSERT( Test.isValid() );
            bool const result = m_pScene->sweep( capsuleGeo, physx::PxTransform( ToPx( start ), ToPx( orientation ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        // Sweep a capsule
        // Note: the capsule half-height is along the X-axis
        template<int N>
        bool CapsuleSweep( float cylinderPortionHalfHeight, float radius, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            EE_ASSERT( unitDirection.IsNormalized3() && distance > 0 );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            outResults.m_orientation = orientation;

            physx::PxCapsuleGeometry const capsuleGeo( radius, cylinderPortionHalfHeight );
            physx::PxTransform Test( ToPx( start ), ToPx( orientation ) );
            EE_ASSERT( Test.isValid() );
            bool const result = m_pScene->sweep( capsuleGeo, physx::PxTransform( ToPx( start ), ToPx( orientation ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        // Perform an overlap query. NOTE: This overlap may not agree with the results of a sweep query with regards to the initially overlapping state
        // Note: Overlap results will never have the block hit set!
        // Note: the capsule half-height is along the X-axis
        template<int N>
        bool CapsuleOverlap( float cylinderPortionHalfHeight, float radius, Quaternion const& orientation, Vector const& position, QueryFilter& filter, OverlapResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_position = position;
            outResults.m_orientation = orientation;

            // Set the no block value for overlaps
            TScopedGuardValue guard( filter.m_filterData.flags, filter.m_filterData.flags | physx::PxQueryFlag::eNO_BLOCK );
            physx::PxCapsuleGeometry const capsuleGeo( radius, cylinderPortionHalfHeight );
            bool result = m_pScene->overlap( capsuleGeo, physx::PxTransform( ToPx( position ), ToPx( orientation ) ), outResults, filter.m_filterData, &filter );
            return result;
        }

        //-------------------------------------------------------------------------

        template<int N>
        bool CylinderSweep( float halfHeight, float radius, Quaternion const& orientation, Vector const& start, Vector const& end, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = end;
            outResults.m_orientation = orientation;

            Vector unitDirection; float distance;
            ( end - start ).ToDirectionAndLength3( unitDirection, distance );
            EE_ASSERT( !unitDirection.IsNearZero3() );

            physx::PxConvexMeshGeometry const cylinderGeo( SharedMeshes::s_pUnitCylinderMesh, physx::PxMeshScale( physx::PxVec3( 2.0f * halfHeight, 2.0f * radius, 2.0f * radius ) ) );
            bool const result = m_pScene->sweep( cylinderGeo, physx::PxTransform( ToPx( start ), ToPx( orientation ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        template<int N>
        bool CylinderSweep( float halfHeight, float radius, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            EE_ASSERT( unitDirection.IsNormalized3() && distance > 0 );
            EE_ASSERT( SharedMeshes::s_pUnitCylinderMesh != nullptr );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            outResults.m_orientation = orientation;

            physx::PxConvexMeshGeometry const cylinderGeo( SharedMeshes::s_pUnitCylinderMesh, physx::PxMeshScale( physx::PxVec3( 2.0f * halfHeight, 2.0f * radius, 2.0f * radius ) ) );
            bool const result = m_pScene->sweep( cylinderGeo, physx::PxTransform( ToPx( start ), ToPx( orientation ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        // Perform an overlap query. NOTE: This overlap may not agree with the results of a sweep query with regards to the initially overlapping state
        // Note: Overlap results will never have the block hit set!
        template<int N>
        bool CylinderOverlap( float halfHeight, float radius, Quaternion const& orientation, Vector const& position, QueryFilter& filter, OverlapResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_position = position;
            outResults.m_orientation = orientation;

            // Set the no block value for overlaps
            TScopedGuardValue guard( filter.m_filterData.flags, filter.m_filterData.flags | physx::PxQueryFlag::eNO_BLOCK );
            physx::PxConvexMeshGeometry const cylinderGeo( SharedMeshes::s_pUnitCylinderMesh, physx::PxMeshScale( physx::PxVec3( 2.0f * halfHeight, 2.0f * radius, 2.0f * radius ) ) );
            bool const result = m_pScene->overlap( cylinderGeo, physx::PxTransform( ToPx( position ), ToPx( orientation ) ), outResults, filter.m_filterData, &filter );
            return result;
        }

        //-------------------------------------------------------------------------

        template<int N>
        bool BoxSweep( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            EE_ASSERT( unitDirection.IsNormalized3() && distance > 0 );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            outResults.m_orientation = orientation;

            physx::PxBoxGeometry const boxGeo( ToPx( halfExtents ) );
            bool const result = m_pScene->sweep( boxGeo, physx::PxTransform( ToPx( start ), ToPx( orientation ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        template<int N>
        bool BoxSweep( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& end, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            Vector unitDirection; float distance;
            ( end - start ).ToDirectionAndLength3( unitDirection, distance );
            EE_ASSERT( !unitDirection.IsNearZero3() );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = end;
            outResults.m_orientation = orientation;

            physx::PxBoxGeometry const boxGeo( ToPx( halfExtents ) );
            bool const result = m_pScene->sweep( boxGeo, physx::PxTransform( ToPx( start ), ToPx( orientation ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        // Perform an overlap query. NOTE: This overlap may not agree with the results of a sweep query with regards to the initially overlapping state
        // Note: Overlap results will never have the block hit set!
        template<int N>
        bool BoxOverlap( Vector halfExtents, Quaternion const& orientation, Vector const& position, QueryFilter& filter, OverlapResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_position = position;
            outResults.m_orientation = orientation;

            // Set the no block value for overlaps
            TScopedGuardValue guard( filter.m_filterData.flags, filter.m_filterData.flags | physx::PxQueryFlag::eNO_BLOCK );
            physx::PxBoxGeometry const boxGeo( ToPx( halfExtents ) );
            bool const result = m_pScene->overlap( boxGeo, physx::PxTransform( ToPx( position ), ToPx( orientation ) ), outResults, filter.m_filterData, &filter );
            return result;
        }

        //-------------------------------------------------------------------------

        template<int N>
        bool ShapeSweep( physx::PxShape* pShape, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            EE_ASSERT( unitDirection.IsNormalized3() && distance > 0 );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            outResults.m_orientation = orientation;

            bool const result = m_pScene->sweep( pShape->getGeometry().any(), ToPx( Transform( orientation, start ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        template<int N>
        bool ShapeSweep( physx::PxShape* pShape, Quaternion const& orientation, Vector const& start, Vector const& end, QueryFilter& filter, SweepResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );
            Vector unitDirection; float distance;
            ( end - start ).ToDirectionAndLength3( unitDirection, distance );
            EE_ASSERT( !unitDirection.IsNearZero3() );

            outResults.m_sweepStart = start;
            outResults.m_sweepEnd = end;
            outResults.m_orientation = orientation;

            bool const result = m_pScene->sweep( pShape->getGeometry().any(), ToPx( Transform( orientation, start ) ), ToPx( unitDirection ), distance, outResults, filter.m_hitFlags, filter.m_filterData, &filter );
            outResults.CalculateFinalShapePosition( s_sweepSeperationDistance );
            return result;
        }

        // Perform an overlap query. NOTE: This overlap may not agree with the results of a sweep query with regards to the initially overlapping state
        // Note: Overlap results will never have the block hit set!
        template<int N>
        bool ShapeOverlap( physx::PxShape* pShape, Quaternion const& orientation, Vector const& start, QueryFilter& filter, OverlapResultBuffer<N>& outResults )
        {
            EE_ASSERT( m_readLockCount > 0 );

            outResults.m_position = start;
            outResults.m_orientation = orientation;

            // Set the no block value for overlaps
            TScopedGuardValue guard( filter.m_filterData.flags, filter.m_filterData.flags | physx::PxQueryFlag::eNO_BLOCK );
            bool const result = m_pScene->overlap( pShape->getGeometry().any(), ToPx( Transform( orientation, start ) ), outResults, filter.m_filterData, &filter );
            return result;
        }

    private:

        Scene( Scene const& ) = delete;
        Scene( Scene&& ) = delete;
        Scene& operator=( Scene const& ) = delete;
        Scene& operator=( Scene&& ) = delete;

    private:

        physx::PxScene*                                         m_pScene = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        std::atomic<int32_t>                                    m_readLockCount = false;        // Assertion helper
        std::atomic<bool>                                       m_writeLockAcquired = false;    // Assertion helper
        #endif
    };
}
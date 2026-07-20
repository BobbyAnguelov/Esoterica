#pragma once

#include "PhysicsMaterial.h"
#include "Engine/Physics/PhysicsQuery.h"
#include "Base/Time/Time.h"
#include "Base/Math/Transform.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Threading/Threading.h"
#include "Base/Profiling.h"
#include <atomic>
#include "Base/Math/SimpleMovingAverage.h"

//-------------------------------------------------------------------------

namespace EE 
{
    struct AABB;
    class Viewport;
    class SystemRegistry;
    namespace Render { class DebugMeshRegistry; }
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API PhysicsWorld final
    {
        friend class PhysicsWorldSystem;

        //-------------------------------------------------------------------------

        constexpr static int s_maxAsyncTasks = 64;

        class AsyncTask : public ITaskSet
        {
        public:

            AsyncTask() = default;

            virtual void ExecuteRange( enki::TaskSetPartition range, uint32_t threadIndex ) override
            {
                EE_PROFILE_FUNCTION_PHYSICS();
                m_pTask( m_pTaskContext );
            }

        public:

            b3TaskCallback*     m_pTask = nullptr;
            void*               m_pTaskContext = nullptr;
        };

        // The distance that the shape is pushed away from a detected collision after a sweep - currently set to 5mm as that is a relatively standard value
        static constexpr float const s_sweepSeperationDistance = 0.005f;

    public:

        PhysicsWorld( SystemRegistry const& systemRegistry, bool isGameWorld );
        ~PhysicsWorld();

        EE_FORCE_INLINE bool IsGameWorld() const { return m_isGameWorld; }
        EE_FORCE_INLINE b3WorldId GetWorldID() const { return m_worldID; }
        EE_FORCE_INLINE b3SurfaceMaterial const& GetMaterial( StringID materialID ) const { return m_materialRegistry.GetMaterial( materialID ); }
        EE_FORCE_INLINE b3SurfaceMaterial const& GetMaterial( MaterialID materialID ) const { return m_materialRegistry.GetMaterial( materialID.m_ID ); }

        inline Seconds GetTimeStepInterval() const { return m_fixedTimeStep; }

        // Locks
        //-------------------------------------------------------------------------

        void LockRead() const;
        void UnlockRead() const;
        void LockWrite();
        void UnlockWrite();

        // Queries
        //-------------------------------------------------------------------------

        inline bool RayCast( Vector const& start, Vector const& end, CastQuery& outQuery )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance);
            return RayCastInternal( start, end, direction, distance, outQuery );
        }

        inline bool RayCast( Vector const& start, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            return RayCastInternal( start, end, unitDirection, distance, outQuery );
        }

        bool SphereCast( float radius, Vector const& start, Vector const& end, CastQuery& outQuery )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return SphereCastInternal( radius, start, end, direction, distance, outQuery );
        }

        bool SphereCast( float radius, Vector const& start, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            return SphereCastInternal( radius, start, end, unitDirection, distance, outQuery );
        }

        bool SphereCast( float radius, Transform const& startTransform, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), startTransform.GetTranslation() );
            return SphereCastInternal( radius, startTransform.GetTranslation(), end, unitDirection, distance, outQuery );
        }

        bool CapsuleCast( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, CastQuery& outQuery )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return CapsuleCastInternal( radius, cylinderPortionHalfHeight, orientation, start, end, direction, distance, outQuery );
        }

        bool CapsuleCast( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            return CapsuleCastInternal( radius, cylinderPortionHalfHeight, orientation, start, end, unitDirection, distance, outQuery );
        }

        bool CapsuleCast( float radius, float cylinderPortionHalfHeight, Transform const& startTransform, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), startTransform.GetTranslation() );
            return CapsuleCastInternal( radius, cylinderPortionHalfHeight, startTransform.GetRotation(), startTransform.GetTranslation(), end, unitDirection, distance, outQuery );
        }

        // Note: this is an approximation using a hull with 32 points describing a rough cylinder shape
        bool CylinderCast( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, CastQuery& outQuery )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return CylinderCastInternal( radius, cylinderPortionHalfHeight, orientation, start, end, direction, distance, outQuery );
        }

        // Note: this is an approximation using a hull with 32 points describing a rough cylinder shape
        bool CylinderCast( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            return CylinderCastInternal( radius, cylinderPortionHalfHeight, orientation, start, end, unitDirection, distance, outQuery );
        }

        // Note: this is an approximation using a hull with 32 points describing a rough cylinder shape
        bool CylinderCast( float radius, float cylinderPortionHalfHeight, Transform const& startTransform, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), startTransform.GetTranslation() );
            return CylinderCastInternal( radius, cylinderPortionHalfHeight, startTransform.GetRotation(), startTransform.GetTranslation(), end, unitDirection, distance, outQuery );
        }

        bool BoxCast( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& end, CastQuery& outQuery )
        {
            Vector const dirAndDistance = end - start;
            Vector direction;
            float distance;
            dirAndDistance.ToDirectionAndLength3( direction, distance );
            return BoxCastInternal( halfExtents, orientation, start, end, direction, distance, outQuery );
        }

        bool BoxCast( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), start );
            return BoxCastInternal( halfExtents, orientation, start, end, unitDirection, distance, outQuery );
        }

        bool BoxCast( Vector halfExtents, Transform const& startTransform, Vector const& unitDirection, float distance, CastQuery& outQuery )
        {
            EE_ASSERT( unitDirection.IsNormalized3() );
            EE_ASSERT( distance > 0 );
            Vector const end = Vector::MultiplyAdd( unitDirection, Vector( distance ), startTransform.GetTranslation() );
            return BoxCastInternal( halfExtents, startTransform.GetRotation(), startTransform.GetTranslation(), end, unitDirection, distance, outQuery );
        }

        bool SphereOverlap( float radius, Vector const& position, OverlapQuery& outQuery );

        bool CapsuleOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, OverlapQuery& outQuery );

        inline bool CapsuleOverlap( float radius, float cylinderPortionHalfHeight, Transform const& shapeTransform, OverlapQuery& outQuery )
        {
            return CapsuleOverlap( cylinderPortionHalfHeight, radius, shapeTransform.GetRotation(), shapeTransform.GetTranslation(), outQuery );
        }

        // Note: this is an approximation using a hull with 32 points describing a rough cylinder shape
        bool CylinderOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, OverlapQuery& outQuery );

        // Note: this is an approximation using a hull with 32 points describing a rough cylinder shape
        inline bool CylinderOverlap( float radius, float cylinderPortionHalfHeight, Transform const& shapeTransform, OverlapQuery& outQuery )
        {
            return CylinderOverlap( cylinderPortionHalfHeight, radius, shapeTransform.GetRotation(), shapeTransform.GetTranslation(), outQuery );
        }

        bool BoxOverlap( Vector halfExtents, Quaternion const& orientation, Vector const& position, OverlapQuery& outQuery );

        inline bool BoxOverlap( Vector halfExtents, Transform const& shapeTransform, OverlapQuery& outQuery )
        {
            return BoxOverlap( halfExtents, shapeTransform.GetRotation(), shapeTransform.GetTranslation(), outQuery );
        }

    private:

        PhysicsWorld( PhysicsWorld const& ) = delete;
        PhysicsWorld( PhysicsWorld&& ) = delete;
        PhysicsWorld& operator=( PhysicsWorld const& ) = delete;
        PhysicsWorld& operator=( PhysicsWorld&& ) = delete;

        // Simulation
        //-------------------------------------------------------------------------

        void Simulate( Seconds deltaTime );

        AsyncTask* EnqueueAsyncTask( b3TaskCallback* pTask, void* pTaskContext );
        void WaitForAsyncTask( AsyncTask *pAsyncTask );

        // Queries
        //-------------------------------------------------------------------------

        bool RayCastInternal( Vector const& start, Vector const& end, Vector const& directionAndDistance, float distance, CastQuery& outQuery );
        bool SphereCastInternal( float radius, Vector const& start, Vector const& end, Vector const& directionAndDistance, float distance, CastQuery& outQuery );
        bool CapsuleCastInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, Vector const& directionAndDistance, float distance, CastQuery& outQuery );
        bool CylinderCastInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, Vector const& directionAndDistance, float distance, CastQuery& outQuery );
        bool BoxCastInternal( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery );
        bool CastInternal( b3ShapeProxy& shapeProxy, Transform const& startTransform, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery );
        bool OverlapInternal( b3ShapeProxy& shapeProxy, Transform const& transform, OverlapQuery& outQuery );

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( Viewport* pViewport ) const;
        bool IsRecording() const { return m_pRecording != nullptr; }
        void StartRecording();
        void StopRecording();
        #endif

    private:

        TaskSystem&                                             m_taskSystem;
        MaterialRegistry const&                                 m_materialRegistry;
        b3WorldId                                               m_worldID = {};
        bool                                                    m_isGameWorld = false;

        Seconds                                                 m_accumulatedTime = 0.0f;
        Seconds                                                 m_fixedTimeStep = 1.0f / 120.0f;
        SimpleMovingAverage<120>                                m_averageFrameTime = 1.0f / 120.0f;
        Seconds                                                 m_timeSinceLastTimeStepUpdate = 0.0f;

        AsyncTask                                               m_tasks[s_maxAsyncTasks];
        int32_t                                                 m_usedTasks = 0;

        mutable Threading::ReadWriteMutex                       m_mutex; // Box3D doesnt have a locking mechanism

        #if EE_DEVELOPMENT_TOOLS
        Render::DebugMeshRegistry const&                        m_debugMeshRegistry;
        mutable std::atomic<int32_t>                            m_readLockCount = false;        // Assertion helper
        std::atomic<bool>                                       m_writeLockAcquired = false;    // Assertion helper
        b3Recording*                                            m_pRecording = nullptr;
        #endif
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedWriteLock
    {
    public:

        ScopedWriteLock( PhysicsWorld* pWorld )
            : m_pWorld( pWorld )
        {
            EE_ASSERT( m_pWorld != nullptr );
            m_pWorld->LockWrite();
        }

        ~ScopedWriteLock()
        {
            m_pWorld->UnlockWrite();
        }

    private:

        PhysicsWorld*  m_pWorld = nullptr;
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedReadLock
    {
    public:

        ScopedReadLock( PhysicsWorld const* pWorld )
            : m_pWorld( pWorld )
        {
            EE_ASSERT( m_pWorld != nullptr );
            m_pWorld->LockRead();
        }

        ~ScopedReadLock()
        {
            m_pWorld->UnlockRead();
        }

    private:

        PhysicsWorld const*  m_pWorld = nullptr;
    };
}
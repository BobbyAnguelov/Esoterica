#include "PhysicsWorld.h"
#include "Settings/ViewportSettings_Physics.h"
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Engine/Viewport/Viewport.h"
#include "EASTL/sort.h"
#include "Base/Time/TimeStamp.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    PhysicsWorld::PhysicsWorld( SystemRegistry const& systemRegistry, bool isGameWorld )
        : m_taskSystem( *systemRegistry.GetSystem<TaskSystem>() )
        , m_materialRegistry( *systemRegistry.GetSystem<MaterialRegistry>() )
        , m_isGameWorld( isGameWorld )
        #if EE_DEVELOPMENT_TOOLS
        , m_debugMeshRegistry( *systemRegistry.GetSystem<Render::DebugMeshRegistry>() )
        #endif
    {
        auto EnqueueTask = [] ( b3TaskCallback* pTask, void* pTaskContext, void* pUserContext, const char* taskName ) -> void*
        {
            PhysicsWorld* pPhysicsWorld = static_cast<PhysicsWorld*>( pUserContext );
            return pPhysicsWorld->EnqueueAsyncTask( pTask, pTaskContext );
        };

        auto FinishTask = [] ( void* pTask, void* pUserContext )
        {
            PhysicsWorld* pPhysicsWorld = static_cast<PhysicsWorld*>( pUserContext );
            PhysicsWorld::AsyncTask* pAsyncTask = static_cast<PhysicsWorld::AsyncTask*>( pTask );
            pPhysicsWorld->WaitForAsyncTask( pAsyncTask );
        };

        #if EE_DEVELOPMENT_TOOLS
        auto RegisterDebugMesh = [] ( b3DebugShape const* pDebugShape, void* pUserContext ) -> void*
        {
            auto pDebugMeshRegistry = static_cast<Render::DebugMeshRegistry*>( pUserContext );
            PointerID const instanceID = Core::RegisterDebugShapeInstance( pDebugMeshRegistry, pDebugShape );
            EE_ASSERT( instanceID.IsValid() );
            return instanceID.ToVoidPtr();
        };

        auto UnregisterDebugMesh = [] ( void* pUserShape, void* pUserContext )
        {
            auto pDebugMeshRegistry = static_cast<Render::DebugMeshRegistry*>( pUserContext );
            PointerID const instanceID( pUserShape );
            EE_ASSERT( instanceID.IsValid() );
            Core::UnregisterDebugShapeInstance( pDebugMeshRegistry, instanceID );
        };
        #endif

        //-------------------------------------------------------------------------

        b3WorldDef worldDef = b3DefaultWorldDef();

        worldDef.gravity = ToBox3D( Constants::Gravity );
        worldDef.workerCount = m_taskSystem.GetNumWorkers() + 1; // All workers + main thread
        worldDef.enqueueTask = EnqueueTask;
        worldDef.finishTask = FinishTask;
        worldDef.userTaskContext = this;
        worldDef.enableSleep = true;
        worldDef.enableContinuous = true;

        #if EE_DEVELOPMENT_TOOLS
        worldDef.createDebugShape = RegisterDebugMesh;
        worldDef.destroyDebugShape = UnregisterDebugMesh;
        worldDef.userDebugShapeContext = (void*) &m_debugMeshRegistry;
        #endif

        m_worldID = b3CreateWorld( &worldDef );
        EE_ASSERT( B3_IS_NON_NULL( m_worldID ) );

        //-------------------------------------------------------------------------

        auto CustomFilter = [] ( b3ShapeId shapeIdA, b3ShapeId shapeIdB, void* pContext ) -> bool
        {
            return false;
        };

        b3World_SetCustomFilterCallback( m_worldID, CustomFilter, this );
    }

    PhysicsWorld::~PhysicsWorld()
    {
        #if EE_DEVELOPMENT_TOOLS
        if ( IsRecording() )
        {
            StopRecording();
        }
        #endif

        for ( auto& task : m_tasks )
        {
            m_taskSystem.WaitForTask( &task );
        }

        b3DestroyWorld( m_worldID );
    }

    //-------------------------------------------------------------------------
    // Update
    //-------------------------------------------------------------------------

    void PhysicsWorld::Simulate( Seconds deltaTime )
    {
        EE_PROFILE_FUNCTION_PHYSICS();

        LockWrite();

        // Adjust time-step
        //-------------------------------------------------------------------------

        m_averageFrameTime.AddValue( deltaTime );
        m_timeSinceLastTimeStepUpdate += deltaTime;

        static Seconds const timeStepUpdateInterval = 2.0f;
        if ( m_timeSinceLastTimeStepUpdate > timeStepUpdateInterval || m_fixedTimeStep > deltaTime )
        {
            m_fixedTimeStep = m_averageFrameTime.GetAverage() - Math::LargeEpsilon;
            m_timeSinceLastTimeStepUpdate = 0.0f;
        }

        // Step the physics simulation
        //-------------------------------------------------------------------------

        m_accumulatedTime += deltaTime;

        {
            EE_PROFILE_SCOPE_PHYSICS( "Sim" );

            while ( m_accumulatedTime >= m_fixedTimeStep )
            {
                b3World_Step( m_worldID, m_fixedTimeStep, 4 );
                m_accumulatedTime -= m_fixedTimeStep;
                m_usedTasks = 0;
            }
        }

        // Handle any sensor events
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_PHYSICS( "Events" );

            b3SensorEvents events = b3World_GetSensorEvents( m_worldID );

            for ( int32_t i = 0; i < events.beginCount; ++i )
            {
                EE_UNIMPLEMENTED_FUNCTION();
                b3SensorBeginTouchEvent* pEvent = events.beginEvents + i;
                /*if ( B3_ID_EQUALS( pEvent->sensorShapeId, m_sensorShapeId ) )
                {
                    b3BodyId bodyId = b3Shape_GetBody( event->visitorShapeId );
                    b3DestroyBody( bodyId );
                    break;
                }*/
            }
        }

        UnlockWrite();
    }

    PhysicsWorld::AsyncTask* PhysicsWorld::EnqueueAsyncTask( b3TaskCallback* pTask, void* pTaskContext )
    {
        if ( m_usedTasks < PhysicsWorld::s_maxAsyncTasks )
        {
            PhysicsWorld::AsyncTask& asyncTask = m_tasks[m_usedTasks];
            asyncTask.m_pTask = pTask;
            asyncTask.m_pTaskContext = pTaskContext;
            m_taskSystem.ScheduleTask( &asyncTask );

            m_usedTasks++;
            return &asyncTask;
        }

        EE_LOG_WARNING( LogCategory::Physics, "Physics World", "Exceeded pre-allocated async tasks" );
        pTask( pTaskContext );
        return nullptr;
    }

    void PhysicsWorld::WaitForAsyncTask( AsyncTask *pAsyncTask )
    {
        m_taskSystem.WaitForTask( pAsyncTask );
    }

    //-------------------------------------------------------------------------
    // Locks
    //-------------------------------------------------------------------------

    void PhysicsWorld::LockRead() const
    {
        m_mutex.LockRead();
        EE_DEVELOPMENT_TOOLS_ONLY( ++m_readLockCount );
    }

    void PhysicsWorld::UnlockRead() const
    {
        m_mutex.UnlockRead();
        EE_DEVELOPMENT_TOOLS_ONLY( --m_readLockCount );
    }

    void PhysicsWorld::LockWrite()
    {
        m_mutex.LockWrite();
        EE_DEVELOPMENT_TOOLS_ONLY( m_writeLockAcquired = true );
    }

    void PhysicsWorld::UnlockWrite()
    {
        m_mutex.UnlockWrite();
        EE_DEVELOPMENT_TOOLS_ONLY( m_writeLockAcquired = false );
    }

    //-------------------------------------------------------------------------
    // Casts
    //-------------------------------------------------------------------------

    static float CastCallback( b3ShapeId shapeID, b3Vec3 point, b3Vec3 normal, float fraction, uint64_t userMaterialId, int32_t triangleIndex, int childIndex, void* pContext )
    {
        CastQuery* pQuery = (CastQuery*) pContext;

        bool const allowMultipleHits = pQuery->IsMultipleHitQuery();

        // Handle ignore rules
        UserData* pUserData = (UserData*) ( b3Shape_GetUserData( shapeID ) );
        if ( pUserData != nullptr )
        {
            if ( pQuery->IsEntityIgnored( pUserData->m_entityID ) )
            {
                return -1.0f;
            }

            if ( pQuery->IsComponentIgnored( pUserData->m_componentID ) )
            {
                return -1.0f;
            }
        }

        //-------------------------------------------------------------------------

        // Update existing hit or create a new hit
        CastQuery::Hit* pHit = nullptr;
        if ( allowMultipleHits )
        {
            pHit = &pQuery->m_hits.emplace_back();
        }
        else
        {
            pHit = pQuery->m_hits.empty() ? &pQuery->m_hits.emplace_back() : &pQuery->m_hits.back();
        }

        // Fill out hit data
        pHit->m_bodyID = b3Shape_GetBody( shapeID );
        pHit->m_shapeID = shapeID;
        pHit->m_distance = fraction * pQuery->m_desiredDistance;
        pHit->m_position = Vector::MultiplyAdd( pQuery->m_direction, Vector( pHit->m_distance ), pQuery->m_startPosition );
        pHit->m_contactPoint = FromBox3D( point );
        pHit->m_contactNormal = FromBox3D( normal );
        pHit->m_materialID = userMaterialId;
        pHit->m_isInitiallyOverlapping = ( fraction == 0.0f );

        // Return max fraction to continue the cast, or the current fraction to clip to only the closest hits
        return allowMultipleHits ? 1.0f : fraction;
    }

    bool PhysicsWorld::RayCastInternal( Vector const& start, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery )
    {
        outQuery.m_startPosition = start;
        outQuery.m_endPosition = end;
        outQuery.m_direction = direction;
        outQuery.m_desiredDistance = distance;
        outQuery.m_actualDistance = 0.0f;
        outQuery.m_remainingDistance = 0.0f;
        outQuery.m_hasInitialOverlap = false;
        outQuery.m_hits.clear();

        b3World_CastRay( m_worldID, ToBox3D( outQuery.m_startPosition ), ToBox3D( outQuery.m_endPosition - outQuery.m_startPosition ), ToBox3D( outQuery ), CastCallback, &outQuery );
        eastl::sort( outQuery.m_hits.begin(), outQuery.m_hits.end() );
        return outQuery.HasHits();
    }

    bool PhysicsWorld::CastInternal( b3ShapeProxy& shapeProxy, Transform const& startTransform, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery )
    {
        outQuery.m_startPosition = startTransform.GetTranslation();
        outQuery.m_endPosition = end;
        outQuery.m_direction = direction;
        outQuery.m_desiredDistance = distance;
        outQuery.m_actualDistance = 0.0f;
        outQuery.m_remainingDistance = 0.0f;
        outQuery.m_hasInitialOverlap = false;
        outQuery.m_hits.clear();

        b3World_CastShape( m_worldID, ToBox3D( outQuery.m_startPosition ), &shapeProxy, ToBox3D( outQuery.m_endPosition - outQuery.m_startPosition ), ToBox3D( outQuery ), CastCallback, &outQuery );
        eastl::sort( outQuery.m_hits.begin(), outQuery.m_hits.end() );
        return outQuery.HasHits();
    }

    bool PhysicsWorld::SphereCastInternal( float radius, Vector const& start, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery )
    {
        b3Vec3 origin = ToBox3D( start );

        b3ShapeProxy proxy = {};
        proxy.count = 1;
        proxy.radius = radius;
        proxy.points = &origin;

        return CastInternal( proxy, Transform( Quaternion::Identity, start ), end, direction, distance, outQuery );
    }

    bool PhysicsWorld::CapsuleCastInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery )
    {
        Vector capsuleEnds[2] =
        {
            Vector( 0, 0, cylinderPortionHalfHeight ),
            Vector( 0, 0, -cylinderPortionHalfHeight )
        };

        capsuleEnds[0] = orientation.RotateVector( capsuleEnds[0] );
        capsuleEnds[1] = orientation.RotateVector( capsuleEnds[1] );

        //-------------------------------------------------------------------------

        b3Vec3 const capsulePoints[2] =
        {
            ToBox3D( start + capsuleEnds[0] ),
            ToBox3D( start + capsuleEnds[1] )
        };

        b3ShapeProxy proxy = {};
        proxy.count = 2;
        proxy.radius = radius;
        proxy.points = capsulePoints;

        return CastInternal( proxy, Transform( Quaternion::Identity, start ), end, direction, distance, outQuery );
    }

    bool PhysicsWorld::CylinderCastInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& start, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery )
    {
        TInlineVector<b3Vec3, 32> hullPoints;
        GetTransformedCylinderHullPoints( start, orientation, radius, cylinderPortionHalfHeight, hullPoints );

        b3ShapeProxy proxy = {};
        proxy.points = hullPoints.data();
        proxy.count = (int32_t) hullPoints.size();

        return CastInternal( proxy, Transform( Quaternion::Identity, start ), end, direction, distance, outQuery );
    }

    bool PhysicsWorld::BoxCastInternal( Vector halfExtents, Quaternion const& orientation, Vector const& start, Vector const& end, Vector const& direction, float distance, CastQuery& outQuery )
    {
        b3BoxHull const box = b3MakeTransformedBoxHull( halfExtents.GetX(), halfExtents.GetY(), halfExtents.GetZ(), { ToBox3D( start ), ToBox3D( orientation ) } );

        b3ShapeProxy proxy = {};
        proxy.points = box.boxPoints;
        proxy.count = box.base.vertexCount;

        return CastInternal( proxy, Transform( Quaternion::Identity, start ), end, direction, distance, outQuery );
    }

    //-------------------------------------------------------------------------
    // Overlaps
    //-------------------------------------------------------------------------

    static bool OverlapCallback( b3ShapeId shapeID, void* pContext )
    {
        OverlapQuery* pQuery = static_cast<OverlapQuery*>( pContext );

        // Handle ignore rules
        bool isOverlapIgnored = false;
        UserData* pUserData = (UserData*) ( b3Shape_GetUserData( shapeID ) );
        if ( pUserData != nullptr )
        {
            if ( pQuery->IsEntityIgnored( pUserData->m_entityID ) || pQuery->IsComponentIgnored( pUserData->m_componentID ) )
            {
                isOverlapIgnored = true;
            }
        }

        //-------------------------------------------------------------------------

        if ( !isOverlapIgnored )
        {
            auto& overlap = pQuery->m_overlaps.emplace_back();
            overlap.m_bodyID = b3Shape_GetBody( shapeID );
            overlap.m_shapeID = shapeID;

            // @erin
            // How do I get depenetration info?
            //    if ( pQuery->m_calculateDepenetration )
            //    {
            //        PxCastHit hitInfo;
            //        PxHitFlags const hitFlags = PxHitFlag::eNORMAL | PxHitFlag::eMTD;
            //        PxU32 hitCount = PxGeometryQuery::sweep( PxVec3( 0, 0, 1 ), 0.0f, geo, pxTransform, hit.shape->getGeometry(), hit.actor->getGlobalPose(), hitInfo, hitFlags, 0.0f );
            //        EE_ASSERT( hitCount == 1 );
            //        overlap.m_normal = FromPx( hitInfo.normal );
            //        overlap.m_distance = -hitInfo.distance;
            //    }
            //    else
            //    {
            //        overlap.m_normal = Vector::Zero;
            //        overlap.m_distance = 0.0f;
            //    }
        }

        return pQuery->IsMultipleHitQuery();
    }

    bool PhysicsWorld::OverlapInternal( b3ShapeProxy& shapeProxy, Transform const& transform, OverlapQuery& outQuery )
    {
        LockRead();

        outQuery.m_transform = transform;
        outQuery.m_overlaps.clear();
        b3World_OverlapShape( m_worldID, ToBox3D( transform.GetTranslation() ), &shapeProxy, ToBox3D( outQuery ), OverlapCallback, &outQuery );

        UnlockRead();

        return outQuery.HasOverlaps();
    }

    bool PhysicsWorld::SphereOverlap( float radius, Vector const& position, OverlapQuery& outQuery )
    {
        b3Vec3 origin = ToBox3D( position );

        b3ShapeProxy proxy = {};
        proxy.count = 1;
        proxy.radius = radius;
        proxy.points = &origin;

        return OverlapInternal( proxy, Transform( Quaternion::Identity, position ), outQuery );
    }

    bool PhysicsWorld::CapsuleOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, OverlapQuery& outQuery )
    {
        Vector capsuleEnds[2] =
        {
            Vector( 0, 0, cylinderPortionHalfHeight ),
            Vector( 0, 0, -cylinderPortionHalfHeight )
        };

        capsuleEnds[0] = orientation.RotateVector( capsuleEnds[0] );
        capsuleEnds[1] = orientation.RotateVector( capsuleEnds[1] );

        //-------------------------------------------------------------------------

        b3Vec3 const capsulePoints[2] =
        {
            ToBox3D( position + capsuleEnds[0] ),
            ToBox3D( position + capsuleEnds[1] )
        };

        b3ShapeProxy proxy = {};
        proxy.count = 2;
        proxy.radius = radius;
        proxy.points = capsulePoints;

        return OverlapInternal( proxy, Transform( orientation, position ), outQuery );
    }

    bool PhysicsWorld::CylinderOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, OverlapQuery& outQuery )
    {
        TInlineVector<b3Vec3, 32> hullPoints;
        GetTransformedCylinderHullPoints( position, orientation, radius, cylinderPortionHalfHeight, hullPoints );

        b3ShapeProxy proxy = {};
        proxy.points = hullPoints.data();
        proxy.count = (int32_t) hullPoints.size();

        return OverlapInternal( proxy, Transform( orientation, position ), outQuery );
    }

    bool PhysicsWorld::BoxOverlap( Vector halfExtents, Quaternion const& orientation, Vector const& position, OverlapQuery& outQuery )
    {
        b3BoxHull const box = b3MakeTransformedBoxHull( halfExtents.GetX(), halfExtents.GetY(), halfExtents.GetZ(), { ToBox3D( position ), ToBox3D( orientation ) } );

        b3ShapeProxy proxy = {};
        proxy.points = box.boxPoints;
        proxy.count = box.base.vertexCount;

        return OverlapInternal( proxy, Transform( orientation, position ), outQuery );
    }

    //-------------------------------------------------------------------------
    // Debug
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PhysicsWorld::DrawDebug( Viewport* pViewport ) const
    {
        auto pSettings = pViewport->GetViewportSettings<PhysicsViewportSettings>();
        if ( !pSettings->m_drawDebug )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto DrawShapeCallback = [] ( void* pUserShape, b3Transform transform, b3HexColor color, void* pContext ) -> bool
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            PointerID const instanceID( pUserShape );
            Core::DrawDebugShapeInstance( pDrawContext, instanceID, FromBox3D( transform ), FromBox3D( color, 0.9f ) );
            return true;
        };

        auto DrawPointCallback = [] ( b3Vec3 point, float size, b3HexColor color, void* pContext )
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            pDrawContext->DrawPoint( FromBox3D( point ), FromBox3D( color ), size );
        };

        auto DrawLineCallback = [] ( b3Vec3 vertex1, b3Vec3 vertex2, b3HexColor color, void* pContext )
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            pDrawContext->DrawLine( FromBox3D( vertex1 ), FromBox3D( vertex2 ), FromBox3D( color ), 1.0F );
        };

        auto DrawBoundsCallback = [] ( b3AABB bounds, b3HexColor color, void* pContext )
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            pDrawContext->DrawLitBox( FromBox3D( bounds ), FromBox3D( color, 0.25F ) );
            pDrawContext->DrawWireBox( FromBox3D( bounds ), FromBox3D( color ), 1.0F );
        };

        auto DrawBoxCallback = [] ( b3Vec3 extents, b3Transform transform, b3HexColor color, void* pContext )
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            pDrawContext->DrawLitBox( FromBox3D( transform ), FromBox3D( extents ), FromBox3D( color, 0.25F ) );
            pDrawContext->DrawWireBox( FromBox3D( transform ), FromBox3D( extents ), FromBox3D( color ), 1.0F );
        };

        auto DrawTransformCallback = [] ( b3Transform transform, void* pContext )
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            pDrawContext->DrawAxis( FromBox3D( transform ), 0.25f, 1.0F );
        };

        auto DrawSceneTextCallback = [] ( b3Vec3 position, const char* text, b3HexColor color, void* pContext )
        {
            auto pDrawContext = (DebugDrawContext*) pContext;
            pDrawContext->DrawText3D( FromBox3D( position ), text, FromBox3D( color ) );
        };

        //-------------------------------------------------------------------------

        DebugDrawContext drawingCtx = pViewport->GetDebugDrawContext();

        b3DebugDraw debugDraw = b3DefaultDebugDraw();

        debugDraw.DrawShapeFcn = DrawShapeCallback;
        debugDraw.DrawSegmentFcn = DrawLineCallback;
        debugDraw.DrawPointFcn = DrawPointCallback;
        debugDraw.DrawBoundsFcn = DrawBoundsCallback;
        debugDraw.DrawBoxFcn = DrawBoxCallback;
        debugDraw.DrawTransformFcn = DrawTransformCallback;
        debugDraw.DrawStringFcn = DrawSceneTextCallback;

        debugDraw.context = &drawingCtx;
        debugDraw.drawingBounds = ToBox3D( pViewport->GetViewVolume().GetAABB() );
        debugDraw.forceScale = pSettings->m_forceScale;
        debugDraw.jointScale = pSettings->m_jointScale;
        debugDraw.drawShapes = pSettings->m_drawShapes;
        debugDraw.drawJoints = pSettings->m_drawJoints;
        debugDraw.drawJointExtras = pSettings->m_drawJointExtras;
        debugDraw.drawBounds = pSettings->m_drawBounds;
        debugDraw.drawMass = pSettings->m_drawMass;
        debugDraw.drawSleep = pSettings->m_drawSleep;
        debugDraw.drawBodyNames = pSettings->m_drawBodyNames;
        debugDraw.drawContacts = pSettings->m_drawContacts;
        debugDraw.drawAnchorA = pSettings->m_drawAnchorA ? 1 : 0;
        debugDraw.drawGraphColors = pSettings->m_drawGraphColors;
        debugDraw.drawContactFeatures = pSettings->m_drawContactFeatures;
        debugDraw.drawContactNormals = pSettings->m_drawContactNormals;
        debugDraw.drawContactForces = pSettings->m_drawContactForces;
        debugDraw.drawIslands = pSettings->m_drawIslands;

        b3World_Draw( m_worldID, &debugDraw, B3_DEFAULT_MASK_BITS );
    }

    void PhysicsWorld::StartRecording()
    {
        EE_ASSERT( !IsRecording() );
        m_pRecording = b3CreateRecording( 0 );
        b3World_StartRecording( m_worldID, m_pRecording );
    }

    void PhysicsWorld::StopRecording()
    {
        EE_ASSERT( IsRecording() );
        b3World_StopRecording( m_worldID );

        FileSystem::Path recordingPath = FileSystem::GetCurrentProcessPath();
        EE_ASSERT( recordingPath.IsDirectoryPath() );

        InlineString filename( InlineString::CtorSprintf(), "box3d_recording_%s.b3rec", TimeStamp().GetTimeDetailed().c_str() );
        StringUtils::ReplaceAllOccurrencesInPlace( filename, ":", "_" );
        recordingPath.AppendFilename( filename );
        b3SaveRecordingToFile( m_pRecording, recordingPath.c_str() );

        b3DestroyRecording( m_pRecording );
        m_pRecording = nullptr;
    }
    #endif
}
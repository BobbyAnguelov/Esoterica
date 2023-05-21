#include "PhysicsWorld.h"
#include "Physics.h"
#include "PhysicsQuery.h"
#include "PhysicsRagdoll.h"
#include "Components/Component_PhysicsShape.h"
#include "Components/Component_PhysicsSphere.h"
#include "Components/Component_PhysicsBox.h"
#include "Components/Component_PhysicsCapsule.h"
#include "Components/Component_PhysicsCollisionMesh.h"
#include "Components/Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityLog.h"
#include "System/Profiling.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------
// PhysX
//-------------------------------------------------------------------------

namespace EE::Physics::PX
{
    class TaskDispatcher final : public PxCpuDispatcher
    {
        virtual void submitTask( PxBaseTask & task ) override
        {
            // Surprisingly it is faster to run all physics tasks on a single thread since there is a fair amount of gaps when spreading the tasks across multiple cores.
            // TODO: re-evaluate this when we have additional work. Perhaps we can interleave other tasks while physics tasks are waiting
            auto pTask = &task;
            pTask->run();
            pTask->release();
        }

        virtual PxU32 getWorkerCount() const override
        {
            return 1;
        }
    };

    TaskDispatcher g_taskDispatcher;

    //-------------------------------------------------------------------------

    class SimulationFilter final : public PxSimulationFilterCallback
    {
    public:

        static PxFilterFlags Shader( PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, void const* constantBlock, uint32_t constantBlockSize )
        {
            // Triggers
            //-------------------------------------------------------------------------

            if ( PxFilterObjectIsTrigger( attributes0 ) || PxFilterObjectIsTrigger( attributes1 ) )
            {
                pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
                return PxFilterFlag::eDEFAULT;
            }

            // Articulations
            //-------------------------------------------------------------------------

            if ( PxGetFilterObjectType( attributes0 ) == PxFilterObjectType::eARTICULATION && PxGetFilterObjectType( attributes1 ) == PxFilterObjectType::eARTICULATION )
            {
                pairFlags = PxPairFlag::eCONTACT_DEFAULT;
                return PxFilterFlag::eCALLBACK;
            }

            pairFlags = PxPairFlag::eCONTACT_DEFAULT;

            // Filter
            //-------------------------------------------------------------------------
            // Word 0 is the category, Word 1 is the collision mask
            // The below rule, takes the least blocking result for the pair

            if ( ( filterData0.word0 & filterData1.word1 ) && ( filterData1.word0 & filterData0.word1 ) )
            {
                pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
            }

            return PxFilterFlag::eDEFAULT;
        }

        virtual ~SimulationFilter() = default;

    private:

        virtual PxFilterFlags pairFound( PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1, PxPairFlags& pairFlags ) override
        {
            // Articulations
            //-------------------------------------------------------------------------

            //if ( PxGetFilterObjectType( attributes0 ) == PxFilterObjectType::eARTICULATION && PxGetFilterObjectType( attributes1 ) == PxFilterObjectType::eARTICULATION )
            //{
            //    auto pL0 = static_cast<PxArticulationLink const*>( a0 );
            //    auto pL1 = static_cast<PxArticulationLink const*>( a1 );
            //    PxArticulationReducedCoordinate const* pArticulation0 = &pL0->getArticulation();
            //    PxArticulationReducedCoordinate const* pArticulation1 = &pL1->getArticulation();
            //    Ragdoll const* pRagdoll0 = reinterpret_cast<Ragdoll*>( pArticulation0->userData );
            //    Ragdoll const* pRagdoll1 = reinterpret_cast<Ragdoll*>( pArticulation1->userData );

            //    // If these are different articulation on the same user, then ignore collisions
            //    if ( pArticulation0 != pArticulation1 && pRagdoll0->GetUserID() == pRagdoll1->GetUserID() )
            //    {
            //        return PxFilterFlag::eKILL;
            //    }

            //    // If these are the same articulation, then check the self-collision rules
            //    if ( pArticulation0 == pArticulation1 )
            //    {
            //        uint64_t const bodyIdx0 = (uint64_t) pL0->userData;
            //        uint64_t const bodyIdx1 = (uint64_t) pL1->userData;
            //        if ( !pRagdoll0->ShouldBodiesCollides( (int32_t) bodyIdx0, (int32_t) bodyIdx1 ) )
            //        {
            //            return PxFilterFlag::eKILL;
            //        }
            //    }
            //}

            return PxFilterFlag::eDEFAULT;
        }

        virtual void pairLost( PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, bool objectRemoved ) override
        {
            // Do Nothing
        }

        virtual bool statusChange( PxU32& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags ) override
        {
            return false;
        }
    };

    SimulationFilter g_simulationFilter;

    //-------------------------------------------------------------------------

    class QueryFilter final : public PxQueryFilterCallback
    {
    public:

        QueryFilter( QueryRules const& rules )
            : m_rules( rules )
        {}

        virtual PxQueryHitType::Enum preFilter( PxFilterData const& queryFilterData, PxShape const* pShape, PxRigidActor const* pActor, PxHitFlags& queryFlags ) override
        {
            for ( auto const& ignoredComponentID : m_rules.GetIgnoredComponents() )
            {
                auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
                if ( pOwnerComponent->GetID() == ignoredComponentID )
                {
                    return PxQueryHitType::eNONE;
                }
            }

            //-------------------------------------------------------------------------

            for ( auto const& ignoredEntityID : m_rules.GetIgnoredEntities() )
            {
                auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
                if ( pOwnerComponent->GetEntityID() == ignoredEntityID )
                {
                    return PxQueryHitType::eNONE;
                }
            }

            //-------------------------------------------------------------------------

            return m_rules.m_allowMultipleHits ? PxQueryHitType::eTOUCH : PxQueryHitType::eBLOCK;
        }

        virtual PxQueryHitType::Enum postFilter( PxFilterData const& queryFilterData, PxQueryHit const& hit ) override
        {
            EE_UNREACHABLE_CODE(); // Not currently used
            return PxQueryHitType::eBLOCK;
        }

    private:

        QueryRules const& m_rules;
    };
}

//-------------------------------------------------------------------------
// Physics World
//-------------------------------------------------------------------------

namespace EE::Physics
{
    PhysicsWorld::PhysicsWorld( MaterialRegistry const* pRegistry, bool isGameWorld )
        : m_pMaterialRegistry( pRegistry )
        , m_isGameWorld( isGameWorld )
    {
        EE_ASSERT( m_pMaterialRegistry != nullptr );

        PxTolerancesScale tolerancesScale;
        tolerancesScale.length = Constants::s_lengthScale;
        tolerancesScale.speed = Constants::s_speedScale;

        PxSceneDesc sceneDesc( tolerancesScale );
        sceneDesc.gravity = ToPx( Constants::s_gravity );
        sceneDesc.cpuDispatcher = &PX::g_taskDispatcher;
        sceneDesc.filterShader = PX::SimulationFilter::Shader;
        sceneDesc.filterCallback = &PX::g_simulationFilter;
        sceneDesc.flags = PxSceneFlag::eENABLE_CCD | PxSceneFlag::eREQUIRE_RW_LOCK;
        reinterpret_cast<uint64_t&>( sceneDesc.userData ) = isGameWorld ? 1 : 0;
        m_pScene = Core::GetPxPhysics()->createScene( sceneDesc );

        m_pControllerManager = PxCreateControllerManager( *m_pScene );
        m_pControllerManager->setOverlapRecoveryModule( true );
    }

    PhysicsWorld::~PhysicsWorld()
    {
        m_pControllerManager->purgeControllers();
        m_pControllerManager->release();
        m_pControllerManager = nullptr;

        m_pScene->release();
        m_pScene = nullptr;
    }

    //-------------------------------------------------------------------------
    // Update
    //-------------------------------------------------------------------------

    void PhysicsWorld::Simulate( Seconds deltaTime )
    {
        EE_PROFILE_FUNCTION_PHYSICS();

        AcquireWriteLock();
        {
            // TODO: run at fixed time step
            EE_PROFILE_SCOPE_PHYSICS( "Simulate" );
            m_pScene->simulate( deltaTime );
        }

        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_PHYSICS( "Fetch Results" );
            m_pScene->fetchResults( true );
        }
        ReleaseWriteLock();
    }

    //-------------------------------------------------------------------------
    // Locks
    //-------------------------------------------------------------------------

    void PhysicsWorld::AcquireReadLock()
    {
        m_pScene->lockRead();
        EE_DEVELOPMENT_TOOLS_ONLY( ++m_readLockCount );
    }

    void PhysicsWorld::ReleaseReadLock()
    {
        m_pScene->unlockRead();
        EE_DEVELOPMENT_TOOLS_ONLY( --m_readLockCount );
    }

    void PhysicsWorld::AcquireWriteLock()
    {
        m_pScene->lockWrite();
        EE_DEVELOPMENT_TOOLS_ONLY( m_writeLockAcquired = true );
    }

    void PhysicsWorld::ReleaseWriteLock()
    {
        m_pScene->unlockWrite();
        EE_DEVELOPMENT_TOOLS_ONLY( m_writeLockAcquired = false );
    }

    //-------------------------------------------------------------------------
    // Sweeps
    //-------------------------------------------------------------------------

    bool PhysicsWorld::RayCastInternal( Vector const& position, Vector const& direction, float distance, QueryRules const& rules, RayCastResults& outResults )
    {
        PX::QueryFilter filterCallback( rules );
        PxRaycastBufferN<50> buffer;
        buffer.maxNbTouches = rules.m_allowMultipleHits ? 50 : 0;
        bool const result = m_pScene->raycast( ToPx( position ), ToPx( direction ), distance, buffer, rules.m_hitFlags, rules.m_queryFilterData, &filterCallback );

        //-------------------------------------------------------------------------

        auto ConvertHit = [&] ( PxRaycastHit const& pxHit )
        {
            auto& hit = outResults.m_hits.emplace_back();
            hit.m_pActor = pxHit.actor;
            hit.m_pShape = pxHit.shape;
            hit.m_contactPoint = FromPx( pxHit.position );
            hit.m_distance = pxHit.distance;
            hit.m_normal = FromPx( pxHit.normal );
        };

        // Fill Results
        //-------------------------------------------------------------------------

        outResults.m_startPosition = position;
        outResults.m_direction = direction;
        outResults.m_desiredDistance = distance;

        if ( result )
        {
            // Add block
            if ( buffer.hasBlock )
            {
                ConvertHit( buffer.block );
            }

            // Add additional touches
            for ( uint32_t i = 0; i < buffer.nbTouches; i++ )
            {
                ConvertHit( buffer.getTouch( i ) );
            }

            //-------------------------------------------------------------------------

            eastl::sort( outResults.m_hits.begin(), outResults.m_hits.end(), [] ( RayCastResults::Hit const& a, RayCastResults::Hit const& b ) { return a.m_distance < b.m_distance; } );

            //-------------------------------------------------------------------------

            outResults.m_actualDistance = outResults.m_hits[0].m_distance;
            outResults.m_remainingDistance = distance - outResults.m_actualDistance;
        }
        else
        {
            outResults.m_actualDistance = distance;
            outResults.m_remainingDistance = 0.0f;
        }

        //-------------------------------------------------------------------------

        return result;
    }

    bool PhysicsWorld::SweepInternal( PxGeometry const& geo, Transform const& startTransform, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults )
    {
        PX::QueryFilter filterCallback( rules );
        PxSweepBufferN<50> buffer;
        buffer.maxNbTouches = rules.m_allowMultipleHits ? 50 : 0;
        PxTransform const pxStartTransform( ToPx( startTransform ) );
        PxVec3 const pxSweepDir = ToPx( direction );
        bool const result = m_pScene->sweep( geo, pxStartTransform, pxSweepDir, distance, buffer, rules.m_hitFlags, rules.m_queryFilterData, &filterCallback );

        //-------------------------------------------------------------------------

        auto ConvertHit = [&] ( PxSweepHit const& pxHit )
        {
            auto& hit = outResults.m_hits.emplace_back();
            hit.m_pActor = pxHit.actor;
            hit.m_pShape = pxHit.shape;

            if ( pxHit.hadInitialOverlap() )
            {
                hit.m_shapePosition = outResults.m_sweepStartPosition;
                hit.m_isInitiallyOverlapping = true;

                // If we need to solve the depenetration, run a geometry query
                if ( rules.m_calculateDepenetration )
                {
                    PxSweepHit hitInfo;
                    PxHitFlags const hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMTD;
                    PxU32 hitCount = PxGeometryQuery::sweep( pxSweepDir, 0.0f, geo, pxStartTransform, pxHit.shape->getGeometry(), pxHit.actor->getGlobalPose(), hitInfo, hitFlags, 0.0f );
                    EE_ASSERT( hitCount == 1 );
                    hit.m_contactPoint = FromPx( hitInfo.position );
                    hit.m_normal = FromPx( hitInfo.normal );
                    hit.m_distance = -hitInfo.distance;
                }
                else // Clear all the values
                {
                    hit.m_contactPoint = hit.m_shapePosition;
                    hit.m_normal = Vector::Zero;
                    hit.m_distance = 0;
                }
            }
            else // Regular hit
            {
                hit.m_shapePosition = outResults.m_sweepStartPosition + ( direction * pxHit.distance );
                hit.m_contactPoint = FromPx( pxHit.position );
                hit.m_normal = FromPx( pxHit.normal );
                hit.m_distance = pxHit.distance;
                hit.m_isInitiallyOverlapping = false;
            }
        };

        // Fill Results
        //-------------------------------------------------------------------------

        outResults.m_sweepStartPosition = startTransform.GetTranslation();
        outResults.m_sweepDirection = direction;
        outResults.m_sweepEndPosition = outResults.m_sweepStartPosition + ( direction * distance );
        outResults.m_desiredDistance = distance;

        if ( result )
        {
            // Add block
            if ( buffer.hasBlock )
            {
                ConvertHit( buffer.block );
            }

            // Add additional touches
            for ( uint32_t i = 0; i < buffer.nbTouches; i++ )
            {
                ConvertHit( buffer.getTouch( i ) );
            }

            //-------------------------------------------------------------------------

            eastl::sort( outResults.m_hits.begin(), outResults.m_hits.end(), [] ( SweepResults::Hit const& a, SweepResults::Hit const& b ) { return a.m_distance < b.m_distance; } );

            //-------------------------------------------------------------------------

            auto const& firstHit = outResults.m_hits[0];
            if ( firstHit.m_isInitiallyOverlapping )
            {
                outResults.m_hasInitialOverlap = true;
                outResults.m_actualDistance = 0;
                outResults.m_remainingDistance = distance;
            }
            else
            {
                outResults.m_actualDistance = firstHit.m_distance;
                outResults.m_remainingDistance = distance - firstHit.m_distance;
            }
        }
        else // No hits
        {
            outResults.m_actualDistance = distance;
            outResults.m_remainingDistance = 0.0f;
        }

        //-------------------------------------------------------------------------

        return result;
    }

    bool PhysicsWorld::SphereSweepInternal( float radius, Vector const& position, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults )
    {
        PxSphereGeometry const sphereGeo( radius );
        return SweepInternal( sphereGeo, Transform( Quaternion::Identity, position ), direction, distance, rules, outResults );
    }

    bool PhysicsWorld::CapsuleSweepInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults )
    {
        PxCapsuleGeometry const capsuleGeo( radius, cylinderPortionHalfHeight );
        return SweepInternal( capsuleGeo, Transform( PX::Conversion::s_capsuleConversionToPx * orientation, position ), direction, distance, rules, outResults );
    }

    bool PhysicsWorld::CylinderSweepInternal( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults )
    {
        PxConvexMeshGeometry const cylinderGeo( PX::Shapes::s_pUnitCylinderMesh, PxMeshScale( PxVec3( 2.0f * radius, 2.0f * radius, 2.0f * cylinderPortionHalfHeight ) ) );
        return SweepInternal( cylinderGeo, Transform( orientation, position ), direction, distance, rules, outResults );
    }

    bool PhysicsWorld::BoxSweepInternal( Vector halfExtents, Quaternion const& orientation, Vector const& position, Vector const& direction, float distance, QueryRules const& rules, SweepResults& outResults )
    {
        PxBoxGeometry const boxGeo( ToPx( halfExtents ) );
        return SweepInternal( boxGeo, Transform( orientation, position ), direction, distance, rules, outResults );
    }

    //-------------------------------------------------------------------------
    // Overlaps
    //-------------------------------------------------------------------------

    bool PhysicsWorld::OverlapInternal( PxGeometry const& geo, Transform const& transform, QueryRules const& rules, OverlapResults& outResults )
    {
        PX::QueryFilter filterCallback( rules );
        PxOverlapBufferN<50> buffer;
        PxTransform const pxTransform( ToPx( transform ) );
        bool const result = m_pScene->overlap( geo, pxTransform, buffer, rules.m_queryFilterData, &filterCallback );

        //-------------------------------------------------------------------------

        auto ConvertOverlap = [&] ( PxOverlapHit const& hit )
        {
            auto& overlap = outResults.m_overlaps.emplace_back();
            overlap.m_pActor = hit.actor;
            overlap.m_pShape = hit.shape;

            if ( rules.m_calculateDepenetration )
            {
                PxSweepHit hitInfo;
                PxHitFlags const hitFlags = PxHitFlag::eNORMAL | PxHitFlag::eMTD;
                PxU32 hitCount = PxGeometryQuery::sweep( PxVec3( 0, 0, 1 ), 0.0f, geo, pxTransform, hit.shape->getGeometry(), hit.actor->getGlobalPose(), hitInfo, hitFlags, 0.0f );
                EE_ASSERT( hitCount == 1 );
                overlap.m_normal = FromPx( hitInfo.normal );
                overlap.m_distance = -hitInfo.distance;
            }
            else
            {
                overlap.m_normal = Vector::Zero;
                overlap.m_distance = 0.0f;
            }
        };

        // Fill Results
        //-------------------------------------------------------------------------

        outResults.m_overlapPosition = transform.GetTranslation();

        if ( result )
        {
            // Add block
            if ( buffer.hasBlock )
            {
                ConvertOverlap( buffer.block );
            }

            // Add additional touches
            for ( uint32_t i = 0; i < buffer.nbTouches; i++ )
            {
                ConvertOverlap( buffer.getTouch( i ) );
            }
        }

        //-------------------------------------------------------------------------

        return result;
    }

    bool PhysicsWorld::SphereOverlap( float radius, Vector const& position, QueryRules const& rules, OverlapResults& outResults )
    {
        PxSphereGeometry const sphereGeo( radius );
        return OverlapInternal( sphereGeo, Transform( Quaternion::Identity, position ), rules, outResults );
    }

    bool PhysicsWorld::CapsuleOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, QueryRules const& rules, OverlapResults& outResults )
    {
        PxCapsuleGeometry const capsuleGeo( radius, cylinderPortionHalfHeight );
        return OverlapInternal( capsuleGeo, Transform( PX::Conversion::s_capsuleConversionToPx * orientation, position ), rules, outResults);
    }

    bool PhysicsWorld::CylinderOverlap( float radius, float cylinderPortionHalfHeight, Quaternion const& orientation, Vector const& position, QueryRules const& rules, OverlapResults& outResults )
    {
        PxConvexMeshGeometry const cylinderGeo( PX::Shapes::s_pUnitCylinderMesh, PxMeshScale( PxVec3( 2.0f * radius, 2.0f * radius, 2.0f * cylinderPortionHalfHeight ) ) );
        return OverlapInternal( cylinderGeo, Transform( orientation, position ), rules, outResults );
    }

    bool PhysicsWorld::BoxOverlap( Vector halfExtents, Quaternion const& orientation, Vector const& position, QueryRules const& rules, OverlapResults& outResults )
    {
        PxBoxGeometry const boxGeo( ToPx( halfExtents ) );
        return OverlapInternal( boxGeo, Transform( orientation, position ), rules, outResults );
    }

    //-------------------------------------------------------------------------
    // Actors and Shapes
    //-------------------------------------------------------------------------

    bool PhysicsWorld::CreateActor( PhysicsShapeComponent* pComponent ) const
    {
        EE_ASSERT( pComponent != nullptr );
        PxPhysics* pPhysics = &m_pScene->getPhysics();

        if ( !pComponent->HasValidPhysicsSetup() )
        {
            EE_LOG_ENTITY_WARNING( pComponent, "Physics", "Invalid physics setup set for component: %s (%u), no physics actors will be created!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName = pComponent->GetNameID().IsValid() ? pComponent->GetNameID().c_str() : "Invalid Name";
        #endif

        // Create Actor
        //-------------------------------------------------------------------------

        Transform const& worldTransform = pComponent->ConvertTransformToPhysics( pComponent->GetWorldTransform() );
        PxTransform const pxTransform = ToPx( worldTransform );
        PxRigidActor* pPhysicsActor = nullptr;

        SimulationSettings const& settings = pComponent->GetSimulationSettings();
        switch ( settings.m_mobility )
        {
            case Mobility::Static:
            {
                pPhysicsActor = pPhysics->createRigidStatic( pxTransform );
            }
            break;

            case Mobility::Dynamic:
            {
                PxRigidDynamic * pRigidDynamicActor = pPhysics->createRigidDynamic( pxTransform );
                PxRigidBodyExt::setMassAndUpdateInertia( *pRigidDynamicActor, settings.m_mass );
                pPhysicsActor = pRigidDynamicActor;

                // Disable dynamic actors by default in non game worlds
                if ( !m_isGameWorld )
                {
                    pRigidDynamicActor->setActorFlag( PxActorFlag::eDISABLE_SIMULATION, true );
                }
            }
            break;

            case Mobility::Kinematic:
            {
                PxRigidDynamic* pRigidDynamicActor = pPhysics->createRigidDynamic( pxTransform );
                pRigidDynamicActor->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );
                pRigidDynamicActor->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true );
                pPhysicsActor = pRigidDynamicActor;
            }
            break;
        }

        pPhysicsActor->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, !settings.m_enableGravity );
        pPhysicsActor->userData = pComponent;

        #if EE_DEVELOPMENT_TOOLS
        pPhysicsActor->setName( pComponent->m_debugName.c_str() );
        #endif

        // Create Shape
        //-------------------------------------------------------------------------
        // We also calculate and set the component local bounds

        PxShape* pPhysicsShape = nullptr;
        PxShapeFlags const shapeFlags( PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE );
        float const scale = worldTransform.GetScale();

        if ( auto pMeshComponent = TryCast<CollisionMeshComponent>( pComponent ) )
        {
            EE_ASSERT( pMeshComponent->m_collisionMesh->IsValid() );

            auto const& physicsMaterialIDs = pMeshComponent->m_collisionMesh->GetPhysicsMaterials();
            TInlineVector<PxMaterial*, 4> physicsMaterials;
            for ( auto const& materialID : physicsMaterialIDs )
            {
                PxMaterial* pPhysicsMaterial = m_pMaterialRegistry->GetMaterial( materialID );
                if ( pPhysicsMaterial == nullptr )
                {
                    EE_LOG_ENTITY_ERROR( pComponent, "Physics", "Invalid physics materials for collision mesh (%s) on component %s (%u). No shapes will be created!", pMeshComponent->m_collisionMesh.GetResourceID().c_str(), pComponent->GetNameID().c_str(), pComponent->GetID() );
                    return false;
                }

                physicsMaterials.emplace_back( pPhysicsMaterial );
            }

            if ( physicsMaterials.empty() )
            {
                EE_LOG_ENTITY_ERROR( pComponent, "Physics", "No physics materials set for collision mesh (%s) on component %s (%u). No shapes will be created!", pMeshComponent->m_collisionMesh.GetResourceID().c_str(), pComponent->GetNameID().c_str(), pComponent->GetID() );
                return false;
            }

            Vector const finalScale = pMeshComponent->GetLocalScale() * scale;

            // Triangle Mesh
            if ( pMeshComponent->m_collisionMesh->IsTriangleMesh() )
            {
                PxTriangleMesh const* pTriMesh = pMeshComponent->m_collisionMesh->GetMesh()->is<PxTriangleMesh>();
                PxTriangleMeshGeometry const meshGeo( const_cast<PxTriangleMesh*>( pTriMesh ), ToPx( finalScale ) );
                pPhysicsShape = pPhysics->createShape( meshGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
            }
            else // Convex Mesh
            {
                PxConvexMesh const* pConvexMesh = pMeshComponent->m_collisionMesh->GetMesh()->is<PxConvexMesh>();
                PxConvexMeshGeometry const meshGeo( const_cast<PxConvexMesh*>( pConvexMesh ), ToPx( finalScale ) );
                pPhysicsShape = pPhysics->createShape( meshGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
            }
        }
        else if ( auto pBoxComponent = TryCast<BoxComponent>( pComponent ) )
        {
            PxMaterial* const material[1] = { m_pMaterialRegistry->GetMaterial( pBoxComponent->m_materialID ) };
            Vector const scaledExtents = pBoxComponent->m_boxHalfExtents * scale;
            PxBoxGeometry const boxGeo( ToPx( scaledExtents ) );
            pPhysicsShape = pPhysics->createShape( boxGeo, material, 1, true, shapeFlags );
        }
        else if ( auto pSphereComponent = TryCast<SphereComponent>( pComponent ) )
        {
            PxMaterial* const material[1] = { m_pMaterialRegistry->GetMaterial( pSphereComponent->m_materialID ) };
            float const scaledRadius = pSphereComponent->m_radius * scale;
            PxSphereGeometry const sphereGeo( scaledRadius );
            pPhysicsShape = pPhysics->createShape( sphereGeo, material, 1, true, shapeFlags );
        }
        else if ( auto pCapsuleComponent = TryCast<CapsuleComponent>( pComponent ) )
        {
            PxMaterial* const material[1] = { m_pMaterialRegistry->GetMaterial( pCapsuleComponent->m_materialID ) };
            float const scaledRadius = pCapsuleComponent->m_radius * scale;
            float const scaledHalfHeight = pCapsuleComponent->m_cylinderPortionHalfHeight * scale;
            PxCapsuleGeometry const capsuleGeo( scaledRadius, scaledHalfHeight );
            pPhysicsShape = pPhysics->createShape( capsuleGeo, material, 1, true, shapeFlags );
        }

        if ( pPhysicsShape == nullptr )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "Failed to create physics shape for component %s (%u)!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            pPhysicsActor->release();
            return false;
        }

        // Simulation flags - Word 0 is the category and Word1 is the collision mask
        PxFilterData const simulationFilterData( 1 << (uint32_t) pComponent->m_collisionSettings.m_category, pComponent->m_collisionSettings.m_collidesWithMask, 0, 0 );
        pPhysicsShape->setSimulationFilterData( simulationFilterData );

        // Query flags - Word 0 is the collision mask
        PxFilterData const queryFilterData( pComponent->m_collisionSettings.m_collidesWithMask, 0, 0, 0 );
        pPhysicsShape->setQueryFilterData( queryFilterData );

        // User data
        pPhysicsShape->userData = pComponent;

        // Debug name
        #if EE_DEVELOPMENT_TOOLS
        pPhysicsShape->setName( pComponent->m_debugName.c_str() );
        #endif

        pPhysicsActor->attachShape( *pPhysicsShape );
        pPhysicsShape->release(); // Release temp reference

        // Component
        //-------------------------------------------------------------------------

        pComponent->m_pPhysicsActor = pPhysicsActor;
        pComponent->m_pPhysicsShape = pPhysicsShape;

        // Add to scene
        //-------------------------------------------------------------------------

        m_pScene->lockWrite();
        m_pScene->addActor( *pPhysicsActor );
        m_pScene->unlockWrite();

        return true;
    }

    void PhysicsWorld::DestroyActor( PhysicsShapeComponent* pComponent ) const
    {
        PxScene* pPxScene = m_pScene;
        if ( pComponent->m_pPhysicsActor != nullptr )
        {
            EE_ASSERT( pComponent->m_pPhysicsActor->getScene() != nullptr );

            pPxScene->lockWrite();
            pPxScene->removeActor( *pComponent->m_pPhysicsActor );
            pPxScene->unlockWrite();
            pComponent->m_pPhysicsActor->release();
        }

        //-------------------------------------------------------------------------

        pComponent->m_pPhysicsShape = nullptr;
        pComponent->m_pPhysicsActor = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName.clear();
        #endif
    }

    bool PhysicsWorld::CreateCharacterController( CharacterComponent* pComponent ) const
    {
        EE_ASSERT( pComponent != nullptr );
        PxPhysics* pPhysics = &m_pScene->getPhysics();

        if ( !pComponent->HasValidCharacterSetup() )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "No Physics Material set for component: %s, no physics actors will be created!", pComponent->GetNameID().c_str() );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName = pComponent->GetNameID().c_str();
        #endif

        // Create controller
        //-------------------------------------------------------------------------

        PxTransform const worldTransform = ToPxCapsuleTransform( pComponent->GetWorldTransform() );

        PxCapsuleControllerDesc controllerDesc;
        controllerDesc.radius = pComponent->m_radius;
        controllerDesc.height = pComponent->m_halfHeight * 2;
        controllerDesc.upDirection = PxVec3( 0, 0, 1 );
        controllerDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
        controllerDesc.position = PxExtendedVec3( worldTransform.p.x, worldTransform.p.y, worldTransform.p.z );
        controllerDesc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
        controllerDesc.slopeLimit = Radians( pComponent->m_defaultSlopeLimit ).ToFloat();
        controllerDesc.stepOffset = pComponent->m_defaultStepHeight;
        controllerDesc.material = m_pMaterialRegistry->GetDefaultMaterial();
        controllerDesc.reportCallback = &pComponent->m_callbackHandler;
        controllerDesc.contactOffset = 0.01f; // 1cm
        controllerDesc.scaleCoeff = 0.95f;
        //controllerDesc.behaviorCallback = &pComponent->m_callbackHandler;

        m_pScene->lockWrite();
        PxCapsuleController* pController = static_cast<PxCapsuleController*> ( m_pControllerManager->createController( controllerDesc ) );
        if ( pController != nullptr )
        {
            PxRigidDynamic* pPhysicsActor = pController->getActor();
            pPhysicsActor->userData = pComponent;

            #if EE_DEVELOPMENT_TOOLS
            pPhysicsActor->setName( pComponent->m_debugName.c_str() );
            #endif
        }
        m_pScene->unlockWrite();

        if ( pController == nullptr )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "failed to create physics character controller: %s!", pComponent->GetNameID().c_str() );
            return false;
        }

        // Component
        //-------------------------------------------------------------------------

        pComponent->m_pController = pController;

        return true;
    }

    void PhysicsWorld::DestroyCharacterController( CharacterComponent* pComponent ) const
    {
        if ( pComponent->m_pController != nullptr )
        {
            m_pScene->lockWrite();
            pComponent->m_pController->release();
            pComponent->m_pController = nullptr;
            m_pScene->unlockWrite();
        }

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName.clear();
        #endif
    }

    Ragdoll* PhysicsWorld::CreateRagdoll( RagdollDefinition const* pDefinition, StringID const& profileID, uint64_t userID )
    {
        EE_ASSERT( m_pScene != nullptr && pDefinition != nullptr );
        auto pRagdoll = EE::New<Ragdoll>( pDefinition, profileID, userID );
        pRagdoll->AddToScene( m_pScene );
        return pRagdoll;
    }

    void PhysicsWorld::DestroyRagdoll( Ragdoll*& pRagdoll )
    {
        EE_ASSERT( pRagdoll );
        pRagdoll->RemoveFromScene();
        EE::Delete( pRagdoll );
    }

    //------------------------------------------------------------------------- 
    // Debug
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    bool PhysicsWorld::IsDebugDrawingEnabled() const
    {
        uint32_t const result = m_sceneDebugFlags & ( 1 << PxVisualizationParameter::eSCALE );
        return result != 0;
    }

    void PhysicsWorld::SetDebugDrawingEnabled( bool enableDrawing )
    {
        if ( enableDrawing )
        {
            m_sceneDebugFlags |= ( 1 << PxVisualizationParameter::eSCALE );
        }
        else
        {
            m_sceneDebugFlags = m_sceneDebugFlags & ~( 1 << PxVisualizationParameter::eSCALE );
        }

        SetDebugFlags( m_sceneDebugFlags );
    }

    void PhysicsWorld::SetDebugFlags( uint32_t debugFlags )
    {
        EE_ASSERT( m_pScene != nullptr );
        m_sceneDebugFlags = debugFlags;

        //-------------------------------------------------------------------------

        auto SetVisualizationParameter = [this] ( PxVisualizationParameter::Enum flag, float onValue, float offValue )
        {
            bool const isFlagSet = ( m_sceneDebugFlags & ( 1 << flag ) ) != 0;
            m_pScene->setVisualizationParameter( flag, isFlagSet ? onValue : offValue );
        };

        m_pScene->lockWrite();
        SetVisualizationParameter( PxVisualizationParameter::eSCALE, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCOLLISION_AABBS, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCOLLISION_AXES, 0.25f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCOLLISION_FNORMALS, 0.15f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCOLLISION_EDGES, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCONTACT_POINT, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCONTACT_NORMAL, 0.25f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eCONTACT_FORCE, 0.25f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eACTOR_AXES, 0.25f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eBODY_AXES, 0.25f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eBODY_LIN_VELOCITY, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eBODY_ANG_VELOCITY, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eBODY_MASS_AXES, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eJOINT_LIMITS, 1.0f, 0.0f );
        SetVisualizationParameter( PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f, 0.0f );
        m_pScene->unlockWrite();
    }

    void PhysicsWorld::SetDebugCullingBox( AABB const& cullingBox )
    {
        m_pScene->lockWrite();
        m_pScene->setVisualizationCullingBox( ToPx( cullingBox ) );
        m_pScene->unlockWrite();
    }

    PxRenderBuffer const& PhysicsWorld::GetRenderBuffer() const
    {
        return m_pScene->getRenderBuffer();
    }
    #endif
}
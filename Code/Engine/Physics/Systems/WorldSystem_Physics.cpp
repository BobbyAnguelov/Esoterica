#include "WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Physics/Components/Component_PhysicsMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCapsule.h"
#include "Engine/Physics/Components/Component_PhysicsSphere.h"
#include "Engine/Physics/Components/Component_PhysicsBox.h"
#include "Engine/Physics/PhysX.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityLog.h"
#include "System/Math/BoundingVolumes.h"
#include "System/Profiling.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    static inline PxBoxGeometry CreateBoxGeometry( Vector const& scale, Vector const& extents, OBB* pLocalBounds = nullptr )
    {
        EE_ASSERT( !scale.GetWithW1().IsAnyEqualsZero());

        if ( pLocalBounds != nullptr )
        {
            *pLocalBounds = OBB( Vector::Origin, extents );
        }

        Vector const scaledExtents = scale * extents;
        return PxBoxGeometry( ToPx( scaledExtents ) );
    }

    static inline PxSphereGeometry CreateSphereGeometry( Vector const& scale, float radius, OBB* pLocalBounds = nullptr )
    {
        float const radiusScale = Math::Max( Math::Max( scale.m_y, scale.m_z ), scale.m_z );
        EE_ASSERT( !Math::IsNearZero( radiusScale ) );
        float const scaledRadius = radiusScale * radius;

        if ( pLocalBounds != nullptr )
        {
            Vector const boundsExtents = Vector( scaledRadius ) / scale;
            *pLocalBounds = OBB( Vector::Origin, boundsExtents );
        }

        return PxSphereGeometry( scaledRadius );
    }

    static inline PxCapsuleGeometry CreateCapsuleGeometry( Vector const& scale, float radius, float halfHeight, OBB* pLocalBounds = nullptr )
    {
        float const heightScale = scale.m_x;
        EE_ASSERT( !Math::IsNearZero( heightScale ) );
        float const radiusScale = Math::Max( scale.m_y, scale.m_z );
        EE_ASSERT( !Math::IsNearZero( radiusScale ) );

        float const scaledRadius = radius * radiusScale;
        float const scaledHalfHeight = halfHeight * heightScale;

        if ( pLocalBounds != nullptr )
        {
            // Calculate the scaled bounds and remove the scale from them
            Vector boundsExtents;
            boundsExtents.m_x = scaledHalfHeight + scaledRadius;
            boundsExtents.m_y = scaledRadius;
            boundsExtents.m_z = scaledRadius;
            boundsExtents /= scale;

            *pLocalBounds = OBB( Vector::Origin, boundsExtents );
        }

        return PxCapsuleGeometry( scaledRadius, scaledHalfHeight );
    }

    //-------------------------------------------------------------------------

    PhysicsWorldSystem::PhysicsWorldSystem( PhysicsSystem& physicsSystem )
        : m_pPhysicsSystem( &physicsSystem )
    {}

    physx::PxScene* PhysicsWorldSystem::GetPxScene()
    {
        return m_pScene->m_pScene;
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        m_shapeTransformChangedBindingID = PhysicsShapeComponent::OnStaticActorTransformUpdated().Bind( [this] ( PhysicsShapeComponent* pShapeComponent ) { OnStaticShapeTransformUpdated( pShapeComponent ); } );

        m_pPhysicsSystem = systemRegistry.GetSystem<PhysicsSystem>();
        EE_ASSERT( m_pPhysicsSystem != nullptr );

        m_pScene = m_pPhysicsSystem->CreateScene();
        EE_ASSERT( m_pScene != nullptr );

        #if EE_DEVELOPMENT_TOOLS
        SetDebugFlags( 1 << PxVisualizationParameter::eCOLLISION_SHAPES );
        #endif
    }

    void PhysicsWorldSystem::ShutdownSystem()
    {
        // Destroy scene
        EE::Delete( m_pScene );
        m_pPhysicsSystem = nullptr;

        PhysicsShapeComponent::OnStaticActorTransformUpdated().Unbind( m_shapeTransformChangedBindingID );

        EE_ASSERT( m_staticActorShapeUpdateList.empty() );
        EE_ASSERT( m_physicsShapeComponents.empty() );
        EE_ASSERT( m_dynamicShapeComponents.empty() );
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pPhysicsComponent = TryCast<PhysicsShapeComponent>( pComponent ) )
        {
            m_physicsShapeComponents.Add( pPhysicsComponent );

            if ( pPhysicsComponent->m_actorType != ActorType::Static )
            {
                m_dynamicShapeComponents.Add( pPhysicsComponent );
            }

            if ( !CreateActorAndShape( pPhysicsComponent ) )
            {
                EE_LOG_ENTITY_ERROR( pPhysicsComponent, "Physics", "Failed to create physics actor/shape for shape component %s (%u)!", pPhysicsComponent->GetNameID().c_str(), pPhysicsComponent->GetID() );
            }
        }

        //-------------------------------------------------------------------------

        if ( auto pCharacterComponent = TryCast<CharacterComponent>( pComponent ) )
        {
            m_characterComponents.Add( pCharacterComponent );
            if ( !CreateCharacterActorAndShape( pCharacterComponent ) )
            {
                EE_LOG_ENTITY_ERROR( pCharacterComponent, "Physics", "Failed to create physics actor/shape for character %s (%u)!", pCharacterComponent->GetNameID().c_str(), pCharacterComponent->GetID() );
            }
        }
    }

    void PhysicsWorldSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pPhysicsComponent = TryCast<PhysicsShapeComponent>( pComponent ) )
        {
            if ( pPhysicsComponent->m_pPhysicsActor != nullptr && pPhysicsComponent->m_actorType != ActorType::Static )
            {
                m_dynamicShapeComponents.Remove( pPhysicsComponent->GetID() );
            }

            m_staticActorShapeUpdateList.erase_first_unsorted( pPhysicsComponent );
            m_physicsShapeComponents.Remove( pComponent->GetID() );

            DestroyActor( pPhysicsComponent );
        }

        //-------------------------------------------------------------------------

        if ( auto pCharacterComponent = TryCast<CharacterComponent>( pComponent ) )
        {
            DestroyCharacterActor( pCharacterComponent );
            m_characterComponents.Remove( pComponent->GetID() );
        }
    }

    //-------------------------------------------------------------------------

    bool PhysicsWorldSystem::CreateActorAndShape( PhysicsShapeComponent* pComponent ) const
    {
        if ( !pComponent->HasValidPhysicsSetup() )
        {
            EE_LOG_ENTITY_WARNING( pComponent, "Physics", "Invalid physics setup set for component: %s (%u), no physics actors will be created!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName = pComponent->GetNameID().IsValid() ? pComponent->GetNameID().c_str() : "Invalid Name";
        #endif

        PxRigidActor* pPhysicsActor = CreateActor( pComponent );
        if ( pPhysicsActor == nullptr )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "Failed to create physics actor for component %s (%u)!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return false;
        }

        PxShape* pShape = CreateShape( pComponent, pPhysicsActor );
        if ( pShape == nullptr )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "Failed to create physics shape for component %s (%u)!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return false;
        }

        // Add actor to scene
        PxScene* pPxScene = m_pScene->m_pScene;
        pPxScene->lockWrite();
        pPxScene->addActor( *pPhysicsActor );
        pPxScene->unlockWrite();
        return true;
    }

    physx::PxRigidActor* PhysicsWorldSystem::CreateActor( PhysicsShapeComponent* pComponent ) const
    {
        EE_ASSERT( pComponent != nullptr );

        PxPhysics* pPhysics = m_pPhysicsSystem->GetPxPhysics();

        // Create and setup actor
        //-------------------------------------------------------------------------

        Transform const& worldTransform = pComponent->GetWorldTransform();
        PxTransform const actorPose( ToPx( worldTransform.GetTranslation() ), ToPx( worldTransform.GetRotation() ) );

        physx::PxRigidActor* pPhysicsActor = nullptr;

        switch ( pComponent->m_actorType )
        {
            case ActorType::Static:
            {
                pPhysicsActor = pPhysics->createRigidStatic( actorPose );
            }
            break;

            case ActorType::Dynamic:
            {
                pPhysicsActor = pPhysics->createRigidDynamic( actorPose );
            }
            break;

            case ActorType::Kinematic:
            {
                PxRigidDynamic* pRigidDynamicActor = pPhysics->createRigidDynamic( actorPose );
                pRigidDynamicActor->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );
                pRigidDynamicActor->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true );
                PxRigidBodyExt::setMassAndUpdateInertia( *pRigidDynamicActor, pComponent->m_mass );
                pPhysicsActor = pRigidDynamicActor;
            }
            break;
        }

        // Set up component <-> physics links
        //-------------------------------------------------------------------------

        pPhysicsActor->userData = pComponent;
        pComponent->m_pPhysicsActor = pPhysicsActor;

        #if EE_DEVELOPMENT_TOOLS
        pPhysicsActor->setName( pComponent->m_debugName.c_str() );
        #endif

        //-------------------------------------------------------------------------

        return pPhysicsActor;
    }

    physx::PxShape* PhysicsWorldSystem::CreateShape( PhysicsShapeComponent* pComponent, physx::PxRigidActor* pPhysicsActor ) const
    {
        EE_ASSERT( pComponent != nullptr && pPhysicsActor != nullptr );

        physx::PxShape* pPhysicsShape = nullptr;

        // Get physics materials
        //-------------------------------------------------------------------------

        // Get physic materials
        TInlineVector<StringID, 4> const physicsMaterialIDs = pComponent->GetPhysicsMaterialIDs();
        TInlineVector<PxMaterial*, 4> physicsMaterials;

        for ( auto const& materialID : physicsMaterialIDs )
        {
            PxMaterial* pPhysicsMaterial = m_pPhysicsSystem->GetMaterial( materialID );
            if ( pPhysicsMaterial == nullptr )
            {
                return false;
            }

            physicsMaterials.emplace_back( pPhysicsMaterial );
        }

        if ( physicsMaterials.empty() )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "No physics materials set for component %s (%u). No shapes will be created!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return false;
        }

        // Calculate shape flags
        //-------------------------------------------------------------------------

        PxShapeFlags shapeFlags( PxShapeFlag::eVISUALIZATION );

        switch ( pComponent->m_shapeType )
        {
            case ShapeType::SimulationAndQuery:
            {
                shapeFlags |= PxShapeFlag::eSCENE_QUERY_SHAPE;
                shapeFlags |= PxShapeFlag::eSIMULATION_SHAPE;
            }
            break;

            case ShapeType::SimulationOnly:
            {
                shapeFlags |= PxShapeFlag::eSIMULATION_SHAPE;
            }
            break;

            case ShapeType::QueryOnly:
            {
                shapeFlags |= PxShapeFlag::eSCENE_QUERY_SHAPE;
            }
            break;
        }

        // Create PhysX shape
        //-------------------------------------------------------------------------
        // We also calculate and set the component local bounds

        Transform const& physicsComponentTransform = pComponent->GetWorldTransform();
        Vector const& scale = physicsComponentTransform.GetScale();

        PxPhysics* pPhysics = m_pPhysicsSystem->GetPxPhysics();
        OBB localBounds;
        if ( auto pMeshComponent = TryCast<PhysicsMeshComponent>( pComponent ) )
        {
            EE_ASSERT( pMeshComponent->m_physicsMesh->IsValid() );

            // Triangle Mesh
            if ( pMeshComponent->m_physicsMesh->IsTriangleMesh() )
            {
                PxTriangleMesh const* pTriMesh = pMeshComponent->m_physicsMesh->GetTriangleMesh();
                PxTriangleMeshGeometry const meshGeo( const_cast<PxTriangleMesh*>( pTriMesh ), ToPx( scale ) );
                pPhysicsShape = pPhysics->createShape( meshGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
                localBounds = OBB( FromPx( pTriMesh->getLocalBounds() ) );
            }
            else // Convex Mesh
            {
                PxConvexMesh const* pConvexMesh = pMeshComponent->m_physicsMesh->GetConvexMesh();
                PxConvexMeshGeometry const meshGeo( const_cast<PxConvexMesh*>( pConvexMesh ), ToPx( scale ) );
                pPhysicsShape = pPhysics->createShape( meshGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
                localBounds = OBB( FromPx( pConvexMesh->getLocalBounds() ) );
            }
        }
        else if ( auto pBoxComponent = TryCast<BoxComponent>( pComponent ) )
        {
            PxBoxGeometry const boxGeo = CreateBoxGeometry( scale, pBoxComponent->m_boxExtents, &localBounds );
            pPhysicsShape = pPhysics->createShape( boxGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
        }
        else if ( auto pSphereComponent = TryCast<SphereComponent>( pComponent ) )
        {
            PxSphereGeometry const sphereGeo = CreateSphereGeometry( scale, pSphereComponent->m_radius, &localBounds );
            pPhysicsShape = pPhysics->createShape( sphereGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
        }
        else if ( auto pCapsuleComponent = TryCast<CapsuleComponent>( pComponent ) )
        {
            PxCapsuleGeometry const capsuleGeo = CreateCapsuleGeometry( scale, pCapsuleComponent->m_radius, pCapsuleComponent->m_cylinderPortionHalfHeight, &localBounds );
            pPhysicsShape = pPhysics->createShape( capsuleGeo, physicsMaterials.data(), (uint16_t) physicsMaterials.size(), true, shapeFlags );
        }

        pComponent->SetLocalBounds( localBounds );

        // Set component <-> physics shape links
        //-------------------------------------------------------------------------

        pPhysicsShape->setSimulationFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
        pPhysicsShape->setQueryFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );

        pPhysicsShape->userData = const_cast<PhysicsShapeComponent*>( pComponent );
        pComponent->m_pPhysicsShape = pPhysicsShape;

        #if EE_DEVELOPMENT_TOOLS
        pPhysicsShape->setName( pComponent->m_debugName.c_str() );
        #endif

        // Attach the shape to the new actor and release the temporary reference
        //-------------------------------------------------------------------------

        pPhysicsActor->attachShape( *pPhysicsShape );
        pPhysicsShape->release();

        //-------------------------------------------------------------------------

        return pPhysicsShape;
    }

    void PhysicsWorldSystem::UpdateStaticActorAndShape( PhysicsShapeComponent* pComponent ) const
    {
        EE_ASSERT( pComponent != nullptr );
        if ( pComponent->m_pPhysicsActor == nullptr )
        {
            return;
        }

        Transform const& worldTransform = pComponent->GetWorldTransform();
        Vector const scale = worldTransform.GetScale();

        // Update actor geometry
        //-------------------------------------------------------------------------

        OBB localBounds;
        if ( auto pMeshComponent = TryCast<PhysicsMeshComponent>( pComponent ) )
        {
            EE_ASSERT( pMeshComponent->m_physicsMesh->IsValid() );

            // Triangle Mesh
            if ( pMeshComponent->m_physicsMesh->IsTriangleMesh() )
            {
                PxTriangleMesh const* pTriMesh = pMeshComponent->m_physicsMesh->GetTriangleMesh();
                PxTriangleMeshGeometry const meshGeo( const_cast<PxTriangleMesh*>( pTriMesh ), ToPx( scale ) );
                pComponent->m_pPhysicsShape->setGeometry( meshGeo );
                localBounds = OBB( FromPx( pTriMesh->getLocalBounds() ) );
            }
            else // Convex Mesh
            {
                PxConvexMesh const* pConvexMesh = pMeshComponent->m_physicsMesh->GetConvexMesh();
                PxConvexMeshGeometry const meshGeo( const_cast<PxConvexMesh*>( pConvexMesh ), ToPx( scale ) );
                pComponent->m_pPhysicsShape->setGeometry( meshGeo );
                localBounds = OBB( FromPx( pConvexMesh->getLocalBounds() ) );
            }
        }
        else if ( auto pBoxComponent = TryCast<BoxComponent>( pComponent ) )
        {
            PxBoxGeometry const boxGeo = CreateBoxGeometry( scale, pBoxComponent->m_boxExtents, &localBounds );
            pComponent->m_pPhysicsShape->setGeometry( boxGeo );
        }
        else if ( auto pSphereComponent = TryCast<SphereComponent>( pComponent ) )
        {
            PxSphereGeometry const sphereGeo = CreateSphereGeometry( scale, pSphereComponent->m_radius, &localBounds );
            pComponent->m_pPhysicsShape->setGeometry( sphereGeo );
            
        }
        else if ( auto pCapsuleComponent = TryCast<CapsuleComponent>( pComponent ) )
        {
            PxCapsuleGeometry const capsuleGeo = CreateCapsuleGeometry( scale, pCapsuleComponent->m_radius, pCapsuleComponent->m_cylinderPortionHalfHeight, &localBounds );
            pComponent->m_pPhysicsShape->setGeometry( capsuleGeo );
        }

        pComponent->SetLocalBounds( localBounds );

        // Update actor position
        //-------------------------------------------------------------------------

        PxTransform const actorPose( ToPx( worldTransform.GetTranslation() ), ToPx( worldTransform.GetRotation() ) );
        pComponent->m_pPhysicsActor->setGlobalPose( actorPose );
    }

    void PhysicsWorldSystem::DestroyActor( PhysicsShapeComponent* pComponent ) const
    {
        PxScene* pPxScene = m_pScene->m_pScene;
        if ( pComponent->m_pPhysicsActor != nullptr )
        {
            pPxScene->lockWrite();
            pPxScene->removeActor( *pComponent->m_pPhysicsActor );
            pPxScene->unlockWrite();

            pComponent->m_pPhysicsActor->release();
        }

        //-------------------------------------------------------------------------

        pComponent->m_pPhysicsShape = nullptr;
        pComponent->m_pPhysicsActor = nullptr;
        pComponent->SetLocalBounds( OBB() );

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName.clear();
        #endif
    }

    //-------------------------------------------------------------------------

    bool PhysicsWorldSystem::CreateCharacterActorAndShape( CharacterComponent* pComponent ) const
    {
        EE_ASSERT( pComponent != nullptr );
        PxPhysics* pPhysics = m_pPhysicsSystem->GetPxPhysics();

        if ( !pComponent->HasValidPhysicsSetup() )
        {
            EE_LOG_ERROR( "Physics", "No Physics Material set for component: %s (%u), no physics actors will be created!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName = pComponent->GetNameID().c_str();
        #endif

        // Create actor
        //-------------------------------------------------------------------------

        PxTransform const bodyPose( ToPx( pComponent->m_capsuleWorldTransform.GetTranslation() ), ToPx( pComponent->m_capsuleWorldTransform.GetRotation() ) );

        PxRigidDynamic* pRigidDynamicActor = pPhysics->createRigidDynamic( bodyPose );
        pRigidDynamicActor->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );
        pRigidDynamicActor->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true );
        pRigidDynamicActor->userData = pComponent;

        #if EE_DEVELOPMENT_TOOLS
        pRigidDynamicActor->setName( pComponent->m_debugName.c_str() );
        #endif

        pComponent->m_pPhysicsActor = pRigidDynamicActor;

        // Create Capsule Shape
        //-------------------------------------------------------------------------

        PxMaterial* const defaultMaterial[1] = { m_pPhysicsSystem->GetDefaultMaterial() };
        PxShapeFlags shapeFlags( PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE );
        PxCapsuleGeometry const capsuleGeo( pComponent->m_radius, pComponent->m_cylinderPortionHalfHeight );

        pComponent->m_pCapsuleShape = pPhysics->createShape( capsuleGeo, defaultMaterial, 1, true, shapeFlags );
        pComponent->m_pCapsuleShape->setSimulationFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
        pComponent->m_pCapsuleShape->setQueryFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
        pComponent->m_pCapsuleShape->userData = pComponent;

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_pCapsuleShape->setName( pComponent->m_debugName.c_str() );
        #endif

        pComponent->m_pPhysicsActor->attachShape( *pComponent->m_pCapsuleShape );
        pComponent->m_pCapsuleShape->release();

        // Set Component Bounds
        //-------------------------------------------------------------------------

        OBB const localBounds( Vector::Origin, Vector( pComponent->m_cylinderPortionHalfHeight + pComponent->m_radius, pComponent->m_radius, pComponent->m_radius ) );
        pComponent->SetLocalBounds( localBounds );

        // Add to scene
        //-------------------------------------------------------------------------

        PxScene* pPxScene = m_pScene->m_pScene;
        pPxScene->lockWrite();
        pPxScene->addActor( *pComponent->m_pPhysicsActor );
        pPxScene->unlockWrite();
        return true;
    }

    void PhysicsWorldSystem::DestroyCharacterActor( CharacterComponent* pComponent ) const
    {
        PxScene* pPxScene = m_pScene->m_pScene;
        if ( pComponent->m_pPhysicsActor != nullptr )
        {
            pPxScene->lockWrite();
            pPxScene->removeActor( *pComponent->m_pPhysicsActor );
            pPxScene->unlockWrite();

            pComponent->m_pPhysicsActor->release();
        }

        // Clear component data
        //-------------------------------------------------------------------------

        pComponent->m_pCapsuleShape = nullptr;
        pComponent->m_pPhysicsActor = nullptr;
        pComponent->SetLocalBounds( OBB() );

        #if EE_DEVELOPMENT_TOOLS
        pComponent->m_debugName.clear();
        #endif
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::OnStaticShapeTransformUpdated( PhysicsShapeComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );

        if ( m_physicsShapeComponents.HasItemForID( pComponent->GetID() ) )
        {
            EE_ASSERT( pComponent->IsInitialized() );
            EE_ASSERT( pComponent->GetActorType() == ActorType::Static );

            m_staticActorShapeUpdateList.emplace_back( pComponent );
        }
    }

    //-------------------------------------------------------------------------
    
    void PhysicsWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        PxScene* pPxScene = m_pScene->m_pScene;

        if ( ctx.GetUpdateStage() == UpdateStage::Physics )
        {
            m_pScene->AcquireWriteLock();
            {
                EE_PROFILE_SCOPE_PHYSICS( "Simulate" );
             
                // Handle any static component updates this should not happen in the running game
                for ( auto pShapeComponent : m_staticActorShapeUpdateList )
                {
                    if ( ctx.IsGameWorld() )
                    {
                        EE_LOG_ENTITY_ERROR( pShapeComponent, "Physics", "Someone moved a static physics actor: %s with entity ID %u. This should not be done!", pShapeComponent->GetNameID().c_str(), pShapeComponent->GetEntityID().m_value );
                    }

                    UpdateStaticActorAndShape( pShapeComponent );
                }
                m_staticActorShapeUpdateList.clear();

                // TODO: run at fixed time step
                pPxScene->simulate( ctx.GetDeltaTime() );
            }

            //-------------------------------------------------------------------------

            {
                EE_PROFILE_SCOPE_PHYSICS( "Fetch Results" );
                pPxScene->fetchResults( true );
            }
            m_pScene->ReleaseWriteLock();
        }
        else if ( ctx.GetUpdateStage() == UpdateStage::PostPhysics )
        {
            EE_PROFILE_SCOPE_PHYSICS( "Update Dynamic Objects" );

            #if EE_DEVELOPMENT_TOOLS
            Drawing::DrawContext drawingContext = ctx.GetDrawingContext();
            #endif

            m_pScene->AcquireReadLock();

            //-------------------------------------------------------------------------

            for ( auto const& pDynamicPhysicsComponent : m_dynamicShapeComponents )
            {
                // Transfer physics pose back to component
                if ( pDynamicPhysicsComponent->m_pPhysicsActor != nullptr && pDynamicPhysicsComponent->m_actorType == ActorType::Dynamic )
                {
                    auto physicsPose = pDynamicPhysicsComponent->m_pPhysicsActor->getGlobalPose();
                    pDynamicPhysicsComponent->SetWorldTransform( FromPx( physicsPose ) );
                }

                // Debug
                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                if ( m_drawDynamicActorBounds && pDynamicPhysicsComponent->m_actorType == ActorType::Dynamic )
                {
                    drawingContext.DrawBox( pDynamicPhysicsComponent->GetWorldBounds(), Colors::Orange.GetAlphaVersion( 0.5f ) );
                    drawingContext.DrawWireBox( pDynamicPhysicsComponent->GetWorldBounds(), Colors::Orange );
                }

                if ( m_drawKinematicActorBounds && pDynamicPhysicsComponent->m_actorType == ActorType::Kinematic )
                {
                    drawingContext.DrawBox( pDynamicPhysicsComponent->GetWorldBounds(), Colors::HotPink.GetAlphaVersion( 0.5f ) );
                    drawingContext.DrawWireBox( pDynamicPhysicsComponent->GetWorldBounds(), Colors::HotPink );
                }
                #endif
            }

            m_pScene->ReleaseReadLock();
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }

    //------------------------------------------------------------------------- 
    // Debug
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    bool PhysicsWorldSystem::IsDebugDrawingEnabled() const
    {
        uint32_t const result = m_sceneDebugFlags & ( 1 << physx::PxVisualizationParameter::eSCALE );
        return result != 0;
    }

    void PhysicsWorldSystem::SetDebugDrawingEnabled( bool enableDrawing )
    {
        if ( enableDrawing )
        {
            m_sceneDebugFlags |= ( 1 << physx::PxVisualizationParameter::eSCALE );
        }
        else
        {
            m_sceneDebugFlags = m_sceneDebugFlags & ~( 1 << physx::PxVisualizationParameter::eSCALE );
        }

        SetDebugFlags( m_sceneDebugFlags );
    }

    void PhysicsWorldSystem::SetDebugFlags( uint32_t debugFlags )
    {
        EE_ASSERT( m_pScene != nullptr );
        PxScene* pPxScene = m_pScene->m_pScene;
        m_sceneDebugFlags = debugFlags;

        //-------------------------------------------------------------------------

        auto SetVisualizationParameter = [this, pPxScene] ( PxVisualizationParameter::Enum flag, float onValue, float offValue )
        {
            bool const isFlagSet = ( m_sceneDebugFlags & ( 1 << flag ) ) != 0;
            pPxScene->setVisualizationParameter( flag, isFlagSet ? onValue : offValue );
        };

        m_pScene->AcquireWriteLock();
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
        m_pScene->ReleaseWriteLock();
    }

    void PhysicsWorldSystem::SetDebugCullingBox( AABB const& cullingBox )
    {
        PxScene* pPxScene = m_pScene->m_pScene;
        m_pScene->AcquireWriteLock();
        pPxScene->setVisualizationCullingBox( ToPx( cullingBox ) );
        m_pScene->ReleaseWriteLock();
    }
    #endif
}
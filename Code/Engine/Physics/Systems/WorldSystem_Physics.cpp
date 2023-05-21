#include "WorldSystem_Physics.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCapsule.h"
#include "Engine/Physics/Components/Component_PhysicsSphere.h"
#include "Engine/Physics/Components/Component_PhysicsBox.h"
#include "Engine/Physics/Components/Component_PhysicsTest.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityLog.h"
#include "System/Profiling.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    void PhysicsWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        auto OnRebuild = [this] ( PhysicsShapeComponent* pShapeComponent )
        {
            Threading::ScopeLock const lock( m_mutex );
            m_actorRebuildRequests.emplace_back( pShapeComponent );
        };

        m_actorRebuildBindingID = PhysicsShapeComponent::OnRebuildBodyRequested().Bind( OnRebuild );

        //-------------------------------------------------------------------------

        m_pWorld = EE::New<PhysicsWorld>( systemRegistry.GetSystem<MaterialRegistry>(), IsInAGameWorld() );
        EE_ASSERT( m_pWorld != nullptr );
    }

    void PhysicsWorldSystem::ShutdownSystem()
    {
        EE::Delete( m_pWorld );

        PhysicsShapeComponent::OnRebuildBodyRequested().Unbind( m_actorRebuildBindingID );

        EE_ASSERT( m_actorRebuildRequests.empty() );
        EE_ASSERT( m_physicsShapeComponents.empty() );
        EE_ASSERT( m_dynamicShapeComponents.empty() );
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pPhysicsComponent = TryCast<PhysicsShapeComponent>( pComponent ) )
        {
            m_physicsShapeComponents.Add( pPhysicsComponent );

            if ( m_pWorld->CreateActor( pPhysicsComponent ) )
            {
                if ( pPhysicsComponent->IsDynamic() )
                {
                    RegisterDynamicComponent( pPhysicsComponent );
                }
            }
            else // Failed
            {
                EE_LOG_ENTITY_ERROR( pPhysicsComponent, "Physics", "Failed to create physics actor/shape for shape component %s (%u)!", pPhysicsComponent->GetNameID().c_str(), pPhysicsComponent->GetID() );
            }
        }

        //-------------------------------------------------------------------------

        if ( auto pCharacterComponent = TryCast<CharacterComponent>( pComponent ) )
        {
            m_characterComponents.Add( pCharacterComponent );
            if ( !m_pWorld->CreateCharacterController( pCharacterComponent ) )
            {
                EE_LOG_ENTITY_ERROR( pCharacterComponent, "Physics", "Failed to create physics actor/shape for character %s (%u)!", pCharacterComponent->GetNameID().c_str(), pCharacterComponent->GetID() );
            }
        }

        //-------------------------------------------------------------------------

        if ( auto pTestComponent = TryCast<PhysicsTestComponent>( pComponent ) )
        {
            m_testComponents.emplace_back( pTestComponent );
        }
    }

    void PhysicsWorldSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pPhysicsComponent = TryCast<PhysicsShapeComponent>( pComponent ) )
        {
            // Remove any pending change requests
            {
                Threading::ScopeLock const lock( m_mutex );
                for ( auto i = 0; i < m_actorRebuildRequests.size(); i++ )
                {
                    if ( m_actorRebuildRequests[i] == pPhysicsComponent )
                    {
                        m_actorRebuildRequests.erase_unsorted( m_actorRebuildRequests.begin() + i );
                        break;
                    }
                }
            }

            // Remove any tracked dynamic components
            if ( pPhysicsComponent->IsDynamic() )
            {
                UnregisterDynamicComponent( pPhysicsComponent );
            }

            // Remove from general component list
            m_physicsShapeComponents.Remove( pComponent->GetID() );

            // Destroy the actual physics body
            m_pWorld->DestroyActor( pPhysicsComponent );
        }

        //-------------------------------------------------------------------------

        if ( auto pCharacterComponent = TryCast<CharacterComponent>( pComponent ) )
        {
            m_pWorld->DestroyCharacterController( pCharacterComponent );
            m_characterComponents.Remove( pComponent->GetID() );
        }

        //-------------------------------------------------------------------------

        if ( auto pTestComponent = TryCast<PhysicsTestComponent>( pComponent ) )
        {
            m_testComponents.erase_first_unsorted( pTestComponent );
        }
    }

    void PhysicsWorldSystem::RegisterDynamicComponent( PhysicsShapeComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsActorCreated() && pComponent->IsDynamic() );
        m_dynamicShapeComponents.Add( pComponent );;
    }

    void PhysicsWorldSystem::UnregisterDynamicComponent( PhysicsShapeComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsActorCreated() && pComponent->IsDynamic() );
        m_dynamicShapeComponents.Remove( pComponent->GetID() );
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::ProcessActorRebuildRequests( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_SCOPE_PHYSICS( "Component Body Transform Update" );

        //-------------------------------------------------------------------------

        Threading::ScopeLock const lock( m_mutex );

        for ( auto const& pShapeComponent : m_actorRebuildRequests )
        {
            EE_ASSERT( pShapeComponent != nullptr );

            if ( pShapeComponent->IsActorCreated() )
            {
                m_pWorld->DestroyActor( pShapeComponent );

                if ( m_dynamicShapeComponents.HasItemForID( pShapeComponent->GetID() ) )
                {
                    UnregisterDynamicComponent( pShapeComponent );
                }
            }

            if ( pShapeComponent->HasValidPhysicsSetup() )
            {
                m_pWorld->CreateActor( pShapeComponent );

                if ( pShapeComponent->IsDynamic() )
                {
                    RegisterDynamicComponent( pShapeComponent );
                }
            }
        }
        m_actorRebuildRequests.clear();
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        // HACK HACK
        #if EE_DEVELOPMENT_TOOLS
        m_pWorld->AcquireReadLock();
        if ( ctx.GetUpdateStage() == UpdateStage::Physics )
        {
            Drawing::DrawContext drawingContext = ctx.GetDrawingContext();

            for ( auto pTestComponent : m_testComponents )
            {
                Transform const& worldTransform = pTestComponent->GetWorldTransform();
                Transform shapeOrigin( Quaternion::Identity, worldTransform.GetTranslation() );
                Vector const rayStart = worldTransform.GetTranslation();
                Vector const rayDir = worldTransform.GetForwardVector();
                Vector const rayEnd = rayStart + ( rayDir * 100 );

                QueryRules rules;
                rules.SetCollidesWith( CollisionCategory::Environment );
                rules.SetCollidesWith( QueryChannel::Navigation );
                rules.SetAllowMultipleHits( true );

                /*RayCastResults results;
                if ( m_pWorld->RayCast( rayStart, rayDir, 100.0f, rules, results ) )
                {
                    drawingContext.DrawLine( rayStart, rayEnd, Colors::Red, 2.0f );

                    for ( auto& hit : results.m_hits )
                    {
                        drawingContext.DrawPoint( hit.m_contactPoint, Colors::Red, 10.0f );
                        drawingContext.DrawArrow( hit.m_contactPoint, hit.m_contactPoint + ( hit.m_normal * 0.5f ), Colors::Pink, 10.0f );
                    }
                }
                else
                {
                    drawingContext.DrawLine( rayStart, rayEnd, Colors::Lime, 2.0f );
                }*/

                //-------------------------------------------------------------------------
                
                /*drawingContext.DrawCapsule( shapeOrigin, 0.45f, 0.45f, Colors::Yellow, 2.0f );

                SweepResults castResults;
                if ( m_pWorld->CapsuleSweep( 0.45f, 0.45f, shapeOrigin, rayDir, 100, rules, castResults ) )
                {
                    drawingContext.DrawLine( rayStart, rayEnd, Colors::Red, 2.0f );

                    for ( auto& hit : castResults.m_hits )
                    {
                        drawingContext.DrawPoint( hit.m_contactPoint, Colors::HotPink, 15.0f );
                        drawingContext.DrawCapsule( Transform( shapeOrigin.GetRotation(), hit.m_shapePosition ), 0.45f, 0.45f, hit.m_isInitiallyOverlapping ? Colors::Red : Colors::Orange, 4.0f );

                        if ( hit.m_isInitiallyOverlapping )
                        {
                            drawingContext.DrawArrow( hit.m_contactPoint, hit.m_contactPoint + ( hit.m_normal * 0.5f ), Colors::White, 4.0f );

                            Transform t( shapeOrigin.GetRotation(), hit.m_shapePosition + ( hit.m_normal * hit.m_distance ) );
                            drawingContext.DrawCapsule( t, 0.45f, 0.45f, Colors::White, 2.0f );
                        }
                        else
                        {
                            drawingContext.DrawArrow( hit.m_contactPoint, hit.m_contactPoint + ( hit.m_normal * 0.5f ), Colors::White, 4.0f );
                        }
                    }
                }
                else
                {
                    drawingContext.DrawLine( rayStart, rayEnd, Colors::Lime, 2.0f );
                }*/

                //-------------------------------------------------------------------------

                /*OverlapResults overlapResults;
                if ( m_pWorld->CapsuleOverlap( 0.45f, 0.45f, shapeOrigin, rules, overlapResults ) )
                {
                    drawingContext.DrawCapsule( shapeOrigin, 0.45f, 0.45f, Colors::Red, 3.0f);

                    for ( auto& hit : overlapResults.m_overlaps )
                    {
                        drawingContext.DrawPoint( worldTransform.GetTranslation(), Colors::Yellow, 5.0f );
                        drawingContext.DrawArrow( worldTransform.GetTranslation(), worldTransform.GetTranslation() + ( hit.m_normal * hit.m_distance ), Colors::Yellow, 2.0f );

                        Transform t( shapeOrigin.GetRotation(), worldTransform.GetTranslation() + ( hit.m_normal * hit.m_distance ) );
                        drawingContext.DrawCapsule( t, 0.45f, 0.45f, Colors::White, 2.0f );
                    }
                }
                else
                {
                    drawingContext.DrawCapsule( shapeOrigin, 0.45f, 0.45f, Colors::LimeGreen, 3.0f );
                }*/
            }
        }
        m_pWorld->ReleaseReadLock();
        #endif
        // END HACK HACK HACK

        //-------------------------------------------------------------------------

        if ( ctx.GetUpdateStage() == UpdateStage::Physics )
        {
            ProcessActorRebuildRequests( ctx );
            PhysicsUpdate( ctx );
        }
        else if ( ctx.GetUpdateStage() == UpdateStage::PostPhysics )
        {
            PostPhysicsUpdate( ctx );
        }
        else
        {
           // Do nothing for now
        }
    }

    void PhysicsWorldSystem::PhysicsUpdate( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_FUNCTION_PHYSICS();

        m_pWorld->Simulate( ctx.GetDeltaTime() );
    }

    void PhysicsWorldSystem::PostPhysicsUpdate( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_FUNCTION_PHYSICS();

        // Transfer physics poses back to dynamic components
        //-------------------------------------------------------------------------

        if ( IsInAGameWorld() )
        {
            m_pWorld->AcquireWriteLock();

            for ( auto const& pDynamicPhysicsComponent : m_dynamicShapeComponents )
            {
                EE_ASSERT( pDynamicPhysicsComponent->IsActorCreated() && pDynamicPhysicsComponent->IsDynamic() );

                if ( auto pCapsuleComponent = TryCast<CapsuleComponent>( pDynamicPhysicsComponent ) )
                {
                    auto physicsPose = pDynamicPhysicsComponent->m_pPhysicsActor->getGlobalPose();
                    pCapsuleComponent->SetWorldTransformDirectly( FromPxCapsuleTransform( physicsPose ), false );
                }
                else // Doesnt need a conversion
                {
                    auto physicsPose = pDynamicPhysicsComponent->m_pPhysicsActor->getGlobalPose();
                    pDynamicPhysicsComponent->SetWorldTransformDirectly( FromPx( physicsPose ), false );
                }
            }

            m_pWorld->ReleaseWriteLock();
        }
    }
}
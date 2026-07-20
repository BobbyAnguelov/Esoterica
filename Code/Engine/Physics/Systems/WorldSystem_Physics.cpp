#include "WorldSystem_Physics.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCapsule.h"
#include "Engine/Physics/Components/Component_PhysicsSphere.h"
#include "Engine/Physics/Components/Component_PhysicsBox.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Viewport/Viewport.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    void PhysicsWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        m_pPhysicsWorld = EE::New<PhysicsWorld>( systemRegistry, IsInAGameWorld() );
        EE_ASSERT( m_pPhysicsWorld != nullptr );
    }

    void PhysicsWorldSystem::ShutdownSystem()
    {
        EE::Delete( m_pPhysicsWorld );

        EE_ASSERT( m_physicsShapeComponents.empty() );
        EE_ASSERT( m_dynamicShapeComponents.empty() );
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pShapeComponent = TryCast<PhysicsShapeComponent>( pComponent ) )
        {
            RegisterShapeComponent( pShapeComponent );
        }
    }

    void PhysicsWorldSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pShapeComponent = TryCast<PhysicsShapeComponent>( pComponent ) )
        {
            UnregisterShapeComponent( pShapeComponent );
        }
    }

    void PhysicsWorldSystem::RegisterShapeComponent( PhysicsShapeComponent* pComponent )
    {
        // Add to track components
        m_physicsShapeComponents.Add( pComponent );

        // Bind
        m_bodyRebuildRequests.Bind( pComponent, pComponent->GetRebuildBodySignal() );

        // Create Body
        if ( !pComponent->HasValidPhysicsSetup() )
        {
            EE_LOG_ENTITY_WARNING( pComponent, "Physics", "Invalid physics setup set for component: %s (%u), no physics actors will be created!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return;
        }

        pComponent->m_pPhysicsWorld = m_pPhysicsWorld;
        if ( !pComponent->CreatePhysicsBody() )
        {
            EE_LOG_ENTITY_ERROR( pComponent, "Physics", "Failed to create physics actor/shape for shape component %s (%u)!", pComponent->GetNameID().c_str(), pComponent->GetID() );
            return;
        }

        // Register dynamic components
        if ( pComponent->IsPhysicsBodyCreated() )
        {
            if ( pComponent->IsDynamic() )
            {
                m_dynamicShapeComponents.Add( pComponent );
            }
            else if ( pComponent->IsKinematic() )
            {
                m_kinematicShapeComponents.Add( pComponent );
            }
        }
    }

    void PhysicsWorldSystem::UnregisterShapeComponent( PhysicsShapeComponent* pComponent )
    {
        // Remove any tracked dynamic components
        if ( pComponent->IsPhysicsBodyCreated() )
        {
            if ( pComponent->IsDynamic() )
            {
                EE_ASSERT( m_dynamicShapeComponents.HasItemForID( pComponent->GetID() ) );
                m_dynamicShapeComponents.Remove( pComponent->GetID() );
            }
            else if ( pComponent->IsKinematic() )
            {
                EE_ASSERT( m_kinematicShapeComponents.HasItemForID( pComponent->GetID() ) );
                m_kinematicShapeComponents.Remove( pComponent->GetID() );
            }
        }

        // Destroy the actual physics body
        pComponent->DestroyPhysicsBody();
        pComponent->m_pPhysicsWorld = nullptr;

        // Unbind
        m_bodyRebuildRequests.Unbind( pComponent, pComponent->GetRebuildBodySignal() );

        // Remove from general component list
        m_physicsShapeComponents.Remove( pComponent->GetID() );
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::ProcessBodyRebuildRequests( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_SCOPE_PHYSICS( "Component Body Transform Update" );

        //-------------------------------------------------------------------------

        TEntityMessageQueue<PhysicsShapeComponent>::Message msg;
        while( m_bodyRebuildRequests.Dequeue( msg ) )
        {
            PhysicsShapeComponent* pShapeComponent = msg.m_pComponent;
            EE_ASSERT( pShapeComponent != nullptr );

            // Destroy
            if ( pShapeComponent->IsPhysicsBodyCreated() )
            {
                if ( pShapeComponent->IsDynamic() )
                {
                    m_dynamicShapeComponents.Remove( pShapeComponent->GetID() );
                }
                else if ( pShapeComponent->IsKinematic() )
                {
                    m_kinematicShapeComponents.Remove( pShapeComponent->GetID() );
                }

                pShapeComponent->DestroyPhysicsBody();
            }

            // Create
            if ( pShapeComponent->HasValidPhysicsSetup() )
            {
                pShapeComponent->CreatePhysicsBody();

                if ( pShapeComponent->IsPhysicsBodyCreated() )
                {
                    if ( pShapeComponent->IsDynamic() )
                    {
                        m_dynamicShapeComponents.Add( pShapeComponent );
                    }
                    else if ( pShapeComponent->IsKinematic() )
                    {
                        m_kinematicShapeComponents.Add( pShapeComponent );
                    }
                }
            }
        }

        m_bodyRebuildRequests.ClearIgnoredComponents();
    }

    //-------------------------------------------------------------------------

    void PhysicsWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( ctx.GetUpdateStage() == UpdateStage::Physics )
        {
            ProcessBodyRebuildRequests( ctx );
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

        // Set kinematic target requests
        //-------------------------------------------------------------------------

        m_pPhysicsWorld->LockWrite();
        for ( PhysicsShapeComponent const* pKinematicComponent : m_kinematicShapeComponents )
        {
            if ( B3_IS_NON_NULL( pKinematicComponent->m_physicsBodyID ) )
            {
                b3Body_SetTargetTransform( pKinematicComponent->m_physicsBodyID, ToBox3D( pKinematicComponent->m_kinematicTargetTransform ), ctx.GetDeltaTime(), true );
            }
        }
        m_pPhysicsWorld->UnlockWrite();

        // Step world
        //-------------------------------------------------------------------------

        m_pPhysicsWorld->Simulate( ctx.GetDeltaTime() );
    }

    void PhysicsWorldSystem::PostPhysicsUpdate( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_FUNCTION_PHYSICS();

        // Transfer physics poses back to dynamic components
        //-------------------------------------------------------------------------

        if ( IsInAGameWorld() )
        {
            m_pPhysicsWorld->LockRead();

            for ( auto const& pDynamicPhysicsComponent : m_dynamicShapeComponents )
            {
                EE_ASSERT( pDynamicPhysicsComponent->IsPhysicsBodyCreated() && pDynamicPhysicsComponent->IsDynamic() );
                Transform const bodyTransform = FromBox3D( b3Body_GetTransform( pDynamicPhysicsComponent->m_physicsBodyID ) );
                pDynamicPhysicsComponent->SetWorldTransformDirectly( bodyTransform, false );
            }

            m_pPhysicsWorld->UnlockRead();
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void PhysicsWorldSystem::DebugDraw( EntityWorldUpdateContext const& ctx )
    {
        for ( auto pViewport : ctx.GetViewports() )
        {
            m_pPhysicsWorld->DrawDebug( pViewport );
        }
    }

    bool PhysicsWorldSystem::IsRecording() const
    {
        return m_pPhysicsWorld->IsRecording();
    }

    void PhysicsWorldSystem::StartRecording()
    {
        m_pPhysicsWorld->StartRecording();
    }

    void PhysicsWorldSystem::StopRecording()
    {
        m_pPhysicsWorld->StopRecording();
    }
    #endif
}
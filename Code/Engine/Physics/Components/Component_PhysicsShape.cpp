#include "Component_PhysicsShape.h"
#include "Engine/Physics/PhysX.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    TEvent<PhysicsShapeComponent*> PhysicsShapeComponent::s_staticActorTransformChanged;

    //-------------------------------------------------------------------------

    void PhysicsShapeComponent::OnWorldTransformUpdated()
    {
        if ( m_pPhysicsActor != nullptr && IsKinematic() )
        {
            // Request the kinematic body be moved by the physics simulation
            auto physicsScene = m_pPhysicsActor->getScene();
            physicsScene->lockWrite();
            auto pKinematicActor = m_pPhysicsActor->is<physx::PxRigidDynamic>();
            EE_ASSERT( pKinematicActor->getRigidBodyFlags().isSet( physx::PxRigidBodyFlag::eKINEMATIC ) );
            pKinematicActor->setKinematicTarget( ToPx( GetWorldTransform() ) );
            physicsScene->unlockWrite();
        }
        // Notify listeners that our transform has changed!
        else if ( m_actorType == ActorType::Static )
        {
            s_staticActorTransformChanged.Execute( this );
        }
    }

    void PhysicsShapeComponent::TeleportTo( Transform const& newWorldTransform )
    {
        EE_ASSERT( m_pPhysicsActor != nullptr && IsKinematic() );
        SetWorldTransformDirectly( newWorldTransform, false ); // Do not fire callback as we dont want to lock the scene twice

        // Teleport kinematic body
        auto physicsScene = m_pPhysicsActor->getScene();
        physicsScene->lockWrite();
        auto pKinematicActor = m_pPhysicsActor->is<physx::PxRigidDynamic>();
        EE_ASSERT( pKinematicActor->getRigidBodyFlags().isSet( physx::PxRigidBodyFlag::eKINEMATIC ) );
        pKinematicActor->setGlobalPose( ToPx( GetWorldTransform() ) );
        physicsScene->unlockWrite();
    }

    void PhysicsShapeComponent::MoveTo( Transform const& newWorldTransform )
    {
        EE_ASSERT( m_pPhysicsActor != nullptr && IsKinematic() );
        SetWorldTransform( newWorldTransform ); // The callback will update the kinematic target
    }

    void PhysicsShapeComponent::SetVelocity( Float3 newVelocity )
    {
        EE_ASSERT( m_pPhysicsActor != nullptr && IsDynamic() );
        EE_UNIMPLEMENTED_FUNCTION();
    }
}
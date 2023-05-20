#include "Component_PhysicsShape.h"
#include "Engine/Physics/Physics.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    TEvent<PhysicsShapeComponent*> PhysicsShapeComponent::s_rebuildBodyRequest;

    //-------------------------------------------------------------------------

    void PhysicsShapeComponent::OnWorldTransformUpdated()
    {
        Transform const& worldTransform = GetWorldTransform();

        if ( IsActorCreated() )
        {
            MoveTo( worldTransform );
        }
    }

    void PhysicsShapeComponent::SetCollisionSettings( CollisionSettings const& newSettings )
    {
        m_collisionSettings = newSettings;

        if ( IsActorCreated() )
        {
            s_rebuildBodyRequest.Execute( this );
        }
    }

    void PhysicsShapeComponent::TeleportTo( Transform const& newWorldTransform )
    {
        EE_ASSERT( IsActorCreated() );
        SetWorldTransformDirectly( newWorldTransform, false ); // Do not fire callback
        Transform const physicsActorTransform = ConvertTransformToPhysics( GetWorldTransform() );

        //-------------------------------------------------------------------------

        auto pPhysicsScene = m_pPhysicsActor->getScene();

        if ( IsStatic() && IsGameScene( pPhysicsScene ) )
        {
            EE_LOG_WARNING( "Physics", "Shape Component", "Teleporting a static shape, this is not recommended!" );
        }

        pPhysicsScene->lockWrite();
        m_pPhysicsActor->setGlobalPose( ToPx( physicsActorTransform ) );
        pPhysicsScene->unlockWrite();
    }

    void PhysicsShapeComponent::MoveTo( Transform const& newWorldTransform )
    {
        EE_ASSERT( IsActorCreated() );
        SetWorldTransformDirectly( newWorldTransform, false ); // Do not fire callback
        Transform const physicsActorTransform = ConvertTransformToPhysics( GetWorldTransform() );

        //-------------------------------------------------------------------------

        auto pPhysicsScene = m_pPhysicsActor->getScene();

        if ( IsKinematic() )
        {
            auto pKinematicActor = m_pPhysicsActor->is<physx::PxRigidDynamic>();
            EE_ASSERT( pKinematicActor->getRigidBodyFlags().isSet( physx::PxRigidBodyFlag::eKINEMATIC ) );

            pPhysicsScene->lockWrite();
            pKinematicActor->setKinematicTarget( ToPx( physicsActorTransform ) );
            pPhysicsScene->unlockWrite();
        }
        else
        {
            if ( IsStatic() && IsGameScene( pPhysicsScene ) )
            {
                EE_LOG_WARNING( "Physics", "Shape Component", "Moving a static shape, this is not recommended!" );
            }

            pPhysicsScene->lockWrite();
            m_pPhysicsActor->setGlobalPose( ToPx( physicsActorTransform ) );
            pPhysicsScene->unlockWrite();
        }
    }

    void PhysicsShapeComponent::SetVelocity( Float3 newVelocity )
    {
        EE_ASSERT( IsActorCreated() && IsDynamic() );
        EE_UNIMPLEMENTED_FUNCTION();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PhysicsShapeComponent::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        SpatialEntityComponent::PostPropertyEdit( pPropertyEdited );
        s_rebuildBodyRequest.Execute( this );
    }
    #endif
}
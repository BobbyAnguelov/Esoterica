#include "Component_PhysicsShape.h"
#include "Engine/Physics/PhysicsWorld.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    void PhysicsShapeComponent::OnWorldTransformUpdated()
    {
        Transform const& worldTransform = GetWorldTransform();

        if ( IsPhysicsBodyCreated() )
        {
            MoveTo( worldTransform );
        }
    }

    void PhysicsShapeComponent::SetCollisionSettings( CollisionSettings const& newSettings )
    {
        m_collisionSettings = newSettings;

        if ( IsPhysicsBodyCreated() )
        {
            GetRebuildBodySignal()->Send( this );
        }
    }

    bool PhysicsShapeComponent::CreatePhysicsBody()
    {
        EE_ASSERT( m_pPhysicsWorld != nullptr );
        EE_ASSERT( HasValidPhysicsSetup() );

        Transform const& worldTransform = GetWorldTransform();

        // User data
        m_userData.m_entityID = GetEntityID();
        m_userData.m_componentID = GetID();

        #if EE_DEVELOPMENT_TOOLS
        m_userData.m_name = GetNameID().c_str();
        #endif

        // Body
        //-------------------------------------------------------------------------

        b3BodyDef bodyDef = b3DefaultBodyDef();
        bodyDef.position = ToBox3D( worldTransform.GetTranslation() );
        bodyDef.rotation = ToBox3D( worldTransform.GetRotation() );
        bodyDef.gravityScale = m_defaultGravityScale;
        bodyDef.userData = &m_userData;

        #if EE_DEVELOPMENT_TOOLS
        bodyDef.name = GetNameID().c_str();
        #endif

        switch ( m_mobility )
        {
            case Mobility::Static:
            {
                bodyDef.type = b3_staticBody;
            }
            break;

            case Mobility::Dynamic:
            {
                bodyDef.type = b3_dynamicBody;

                // Disable dynamic actors by default in non game worlds
                if ( !m_pPhysicsWorld->IsGameWorld() )
                {
                    bodyDef.isEnabled = false;
                }
            }
            break;

            case Mobility::Kinematic:
            {
                bodyDef.type = b3_kinematicBody;
            }
            break;
        }

        //-------------------------------------------------------------------------

        m_pPhysicsWorld->LockWrite();
        m_physicsBodyID = b3CreateBody( m_pPhysicsWorld->GetWorldID(), &bodyDef );
        CreatePhysicsShape();
        m_pPhysicsWorld->UnlockWrite();

        if ( B3_IS_NULL( m_physicsShapeID ) )
        {
            DestroyPhysicsBody();
        }

        m_kinematicTargetTransform = GetWorldTransform();

        return IsPhysicsBodyCreated();
    }

    void PhysicsShapeComponent::DestroyPhysicsBody()
    {
        if ( B3_IS_NON_NULL( m_physicsBodyID ) )
        {
            m_pPhysicsWorld->LockWrite();
            b3DestroyBody( m_physicsBodyID );
            m_pPhysicsWorld->UnlockWrite();
        }

        m_physicsShapeID = {};
        m_physicsBodyID = {};
    }

    void PhysicsShapeComponent::TeleportTo( Transform const& newWorldTransform )
    {
        EE_ASSERT( IsPhysicsBodyCreated() );
        SetWorldTransformDirectly( newWorldTransform, false ); // Do not fire callback

        //-------------------------------------------------------------------------

        if ( IsStatic() && m_pPhysicsWorld->IsGameWorld() )
        {
            EE_LOG_WARNING( LogCategory::Physics, "Shape Component", "Teleporting a static shape, this is not recommended!" );
        }

        if ( IsKinematic() )
        {
            m_kinematicTargetTransform = newWorldTransform;
        }

        Transform const& worldTransform = GetWorldTransform();

        m_pPhysicsWorld->LockWrite();
        b3Body_SetTransform( m_physicsBodyID, ToBox3D( worldTransform.GetTranslation() ), ToBox3D( worldTransform.GetRotation() ) );
        m_pPhysicsWorld->UnlockWrite();
    }

    void PhysicsShapeComponent::MoveTo( Transform const& newWorldTransform )
    {
        EE_ASSERT( IsPhysicsBodyCreated() );
        SetWorldTransformDirectly( newWorldTransform, false ); // Do not fire callback
        Transform const& worldTransform = GetWorldTransform();

        //-------------------------------------------------------------------------

        if ( IsKinematic() )
        {
            m_kinematicTargetTransform = worldTransform;
        }
        else
        {
            if ( IsStatic() && m_pPhysicsWorld->IsGameWorld() )
            {
                EE_LOG_WARNING( LogCategory::Physics, "Shape Component", "Moving a static shape, this is not recommended!" );
            }

            m_pPhysicsWorld->LockWrite();
            b3Body_SetTransform( m_physicsBodyID, ToBox3D( worldTransform.GetTranslation() ), ToBox3D( worldTransform.GetRotation() ) );
            m_pPhysicsWorld->UnlockWrite();
        }
    }

    void PhysicsShapeComponent::SetVelocity( Float3 newVelocity )
    {
        EE_ASSERT( IsPhysicsBodyCreated() && IsDynamic() );
        EE_UNIMPLEMENTED_FUNCTION();
    }
}
#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Base class for all physics shape components
    //-------------------------------------------------------------------------
    // Each physics component will create a new body in the physics world
    // TODO: provide option to weld the shape from the component's body to it first parent body

    class EE_ENGINE_API PhysicsShapeComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( PhysicsShapeComponent );

        friend class PhysicsWorldSystem;

    public:

        inline PhysicsShapeComponent() = default;
        inline PhysicsShapeComponent( StringID name ) : SpatialEntityComponent( name ) {}

        // Did we create a physics body for this shape?
        inline bool IsPhysicsBodyCreated() const { return !B3_IS_NULL( m_physicsBodyID ); }

        // Collision Settings
        //-------------------------------------------------------------------------

        // Update the component's current collision settings
        virtual CollisionSettings const& GetCollisionSettings() const { return m_collisionSettings; }

        // Update the component's collision settings
        virtual void SetCollisionSettings( CollisionSettings const& newSettings );

        // Simulation Settings
        //-------------------------------------------------------------------------

        // Get the mobility for this shape
        Mobility GetMobility() const { return m_mobility; }

        // Get the mobility for this shape
        float GetGravityScale() const { return m_defaultGravityScale; }

        // Are we a static body
        EE_FORCE_INLINE bool IsStatic() const { return m_mobility == Mobility::Static; }

        // Are we a dynamic (simulated) body
        EE_FORCE_INLINE bool IsDynamic() const { return m_mobility == Mobility::Dynamic; }

        // Are we a kinematic ( key-framed/user positioned ) body
        EE_FORCE_INLINE bool IsKinematic() const { return m_mobility == Mobility::Kinematic; }

        // Spatial API
        //-------------------------------------------------------------------------

        // Moves the body in the physics world
        // For kinematic bodies: This will set the immediately set component's world transform and request that the kinematic actor be moved to the desired location, correctly interacting with other actors on it's path.
        //                       The actual physics body will only be moved during the next physics simulation step
        // For static/dynamic bodies : this is the same as requesting a teleport!
        void MoveTo( Transform const& newWorldTransform );

        // Teleport the body to the requested transform
        void TeleportTo( Transform const& newWorldTransform );

        // Set the body's velocity
        // Note: This is only valid to call for created dynamic bodies!!!
        void SetVelocity( Float3 newVelocity );

        // Signals
        //-------------------------------------------------------------------------

        inline TEntityWorldSystemSignal<PhysicsShapeComponent>* GetRebuildBodySignal() { return &m_rebuildBodySignal; }

    protected:

        // Check if the physics setup if valid for this component, will log any problems detected!
        virtual bool HasValidPhysicsSetup() const = 0;

        bool CreatePhysicsBody();
        void DestroyPhysicsBody();

        // Create the specific shape for the component, will only be called if we have a valid body
        virtual void CreatePhysicsShape() = 0;

        // Update physics world position for this shape
        virtual void OnWorldTransformUpdated() override final;

    protected:

        EE_REFLECT( Category = "Collision" )
        CollisionSettings                               m_collisionSettings;

        // The mobility of the object
        EE_REFLECT( Category = "Body" );
        Mobility                                        m_mobility = Mobility::Static;

        // Dynamic Only: gravity scale
        EE_REFLECT( Category = "Body" );
        float                                           m_defaultGravityScale = 1.0f;

        // The density of the object. Defaulted to the density of water
        EE_REFLECT( Category = "Shape" );
        float                                           m_defaultDensity = Constants::BaseDensity;

        //-------------------------------------------------------------------------

        PhysicsWorld*                                   m_pPhysicsWorld = nullptr;
        b3BodyId                                        m_physicsBodyID = {};
        b3ShapeId                                       m_physicsShapeID = {};
        UserData                                        m_userData;

        TEntityWorldSystemSignal<PhysicsShapeComponent> m_rebuildBodySignal;

        Transform                                       m_kinematicTargetTransform;
    };
}
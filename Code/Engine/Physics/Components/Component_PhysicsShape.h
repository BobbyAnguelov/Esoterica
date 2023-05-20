#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Physics/PhysicsSettings.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace physx
{
    class PxRigidActor;
    class PxShape;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Base class for all physics shape components
    //-------------------------------------------------------------------------
    // Each physics component will create a new body in the physics world
    // TODO: provide option to simple weld the shape from the component to its parent actor

    class EE_ENGINE_API PhysicsShapeComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( PhysicsShapeComponent );

        friend class PhysicsWorld;
        friend class PhysicsWorldSystem;

        // Event fired whenever we require the body to be recreated
        static TEvent<PhysicsShapeComponent*> s_rebuildBodyRequest;

    public:

        // Args: Component
        inline static TEventHandle<PhysicsShapeComponent*> OnRebuildBodyRequested() { return s_rebuildBodyRequest; }

    public:

        inline PhysicsShapeComponent() = default;
        inline PhysicsShapeComponent( StringID name ) : SpatialEntityComponent( name ) {}

        // Did we create a physics body for this shape?
        inline bool IsActorCreated() const { return m_pPhysicsActor != nullptr; }

        // Collision Settings
        //-------------------------------------------------------------------------

        // Update the component's current collision settings
        virtual CollisionSettings const& GetCollisionSettings() const { return m_collisionSettings; }

        // Update the component's collision settings
        virtual void SetCollisionSettings( CollisionSettings const& newSettings );

        // Simulation Settings
        //-------------------------------------------------------------------------

        // Update the component's current simulation settings
        inline SimulationSettings const& GetSimulationSettings() const { return m_simulationSettings; }

        // Are we a static body
        EE_FORCE_INLINE bool IsStatic() const { return m_simulationSettings.IsStatic(); }

        // Are we a dynamic (simulated) body
        EE_FORCE_INLINE bool IsDynamic() const { return m_simulationSettings.IsDynamic(); }

        // Are we a kinematic ( key-framed/user positioned ) body
        EE_FORCE_INLINE bool IsKinematic() const { return m_simulationSettings.IsKinematic(); }

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

    protected:

        // Check if the physics setup if valid for this component, will log any problems detected!
        virtual bool HasValidPhysicsSetup() const = 0;

        // Update physics world position for this shape
        virtual void OnWorldTransformUpdated() override final;

        // Some shapes require a conversion between esoterica and physx (notably capsules)
        virtual Transform ConvertTransformToPhysics( Transform const& esotericaTransform ) const { return esotericaTransform; }

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

    protected:

        EE_REFLECT( "Category" : "Simulation" )
        SimulationSettings                              m_simulationSettings;

        EE_REFLECT( "Category" : "Collision" )
        CollisionSettings                               m_collisionSettings;

    private:

        physx::PxRigidActor*                            m_pPhysicsActor = nullptr;
        physx::PxShape*                                 m_pPhysicsShape = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        String                                          m_debugName; // Keep a debug name here since the physx SDK doesnt store the name data
        #endif
    };
}
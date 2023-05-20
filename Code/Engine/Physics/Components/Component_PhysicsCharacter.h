#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Physics/PhysicsSettings.h"
#include "System/Time/Time.h"
#include "System/Time/Timers.h"
#include "System/Types/Color.h"

#include <PxQueryFiltering.h>
#include <characterkinematic/PxController.h>
#include <characterkinematic/PxControllerBehavior.h>

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

namespace physx
{
    class PxCapsuleController;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    namespace PX { class ControllerCallbackHandler; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CharacterComponent : public SpatialEntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( CharacterComponent );

        friend class PhysicsWorld;
        friend PX::ControllerCallbackHandler;

    public:

        enum class FloorType : uint8_t
        {
            Navigable,
            Unnavigable,
            NoFloor,
        };

    public:

        // Movement
        //-------------------------------------------------------------------------

        // This function will try to move a character with the desired rotation and translation
        // Returns the actual delta movement that was performed
        Vector MoveCharacter( Seconds deltaTime, Quaternion const& deltaRotation, Vector const& deltaTranslation );

        // This will set the component's world transform and teleport the physics actor to the desired location.
        // Note: This will reset the character velocity
        // Note: This is the same as directly moving the component
        void TeleportCharacter( Transform const& newWorldTransform ) { SetWorldTransform( newWorldTransform ); }

        // Character Info
        //-------------------------------------------------------------------------

        // The character total height
        EE_FORCE_INLINE float GetCharacterHeight() const { return ( m_radius + m_halfHeight ) * 2; }

        // Get the current linear velocity (m/s)
        EE_FORCE_INLINE Vector const& GetCharacterVelocity() const { return m_linearVelocity; }

        // Clear character velocity
        EE_FORCE_INLINE void ClearCharacterVelocity() { m_linearVelocity = Vector::Zero; }

        // How long have we been without a floor?
        float GetInAirTime() const { return m_timeWithoutFloor.GetElapsedTimeSeconds(); }

        // Do we currently have a floor?
        bool HasFloor() const { return m_floorType != FloorType::NoFloor; }

        // What sort of floor do we have?
        FloorType GetFloorType() const { return m_floorType; }

        // Capsule
        //-------------------------------------------------------------------------
 
        // The character capsule radius
        EE_FORCE_INLINE float GetCapsuleRadius() const { return m_radius; }

        // Get the half-height of the character capsule - this is the cylinder portion
        EE_FORCE_INLINE float GetCapsuleHalfHeight() const { return m_halfHeight; }

        // Get the world space top point of the capsule
        EE_FORCE_INLINE Vector GetCapsuleTop() const { return GetWorldTransform().GetTranslation() + Vector( 0, 0, m_radius + m_halfHeight ); }

        // Get the world space bottom point of the capsule
        EE_FORCE_INLINE Vector GetCapsuleBottom() const { return GetWorldTransform().GetTranslation() - Vector( 0, 0, m_radius + m_halfHeight ); }

        // Resize the capsule with the new dimensions (radius and half-height must be greater than zero)
        // By default, the resized capsule will keep its floor/foot position, if you want to leave the transform untouched, set 'keepFloorPosition' to false
        void ResizeCapsule( float newRadius, float newHalfHeight, bool keepFloorPosition = true );

        // Resets the capsule size to the original defaults
        // By default, the resized capsule will keep its floor/foot position, if you want to leave the transform untouched, set 'keepFloorPosition' to false
        EE_FORCE_INLINE void ResetCapsuleSize( bool keepFloorPosition = true ) { ResizeCapsule( m_defaultRadius, m_defaultHalfHeight, keepFloorPosition ); }

        // Controller Settings
        //-------------------------------------------------------------------------

        // Did we create a physics controller for this character?
        inline bool IsControllerCreated() const { return m_pController != nullptr; }

        // Set the step height - the height at which we can automatically step onto/over objects - ( must be >= 0.0f )
        void SetStepHeight( float stepHeight );

        // Reset the step height to the default value
        EE_FORCE_INLINE void ResetStepHeight() { SetStepHeight( m_defaultStepHeight ); }

        // Set the slope limit - the limit for navigable slopes - ( must be >= 0.0f degrees )
        void SetSlopeLimit( Degrees slopeLimit );

        // Reset the slope limit to the default value
        EE_FORCE_INLINE void ResetSlopeLimit() { SetSlopeLimit( m_defaultSlopeLimit ); }

        // Enable gravity
        void SetGravityEnabled( bool isGravityEnabled, float initialGravitationalSpeed = 0 );

        // Set the gravitational acceleration (the m/s^2 acceleration along the world gravity direction)
        EE_FORCE_INLINE void SetGravitationalAcceleration( float acceleration ) { m_gravitationalAcceleration = acceleration; }

        // Reset the gravitational acceleration
        EE_FORCE_INLINE void ResetGravitationlAcceleration() { SetGravitationalAcceleration( m_defaultGravitationalAcceleration ); }

        // Reset the current vertical speed
        EE_FORCE_INLINE void ResetVerticalSpeed() { m_verticalSpeed = 0.f; }

        // Ignore Rules
        //-------------------------------------------------------------------------

        inline TInlineVector<EntityID, 5> const& GetIgnoredEntities() const { return m_ignoredEntities; }

        inline bool IsEntityIgnored( EntityID ID ) const { return VectorContains( m_ignoredEntities, ID ); }

        inline void AddIgnoredEntity( EntityID const& ID ) { m_ignoredEntities.emplace_back( ID ); }

        inline void RemoveIgnoredEntity( EntityID const& ID ) { m_ignoredEntities.erase_first_unsorted( ID ); }

        inline void ClearIgnoredEntities() { m_ignoredEntities.clear(); }

        //-------------------------------------------------------------------------

        inline TInlineVector<ComponentID, 5> const& GetIgnoredComponents() const { return m_ignoredComponents; }

        inline bool IsComponentIgnored( ComponentID componentID ) const { return VectorContains( m_ignoredComponents, componentID ); }

        inline void AddIgnoredComponent( ComponentID componentID ) { m_ignoredComponents.emplace_back( componentID ); }

        inline void RemoveIgnoredComponent( ComponentID componentID ) { m_ignoredComponents.erase_first_unsorted( componentID ); }

        inline void ClearIgnoredComponents() { m_ignoredComponents.clear(); }

        // Physics
        //-------------------------------------------------------------------------

        // Is the physics setup correct for this component
        bool HasValidCharacterSetup() const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Enable a debug-only ghost/noclip mode
        EE_FORCE_INLINE void EnableGhostMode( bool isEnabled ) { m_isGhostModeEnabled = isEnabled; }

        // Draw an imgui debug UI window
        void DrawDebugUI();

        // Draw the debug visualization
        void DrawDebug( Drawing::DrawContext& ctx ) const;
        #endif

    protected:

        virtual OBB CalculateLocalBounds() const override final;

        virtual void Initialize() override;
        virtual void Shutdown() override;

    private:

        // Override this function to change access specifier
        // Use the MoveCharacter or TeleportCharacter functions instead!
        EE_FORCE_INLINE void SetWorldTransform( Transform const& newTransform )
        {
            SpatialEntityComponent::SetWorldTransform( newTransform );
        }

        // Update physics world position for this shape
        virtual void OnWorldTransformUpdated() override final;

    protected:

        // The radius of the capsule's end caps
        EE_REFLECT( "Category" : "Shape" );
        float                                   m_defaultRadius = 0.5f;

        // The half-height of the cylinder portion of the capsule (between the end caps)
        EE_REFLECT( "Category" : "Shape" );
        float                                   m_defaultHalfHeight = 1.0f;

        EE_REFLECT( "Category" : "Collision" )
        CollisionSettings                       m_collisionSettings;

        // The height at which we will automatically step over objects
        EE_REFLECT( "Category" : "Controller" )
        float                                   m_defaultStepHeight = 0.2f;

        // The maximum slope we can walk on
        EE_REFLECT( "Category" : "Controller" )
        Degrees                                 m_defaultSlopeLimit = 45.f;

        // The default gravitational acceleration we apply along the gravity vector
        EE_REFLECT( "Category" : "Controller" )
        float                                   m_defaultGravitationalAcceleration = 30.f;

    private:

        float                                   m_radius = m_defaultRadius;
        float                                   m_halfHeight = m_defaultHalfHeight;
        float                                   m_stepHeight = m_defaultStepHeight;
        Degrees                                 m_slopeLimit = m_defaultSlopeLimit;
        float                                   m_gravitationalAcceleration = m_defaultGravitationalAcceleration;

        physx::PxCapsuleController*             m_pController = nullptr;
        PX::ControllerCallbackHandler*          m_pCallbackHandler = nullptr;

        ManualTimer                             m_timeWithoutFloor;
        float                                   m_verticalSpeed = 0.0f; // The speed of the character along the up vector
        Vector                                  m_floorNormal = Vector::Zero;
        Vector                                  m_linearVelocity = Vector::Zero;
        bool                                    m_isGravityEnabled = true;
        FloorType                               m_floorType = FloorType::NoFloor;

        TInlineVector<EntityID, 5>              m_ignoredEntities;
        TInlineVector<ComponentID, 5>           m_ignoredComponents;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        String                                  m_debugName; // Keep a debug name here since the physx SDK doesnt store the name data
        bool                                    m_isGhostModeEnabled = false;
        bool                                    m_debugCapsule = false;
        bool                                    m_debugFloor = false;
        Transform                               m_debugPreMoveTransform;
        Transform                               m_debugPostMoveTransform;
        #endif
    };

    //-------------------------------------------------------------------------

    namespace PX
    {
        class ControllerCallbackHandler final : public physx::PxQueryFilterCallback, public physx::PxUserControllerHitReport, public physx::PxControllerBehaviorCallback
        {
        public:

            ControllerCallbackHandler( CharacterComponent* pComponent );

        private:

            virtual physx::PxQueryHitType::Enum preFilter( physx::PxFilterData const& queryFilterData, physx::PxShape const* pShape, physx::PxRigidActor const* pActor, physx::PxHitFlags& queryFlags ) override;
            virtual physx::PxQueryHitType::Enum postFilter( physx::PxFilterData const& queryFilterData, physx::PxQueryHit const& hit ) override;
            virtual void onShapeHit( physx::PxControllerShapeHit const& hit ) override;
            virtual void onControllerHit( physx::PxControllersHit const& hit ) override;
            virtual void onObstacleHit( physx::PxControllerObstacleHit const& hit ) override;
            virtual physx::PxControllerBehaviorFlags getBehaviorFlags( physx::PxShape const& shape, physx::PxActor const& actor ) override;
            virtual physx::PxControllerBehaviorFlags getBehaviorFlags( physx::PxController const& controller ) override;
            virtual physx::PxControllerBehaviorFlags getBehaviorFlags( physx::PxObstacle const& obstacle ) override;

        private:

            CharacterComponent* m_pComponent = nullptr;
        };
    }
}
#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Physics/PhysicsSettings.h"
#include "Engine/Physics/PhysicsQuery.h"
#include "System/Time/Time.h"
#include "System/Time/Timers.h"
#include "System/Types/Color.h"

#include <characterkinematic/PxController.h>
#include <characterkinematic/PxControllerBehavior.h>

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

namespace physx
{
    class PxCapsuleController;
    class PxShape;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CharacterComponent;

    //-------------------------------------------------------------------------

    namespace PX
    {
        class ControllerCallbackHandler final : public physx::PxQueryFilterCallback, public physx::PxUserControllerHitReport, public physx::PxControllerBehaviorCallback
        {
            // Ideally this would be part of the character controller but they are not DLL exposed classes so this causes a warning
            // Hence we have the bidirectional friendship
            friend class CharacterComponent;

            struct FloorCollision
            {
                FloorCollision( physx::PxShape* pShape, physx::PxVec3 const& normal, physx::PxExtendedVec3 const& point )
                    : m_pShape( pShape) 
                    , m_normal( normal )
                    , m_point( point )
                {}

                physx::PxShape* m_pShape;
                physx::PxVec3 m_normal;
                physx::PxExtendedVec3 m_point;
            };

        private:

            ControllerCallbackHandler( CharacterComponent* pComponent );

            void Reset();

            virtual physx::PxQueryHitType::Enum preFilter( physx::PxFilterData const& queryFilterData, physx::PxShape const* pShape, physx::PxRigidActor const* pActor, physx::PxHitFlags& queryFlags ) override;
            virtual physx::PxQueryHitType::Enum postFilter( physx::PxFilterData const& queryFilterData, physx::PxQueryHit const& hit ) override;
            virtual void onShapeHit( physx::PxControllerShapeHit const& hit ) override;
            virtual void onControllerHit( physx::PxControllersHit const& hit ) override;
            virtual void onObstacleHit( physx::PxControllerObstacleHit const& hit ) override;
            virtual physx::PxControllerBehaviorFlags getBehaviorFlags( physx::PxShape const& shape, physx::PxActor const& actor ) override;
            virtual physx::PxControllerBehaviorFlags getBehaviorFlags( physx::PxController const& controller ) override;
            virtual physx::PxControllerBehaviorFlags getBehaviorFlags( physx::PxObstacle const& obstacle ) override;

        private:

            CharacterComponent*         m_pComponent = nullptr;
            TVector<FloorCollision>     m_floorCollisions;
        };
    }

    //-------------------------------------------------------------------------

    enum class ControllerFloorType : uint8_t
    {
        Floor, // Valid navigable floor
        Slope, // Unnavigable slope
        NoFloor,
    };

    enum class ControllerGravityMode : uint8_t
    {
        EE_REFLECT_ENUM

        NoGravity, // No vertical adjustment
        Acceleration, // Acceleration based
        FixedVelocity // Fixed speed gravity
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CharacterComponent : public SpatialEntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( CharacterComponent );

        friend class PhysicsWorld;
        friend PX::ControllerCallbackHandler;

        constexpr static float const s_maxGravitationalSpeed = 75.0f; // m/s

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
        EE_FORCE_INLINE float GetCharacterHeight() const { return ( m_halfHeight + m_radius ) * 2; }

        // Get the current linear velocity (m/s)
        EE_FORCE_INLINE Vector const& GetCharacterVelocity() const { return m_linearVelocity; }

        // Clear character velocity
        EE_FORCE_INLINE void ClearCharacterVelocity() { m_linearVelocity = Vector::Zero; }

        // How long have we been without a floor?
        EE_FORCE_INLINE Seconds GetInAirTime() const { return m_timeWithoutFloor.GetElapsedTimeSeconds(); }

        // Do we currently have a floor?
        bool HasFloor() const { return m_floorType != ControllerFloorType::NoFloor; }

        // What sort of floor do we have?
        ControllerFloorType GetFloorType() const { return m_floorType; }

        // Get the query rules for this character if you want to do additional queries
        QueryRules const& GetQueryRules() const { return m_queryRules; }

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

        // Resize the capsule with the new dimensions (radius > 0 and half-height >= 0)
        // By default, the resized capsule will keep its floor/foot position, if you want to leave the transform untouched, set 'keepFloorPosition' to false
        void ResizeCapsule( float newRadius, float newHalfHeight, bool keepFloorPosition = true );

        // Resize only the capsule's height with the new dimensions (half-height >= 0)
        // By default, the resized capsule will keep its floor/foot position, if you want to leave the transform untouched, set 'keepFloorPosition' to false
        EE_FORCE_INLINE void ResizeCapsuleHeight( float newHalfHeight, bool keepFloorPosition = true ) { ResizeCapsule( m_radius, newHalfHeight, keepFloorPosition ); }

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

        // Set the gravity mode - Only applicable to acceleration and no gravity
        // Do not call this function for fixed velocity!
        void SetGravityMode( ControllerGravityMode mode );

        // Set the gravity mode - the value is either an acceleration or fixed speed along the world's gravity direction
        void SetGravityMode( ControllerGravityMode mode, float gravityValue );

        // Set the gravity mode back to the default
        void ResetGravityMode();

        // Keep the characters vertical momentum by setting the gravitational speed to its current vertical velocity
        EE_FORCE_INLINE void TryMaintainVerticalMomentum() { m_gravitationalSpeed = -m_linearVelocity.GetZ(); }

        // Set the gravitational speed
        // Warning: this is a speed along the gravity direction!!
        // Note: this will be overwritten if you are in fixed gravity mode
        EE_FORCE_INLINE void SetGravitationalSpeed( float newSpeed ) { m_gravitationalSpeed = newSpeed; }

        // Reset the current gravitational speed
        EE_FORCE_INLINE void ResetGravitationalSpeed() { m_gravitationalSpeed = 0.f; }

        // Ignore Rules
        //-------------------------------------------------------------------------

        inline TInlineVector<EntityID, 5> const& GetIgnoredEntities() const { return m_queryRules.GetIgnoredEntities(); }

        inline bool IsEntityIgnored( EntityID ID ) const { return m_queryRules.IsEntityIgnored( ID ); }

        inline void AddIgnoredEntity( EntityID const& ID ) { m_queryRules.AddIgnoredEntity( ID ); }

        inline void RemoveIgnoredEntity( EntityID const& ID ) { m_queryRules.RemoveIgnoredEntity( ID ); }

        inline void ClearIgnoredEntities() { m_queryRules.ClearIgnoredEntities(); }

        //-------------------------------------------------------------------------

        inline TInlineVector<ComponentID, 5> const& GetIgnoredComponents() const { return m_queryRules.GetIgnoredComponents(); }

        inline bool IsComponentIgnored( ComponentID ID ) const { return m_queryRules.IsComponentIgnored( ID ); }

        inline void AddIgnoredComponent( ComponentID ID ) { m_queryRules.AddIgnoredComponent( ID ); }

        inline void RemoveIgnoredComponent( ComponentID ID ) { m_queryRules.RemoveIgnoredComponent( ID ); }

        inline void ClearIgnoredComponents() { m_queryRules.ClearIgnoredComponents(); }

        // Physics
        //-------------------------------------------------------------------------

        // Is the physics setup correct for this component
        bool HasValidCharacterSetup() const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Enable a debug-only ghost/noclip mode
        void EnableGhostMode( bool isEnabled );

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

        // The half-height of the cylinder portion of the capsule
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

        // The default gravity acceleration value, all character controllers are by default in acceleration mode
        EE_REFLECT( "Category" : "Controller" )
        float                                   m_defaultGravity = 30.f;

    private:

        float                                   m_radius = m_defaultRadius;
        float                                   m_halfHeight = m_defaultHalfHeight;
        float                                   m_stepHeight = m_defaultStepHeight;
        Degrees                                 m_slopeLimit = m_defaultSlopeLimit;
        float                                   m_gravityValue = m_defaultGravity;

        physx::PxCapsuleController*             m_pController = nullptr;
        PX::ControllerCallbackHandler           m_callbackHandler = PX::ControllerCallbackHandler( this );
        Physics::QueryRules                     m_queryRules;

        ManualTimer                             m_timeWithoutFloor;
        float                                   m_gravitationalSpeed = 0.0f; // The additional vertical adjustment we need to perform this frame, positive values are along the gravity direction
        Vector                                  m_floorNormal = Vector::Zero;
        Vector                                  m_floorContactPoint = Vector::Zero;
        Vector                                  m_linearVelocity = Vector::Zero;
        ControllerGravityMode                   m_gravityMode = ControllerGravityMode::Acceleration;
        ControllerFloorType                     m_floorType = ControllerFloorType::NoFloor;

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
}
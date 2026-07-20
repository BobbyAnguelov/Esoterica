#pragma once

#include "Game/_Module/API.h"
#include "Game/Player/PlayerGameState.h"
#include "Base/TypeSystem/TypeInstance.h"
#include "Engine/Physics/Components/Component_PhysicsCapsule.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------
// Character Component
//-------------------------------------------------------------------------

namespace EE
{
    // Todo: move somewhere shared
    enum class FloorType : uint8_t
    {
        Floor, // Valid navigable floor
        Slope, // Unnavigable slope
        None,
    };

    //-------------------------------------------------------------------------

    class EE_GAME_API CharacterComponent : public Physics::CapsuleComponent
    {
        EE_ENTITY_COMPONENT( CharacterComponent );

        friend class CharacterDebugView;
        friend bool MoverFilterFcn( b3ShapeId shapeId, void* context );

        constexpr static float const s_maxGravitationalSpeed = 75.0f; // m/s
        constexpr static float const s_groundProbeLength = 15.0f; // m

    public:

        CharacterComponent();
        CharacterComponent( StringID name );

        //-------------------------------------------------------------------------
        // Character Controller
        //-------------------------------------------------------------------------

        // Get the full height of the capsule (including radius)
        EE_FORCE_INLINE float GetCharacterHeight() const { return ( m_defaultHalfHeight + m_defaultRadius ) * 2; }

        // Get the half-height of the capsule (including radius)
        EE_FORCE_INLINE float GetCharacterHalfHeight() const { return ( m_defaultHalfHeight + m_defaultRadius ); }

        // Get the half-height of the capsule (including radius)
        EE_FORCE_INLINE Vector GetCharacterBottom() const { return GetCapsuleBottom(); }

        // Get the current linear velocity (m/s)
        EE_FORCE_INLINE Vector const& GetVelocity() const { return m_velocity; }

        // Do we currently have a floor?
        bool HasFloor() const { return m_floorType != FloorType::None; }

        // What sort of floor do we have?
        FloorType GetFloorType() const { return m_floorType; }

        // Get the normal of the floor we are standing on
        inline Vector const& GetFloorNormal() const { EE_ASSERT( HasFloor() ); return m_floorNormal; }

        // Returns the height above the floor up to a certain max distance
        inline float GetHeightAboveGround() const { return m_heightAboveGround; }

        // Are we currently on the ground?
        bool IsOnGround() const { return m_floorType != FloorType::None; }

        // Are we currently in the air/falling?
        bool IsInAir() const { return m_floorType == FloorType::None; }

        // How long have we been without a floor?
        EE_FORCE_INLINE Seconds GetInAirTime() const { return m_timeWithoutFloor.GetElapsedTimeSeconds(); }

        #if EE_DEVELOPMENT_TOOLS
        // Enable a ghost/no-clip mode
        void EnableGhostMode( bool isEnabled );

        // Are we in ghost/no-clip mode?
        inline bool IsGhostModeEnabled() const { return m_isGhostModeEnabled; }
        #endif

        // Set the step height - the height at which we can automatically step onto/over objects - ( must be >= 0.0f )
        void SetStepHeight( float stepHeight );

        // Reset the step height to the default value
        EE_FORCE_INLINE void ResetStepHeight() { SetStepHeight( m_defaultStepHeight ); }

        // Set the slope limit - the limit for navigable slopes - ( must be >= 0.0f degrees )
        void SetSlopeLimit( Degrees slopeLimit );

        // Reset the slope limit to the default value
        EE_FORCE_INLINE void ResetSlopeLimit() { SetSlopeLimit( m_defaultSlopeLimit ); }

        // Set the gravity multiplier for this character
        void SetGravityScale( float gravityScale );

        // Set the gravity multiplier back to the default
        void ResetGravityScale();

        // This is the core character controller function, this will move the character as well as the physics blocker
        void MoveCharacter( Seconds deltaTime, Quaternion const& deltaRotation, Vector const& deltaTranslation );

        // This will teleport the character and reset all internal physics state and characater info like velocity, height above ground, etc...
        void TeleportCharacter( Transform const& targetTransform ); 

        //-------------------------------------------------------------------------
        // Physics Query Rules
        //-------------------------------------------------------------------------

        Physics::QueryRules const& GetQueryRules() const { return m_queryRules; }

        inline TInlineVector<EntityID, 5> const& GetIgnoredEntities() const { return m_queryRules.GetIgnoredEntities(); }

        inline bool IsEntityIgnored( EntityID ID ) const { return m_queryRules.IsEntityIgnored( ID ); }

        inline void AddIgnoredEntity( EntityID const& ID ) { m_queryRules.AddIgnoredEntity( ID ); }

        inline void RemoveIgnoredEntity( EntityID const& ID ) { m_queryRules.RemoveIgnoredEntity( ID ); }

        inline void ClearIgnoredEntities() { m_queryRules.ClearIgnoredEntities(); }

        inline TInlineVector<ComponentID, 5> const& GetIgnoredComponents() const { return m_queryRules.GetIgnoredComponents(); }

        inline bool IsComponentIgnored( ComponentID ID ) const { return m_queryRules.IsComponentIgnored( ID ); }

        inline void AddIgnoredComponent( ComponentID ID ) { m_queryRules.AddIgnoredComponent( ID ); }

        inline void RemoveIgnoredComponent( ComponentID ID ) { m_queryRules.RemoveIgnoredComponent( ID ); }

        inline void ClearIgnoredComponents() { m_queryRules.ClearIgnoredComponents(); }

        //-------------------------------------------------------------------------
        // Gameplay State
        //-------------------------------------------------------------------------

        inline bool HasGameState() const { return m_gameState.IsSet(); }

        GameState* GetGameState() { return m_gameState.Get(); }

        GameState const* GetGameState() const { return m_gameState.Get(); }

        template<typename T>
        T* GetGameState() { return m_gameState.GetAs<T>(); }

        template<typename T>
        T const* GetGameState() const { return m_gameState.GetAs<T>(); }

    protected:

        virtual void Initialize() override;
        virtual void Shutdown() override;

    private:

        // Try to prevent people calling these functions directly, use the character versions above
        void MoveTo( Transform const& newWorldTransform ) { CapsuleComponent::MoveTo( newWorldTransform ); }
        void TeleportTo( Transform const& newWorldTransform ) { CapsuleComponent::TeleportTo( newWorldTransform ); }

    protected:

        EE_REFLECT( Category = "Gameplay" );
        TTypeInstance<GameState>                m_gameState;

        // The height at which we will automatically step over objects
        EE_REFLECT( Category = "Controller" )
        float                                   m_defaultStepHeight = 0.2f;

        // The maximum slope we can walk on
        EE_REFLECT( Category = "Controller" )
        Degrees                                 m_defaultSlopeLimit = 45.f;

        //-------------------------------------------------------------------------

        Physics::QueryRules                     m_queryRules;

        float                                   m_stepHeight = m_defaultStepHeight;
        Degrees                                 m_slopeLimit = m_defaultSlopeLimit;
        float                                   m_gravityScale = m_defaultGravityScale;

        ManualTimer                             m_timeWithoutFloor;
        FloorType                               m_floorType = FloorType::None;
        Vector                                  m_floorNormal = Vector::Zero;
        Vector                                  m_floorContactPoint = Vector::Zero;
        Vector                                  m_velocity = Vector::Zero;
        float                                   m_heightAboveGround = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        bool                                    m_debugCapsule = false;
        bool                                    m_debugFloor = false;
        Transform                               m_debugPreMoveTransform;
        Transform                               m_debugPostMoveTransform;
        bool                                    m_isGhostModeEnabled = false;
        #endif
    };
}
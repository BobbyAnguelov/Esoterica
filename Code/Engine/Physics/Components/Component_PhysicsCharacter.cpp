#include "Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/Physics.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics::PX
{
    ControllerCallbackHandler::ControllerCallbackHandler( CharacterComponent* pComponent )
        : m_pComponent( pComponent )
    {
        EE_ASSERT( m_pComponent != nullptr );
    }

    void ControllerCallbackHandler::Reset()
    {
        m_floorCollisions.clear();
    }

    PxQueryHitType::Enum ControllerCallbackHandler::preFilter( PxFilterData const& queryFilterData, PxShape const* pShape, PxRigidActor const* pActor, PxHitFlags& queryFlags )
    {
        for ( auto const& ignoredComponentID : m_pComponent->m_queryRules.GetIgnoredComponents() )
        {
            auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
            if ( pOwnerComponent->GetID() == ignoredComponentID )
            {
                return PxQueryHitType::eNONE;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& ignoredEntityID : m_pComponent->m_queryRules.GetIgnoredEntities() )
        {
            auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
            if ( pOwnerComponent->GetEntityID() == ignoredEntityID )
            {
                return PxQueryHitType::eNONE;
            }
        }

        //-------------------------------------------------------------------------

        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum ControllerCallbackHandler::postFilter( PxFilterData const& queryFilterData, PxQueryHit const& hit )
    {
        EE_UNREACHABLE_CODE(); // Not currently used
        return PxQueryHitType::eBLOCK;
    }

    void ControllerCallbackHandler::onShapeHit( const PxControllerShapeHit& hit )
    {
        if ( hit.worldNormal.dot( ToPx( Float3::WorldUp ) ) > 0.0f )
        {
            m_floorCollisions.emplace_back( hit.shape, hit.worldNormal, hit.worldPos );
        }
    }

    void ControllerCallbackHandler::onControllerHit( const PxControllersHit& hit )
    {

    }

    void ControllerCallbackHandler::onObstacleHit( const PxControllerObstacleHit& hit )
    {

    }

    PxControllerBehaviorFlags ControllerCallbackHandler::getBehaviorFlags( const PxShape& shape, const PxActor& actor )
    {
        EE_UNREACHABLE_CODE(); // Not currently used - if you need this, enable it in physicsWorld.cpp
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    PxControllerBehaviorFlags ControllerCallbackHandler::getBehaviorFlags( const PxController& controller )
    {
        EE_UNREACHABLE_CODE(); // Not currently used - if you need this, enable it in physicsWorld.cpp
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    PxControllerBehaviorFlags ControllerCallbackHandler::getBehaviorFlags( const PxObstacle& obstacle )
    {
        EE_UNREACHABLE_CODE(); // Not currently used - if you need this, enable it in physicsWorld.cpp
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    OBB CharacterComponent::CalculateLocalBounds() const
    {
        Vector const boundsExtents( m_defaultRadius, m_defaultRadius, m_defaultHalfHeight + m_defaultRadius );
        return OBB( Vector::Zero, boundsExtents );
    }

    void CharacterComponent::Initialize()
    {
        SpatialEntityComponent::Initialize();

        //-------------------------------------------------------------------------

        m_radius = m_defaultRadius;
        m_halfHeight = m_defaultHalfHeight;
        m_stepHeight = m_defaultStepHeight;
        m_slopeLimit = m_defaultSlopeLimit;
        m_gravityMode = ControllerGravityMode::Acceleration;
        m_gravityValue = m_defaultGravity;

        //-------------------------------------------------------------------------

        m_queryRules = QueryRules( m_collisionSettings.m_collidesWithMask );
        m_queryRules.AddIgnoredComponent( GetID() );
    }

    void CharacterComponent::Shutdown()
    {
        SpatialEntityComponent::Shutdown();
    }

    //-------------------------------------------------------------------------

    bool CharacterComponent::HasValidCharacterSetup() const
    {
        if ( m_defaultRadius <= 0 )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius on character component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        if( m_defaultHalfHeight <= 0 )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid half height on character component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }

    void CharacterComponent::OnWorldTransformUpdated()
    {
        m_linearVelocity = Vector::Zero;

        if ( IsControllerCreated() )
        {
            auto physicsScene = m_pController->getScene();
            physicsScene->lockWrite();
            auto result = m_pController->setPosition( ToPxExtended( GetWorldTransform().GetTranslation() ) );
            physicsScene->unlockWrite();
        }
    }

    Vector CharacterComponent::MoveCharacter( Seconds deltaTime, Quaternion const& deltaRotation, Vector const& deltaTranslation )
    {
        EE_ASSERT( deltaTime > 0.0f );
        EE_ASSERT( IsControllerCreated() );
        EE_PROFILE_FUNCTION_GAMEPLAY();

        Transform const startWorldTransform = GetWorldTransform();
        Transform endWorldTransform( deltaRotation * startWorldTransform.GetRotation() );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugCapsule )
        {
            m_debugPreMoveTransform = startWorldTransform;
        }
        #endif

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_isGhostModeEnabled )
        {
            endWorldTransform.SetTranslation( startWorldTransform.GetTranslation() + deltaTranslation );
            TeleportCharacter( endWorldTransform );
            return deltaTranslation;
        }
        #endif

        //-------------------------------------------------------------------------

        Vector desiredDeltaTranslation = deltaTranslation;

        // Gravity
        //-------------------------------------------------------------------------

        if ( m_gravityMode == ControllerGravityMode::NoGravity )
        {
            m_gravitationalSpeed = 0.0f;
        }
        else
        {
            if ( m_gravityMode == ControllerGravityMode::Acceleration )
            {
                m_gravitationalSpeed += ( m_gravityValue * deltaTime );
            }
            else // Fixed gravity
            {
                m_gravitationalSpeed = m_gravityValue;
            }

            m_gravitationalSpeed = Math::Clamp( m_gravitationalSpeed, -s_maxGravitationalSpeed, s_maxGravitationalSpeed );
            Vector const verticalDelta = Physics::Constants::s_gravity * ( m_gravitationalSpeed * deltaTime );
            desiredDeltaTranslation += verticalDelta;
        }

        // Move Controller
        //-------------------------------------------------------------------------

        m_callbackHandler.Reset();
        physx::PxControllerFilters filters( &m_queryRules.GetPxFilterData().data );

        // Note: do not set a minimum distance or we will early out of the move and not get back a floor
        auto physicsScene = m_pController->getScene();
        physicsScene->lockWrite();
        physx::PxControllerCollisionFlags const collisionResultFlags = m_pController->move( ToPx( desiredDeltaTranslation ), 0.0f, deltaTime, filters, nullptr );
        physicsScene->unlockWrite();

        // Process the results of the move
        //-------------------------------------------------------------------------

        physx::PxControllerState controllerState;
        m_pController->getState( controllerState );

        // Collision with a roof
        if ( collisionResultFlags.isSet( physx::PxControllerCollisionFlag::eCOLLISION_UP ) )
        {
            // If we have a upwards adjustment speed, clear it
            if ( m_gravitationalSpeed < 0 )
            {
                m_gravitationalSpeed = 0.0f;
            }
        }

        // Collision with the ground
        if ( collisionResultFlags.isSet( physx::PxControllerCollisionFlag::eCOLLISION_DOWN ) )
        {
            m_timeWithoutFloor.Reset();
            m_gravitationalSpeed = 0.0f;

            //-------------------------------------------------------------------------

            if ( controllerState.touchedShape == nullptr )
            {
                m_floorType = ControllerFloorType::Slope;

                Radians steepestAngle = FLT_MAX;
                for ( auto const& floorCollision : m_callbackHandler.m_floorCollisions )
                {
                    Radians const slopeAngle = Math::GetAngleBetweenNormalizedVectors( Vector::WorldUp, FromPx( floorCollision.m_normal ) );
                    if ( slopeAngle < steepestAngle )
                    {
                        steepestAngle = slopeAngle;
                        m_floorNormal = FromPx( floorCollision.m_normal );
                        m_floorContactPoint = FromPx( floorCollision.m_point );
                    }
                }
            }
            else // We're on a valid floor
            {
                m_floorType = ControllerFloorType::Floor;

                for ( auto const& floorCollision : m_callbackHandler.m_floorCollisions )
                {
                    if ( floorCollision.m_pShape == controllerState.touchedShape )
                    {
                        m_floorNormal = FromPx( floorCollision.m_normal );
                        m_floorContactPoint = FromPx( floorCollision.m_point );
                        break;
                    }
                }
            }

            EE_ASSERT( m_floorType != ControllerFloorType::NoFloor );
        }
        else // Not ground hit so we dont have a floor
        {
            m_floorType = ControllerFloorType::NoFloor;
            m_floorNormal = Vector::Zero;
            m_floorContactPoint = Vector::Zero;
            m_timeWithoutFloor.Update( deltaTime );
        }

        //-------------------------------------------------------------------------

        // Set the world transform to the result of the controller move
        endWorldTransform.SetTranslation( FromPx( m_pController->getPosition() ) );
        SetWorldTransformDirectly( endWorldTransform, false ); // Do not fire callback as we dont want to teleport the character
        Vector const actualDeltaTranslation = endWorldTransform.GetTranslation() - startWorldTransform.GetTranslation();
        m_linearVelocity = actualDeltaTranslation / deltaTime;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugCapsule )
        {
            m_debugPostMoveTransform = endWorldTransform;
        }
        #endif

        //-------------------------------------------------------------------------

        return actualDeltaTranslation;
    }

    void CharacterComponent::ResizeCapsule( float newRadius, float newHalfHeight, bool keepFloorPosition )
    {
        EE_ASSERT( newRadius > 0.0f );
        EE_ASSERT( newHalfHeight >= 0.0f );

        float const heightDelta = newHalfHeight - m_halfHeight;
        m_radius = newRadius;
        m_halfHeight = newHalfHeight;

        //-------------------------------------------------------------------------

        ApplyOffsetToAllChildren( Vector( 0, 0, -heightDelta ) );

        //-------------------------------------------------------------------------

        auto physicsScene = m_pController->getScene();
        physicsScene->lockWrite();
        {
            m_pController->setRadius( newRadius );

            // If we are keeping the floor position, then call the provided function
            if ( keepFloorPosition )
            {
                m_pController->resize( newHalfHeight * 2.0f );
            }
            else // Just change size
            {
                m_pController->setHeight( newHalfHeight * 2.0f );
            }
        }
        physicsScene->unlockWrite();
    }

    void CharacterComponent::SetStepHeight( float stepHeight )
    {
        EE_ASSERT( stepHeight > 0.0f );
        m_stepHeight = stepHeight;

        //-------------------------------------------------------------------------

        auto physicsScene = m_pController->getScene();
        physicsScene->lockWrite();
        m_pController->setStepOffset( stepHeight );
        physicsScene->unlockWrite();
    }

    void CharacterComponent::SetSlopeLimit( Degrees slopeLimit )
    {
        EE_ASSERT( slopeLimit > 0.0f );
        m_slopeLimit = slopeLimit;

        //-------------------------------------------------------------------------

        auto physicsScene = m_pController->getScene();
        physicsScene->lockWrite();
        m_pController->setSlopeLimit( Radians( slopeLimit ).ToFloat() );
        physicsScene->unlockWrite();
    }

    void CharacterComponent::SetGravityMode( ControllerGravityMode mode, float gravityValue )
    {
        m_gravityMode = mode;
        m_gravitationalSpeed = ( m_gravityMode == ControllerGravityMode::NoGravity ) ? 0.0f : gravityValue;
    }

    void CharacterComponent::SetGravityMode( ControllerGravityMode mode )
    {
        EE_ASSERT( m_gravityMode != ControllerGravityMode::FixedVelocity );
        m_gravityMode = mode;
        m_gravityValue = ( m_gravityMode == ControllerGravityMode::NoGravity ) ? 0.0f : m_defaultGravity;
    }

    void CharacterComponent::ResetGravityMode()
    {
        m_gravityMode = ControllerGravityMode::Acceleration;
        m_gravityValue = m_defaultGravity;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void CharacterComponent::EnableGhostMode( bool isEnabled )
    {
        m_gravitationalSpeed = 0.0f;
        m_linearVelocity = Vector::Zero;
        m_floorType = ControllerFloorType::NoFloor;
        m_floorNormal = Vector::Zero;
        m_floorContactPoint = Vector::Zero;
        m_isGhostModeEnabled = isEnabled;
    }

    void CharacterComponent::DrawDebugUI()
    {
        ImGui::SeparatorText( "Capsule" );
        ImGui::Checkbox( "Debug Capsule", &m_debugCapsule );

        if ( m_debugCapsule )
        {
            ImGui::Text( "Before: %s", Math::ToString( m_debugPreMoveTransform.GetTranslation() ).c_str() );
            ImGui::Text( "After: %s", Math::ToString( m_debugPostMoveTransform.GetTranslation() ).c_str() );
            ImGui::Text( "Delta: %s", Math::ToString( m_debugPostMoveTransform.GetTranslation() - m_debugPreMoveTransform.GetTranslation() ).c_str() );
        }

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Gravity" );

        switch ( m_gravityMode )
        {
            case ControllerGravityMode::NoGravity:
            {
                ImGui::Text( "Mode: No Gravity" );
            }
            break;

            case ControllerGravityMode::Acceleration:
            {
                ImGui::Text( "Mode: Acceleration" );
                ImGui::Text( "Acceleration: %.3f", m_gravityValue );
            }
            break;

            case ControllerGravityMode::FixedVelocity:
            {
                ImGui::Text( "Mode: Fixed Velocity" );
            }
            break;
        }

        ImGui::Text( "Current Gravitational Speed: %.2f", m_gravitationalSpeed );

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Floor" );

        ImGui::Checkbox( "Draw floor info", &m_debugFloor );
        ImGui::Text( "Time without floor: %.2fs", m_timeWithoutFloor.GetElapsedTimeSeconds().ToFloat() );

        switch ( m_floorType )
        {
            case ControllerFloorType::Floor:
            {
                ImGuiX::ScopedFont sf( Colors::Green );
                ImGui::Text( "Floor Type: Navigable" );
            }
            break;

            case ControllerFloorType::Slope:
            {
                ImGuiX::ScopedFont sf( Colors::Red );
                ImGui::Text( "Floor Type: Unnavigable" );
            }
            break;

            case ControllerFloorType::NoFloor:
            {
                ImGui::Text( "Floor Type: None" );
            }
            break;
        }
    }

    void CharacterComponent::DrawDebug( Drawing::DrawContext& ctx ) const
    {
        if ( m_debugCapsule )
        {
            ctx.DrawCapsule( m_debugPreMoveTransform, m_radius, m_halfHeight, Colors::White, 2.0f );
            ctx.DrawCapsule( m_debugPostMoveTransform, m_radius, m_halfHeight, Colors::Green, 2.0f );
        }

        //-------------------------------------------------------------------------

        if ( m_debugFloor )
        {
            Vector const floorPosition = GetCapsuleBottom();
            ctx.DrawPoint( floorPosition, Colors::Yellow );
            ctx.DrawArrow( floorPosition, floorPosition + ( m_floorNormal * 0.5f ), Colors::Yellow );
            ctx.DrawCircle( floorPosition, Axis::Z, GetCapsuleRadius(), Colors::Yellow );
        }
    }
    #endif
}
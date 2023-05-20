#include "Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Physics/Physics.h"
#include "System/Profiling.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathStringHelpers.h"

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

    PxQueryHitType::Enum ControllerCallbackHandler::preFilter( PxFilterData const& queryFilterData, PxShape const* pShape, PxRigidActor const* pActor, PxHitFlags& queryFlags )
    {
        for ( auto const& ignoredComponentID : m_pComponent->m_ignoredComponents )
        {
            auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
            if ( pOwnerComponent->GetID() == ignoredComponentID )
            {
                return PxQueryHitType::eNONE;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& ignoredEntityID : m_pComponent->m_ignoredEntities )
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

    }

    void ControllerCallbackHandler::onControllerHit( const PxControllersHit& hit )
    {

    }

    void ControllerCallbackHandler::onObstacleHit( const PxControllerObstacleHit& hit )
    {

    }

    PxControllerBehaviorFlags ControllerCallbackHandler::getBehaviorFlags( const PxShape& shape, const PxActor& actor )
    {
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    PxControllerBehaviorFlags ControllerCallbackHandler::getBehaviorFlags( const PxController& controller )
    {
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    PxControllerBehaviorFlags ControllerCallbackHandler::getBehaviorFlags( const PxObstacle& obstacle )
    {
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
        m_gravitationalAcceleration = m_defaultGravitationalAcceleration;

        //-------------------------------------------------------------------------

        m_pCallbackHandler = EE::New<PX::ControllerCallbackHandler>( this );
    }

    void CharacterComponent::Shutdown()
    {
        EE::Delete( m_pCallbackHandler );
        SpatialEntityComponent::Shutdown();
    }

    //-------------------------------------------------------------------------

    bool CharacterComponent::HasValidCharacterSetup() const
    {
        if ( m_defaultRadius <= 0 || m_defaultHalfHeight <= 0 )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid radius or half height on Physics Capsule Component: %s (%u). Negative or zero values are not allowed!", GetNameID().c_str(), GetID() );
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
        }
        else
        #endif
        {
            Vector desiredDeltaTranslation = deltaTranslation;

            // Gravity
            //-------------------------------------------------------------------------

            if ( m_isGravityEnabled )
            {
                m_verticalSpeed += ( m_gravitationalAcceleration * deltaTime );
                Vector const verticalDelta = Physics::Constants::s_gravity * ( m_verticalSpeed * deltaTime );
                desiredDeltaTranslation += verticalDelta;
            }
            else
            {
                m_verticalSpeed = 0.0f;
            }

            // Move Controller
            //-------------------------------------------------------------------------

            physx::PxFilterData filterData( m_collisionSettings.m_collidesWithMask, 0, 0, 0 );
            physx::PxControllerFilters filters( &filterData );

            auto physicsScene = m_pController->getScene();
            physicsScene->lockWrite();
            physx::PxControllerCollisionFlags const collisionResultFlags = m_pController->move( ToPx( desiredDeltaTranslation ), 0.01f, deltaTime, filters, nullptr );
            physicsScene->unlockWrite();

            // Results of move
            //-------------------------------------------------------------------------

            physx::PxControllerState state;
            m_pController->getState( state );

            if ( collisionResultFlags.isSet( physx::PxControllerCollisionFlag::eCOLLISION_UP ) )
            {

            }

            if ( collisionResultFlags.isSet( physx::PxControllerCollisionFlag::eCOLLISION_DOWN ) )
            {

            }

            if ( collisionResultFlags.isSet( physx::PxControllerCollisionFlag::eCOLLISION_SIDES ) )
            {

            }

            endWorldTransform.SetTranslation( FromPx( m_pController->getPosition() ) );
            SetWorldTransformDirectly( endWorldTransform, false ); // Do not fire callback as we dont want to teleport the character
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugCapsule )
        {
            m_debugPostMoveTransform = endWorldTransform;
        }
        #endif

        //-------------------------------------------------------------------------

        Vector const actualDeltaTranslation = endWorldTransform.GetTranslation() - startWorldTransform.GetTranslation();
        m_linearVelocity = actualDeltaTranslation / deltaTime;

        //-------------------------------------------------------------------------

        return actualDeltaTranslation;
    }

    void CharacterComponent::ResizeCapsule( float newRadius, float newHalfHeight, bool keepFloorPosition )
    {
        EE_ASSERT( newRadius > 0.0f );
        EE_ASSERT( newHalfHeight > 0.0f );

        m_radius = newRadius;
        m_halfHeight = newHalfHeight;

        //-------------------------------------------------------------------------

        auto physicsScene = m_pController->getScene();
        physicsScene->lockWrite();
        {
            m_pController->setRadius( newRadius );

            // If we are keeping the floor position, then call the provided function
            if ( keepFloorPosition )
            {
                m_pController->resize( newHalfHeight );
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

    void CharacterComponent::SetGravityEnabled( bool isGravityEnabled, float initialGravitationalSpeed /*= 0 */ )
    {
        m_isGravityEnabled = isGravityEnabled;
        m_verticalSpeed = m_isGravityEnabled ? initialGravitationalSpeed : 0.0f;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void CharacterComponent::DrawDebugUI()
    {
        ImGuiX::TextSeparator( "Capsule" );
        ImGui::Checkbox( "Debug Capsule", &m_debugCapsule );

        if ( m_debugCapsule )
        {
            ImGui::Text( "Before: %s", Math::ToString( m_debugPreMoveTransform.GetTranslation() ).c_str() );
            ImGui::Text( "After: %s", Math::ToString( m_debugPostMoveTransform.GetTranslation() ).c_str() );
            ImGui::Text( "Delta: %s", Math::ToString( m_debugPostMoveTransform.GetTranslation() - m_debugPreMoveTransform.GetTranslation() ).c_str() );
        }

        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Gravity" );

        ImGui::Text( "Is Enabled: %s", m_isGravityEnabled ? "True" : "False" );
        ImGui::Text( "Acceleration: %.3f", m_gravitationalAcceleration );
        ImGui::Text( "Current Vertical Speed: %.2f", m_verticalSpeed );

        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Floor" );

        ImGui::Checkbox( "Draw floor info", &m_debugFloor );
        ImGui::Text( "Time without floor: %.2f", m_timeWithoutFloor );

        switch ( m_floorType )
        {
            case FloorType::Navigable:
            {
                ImGuiX::ScopedFont sf( ImGuiX::ImColors::Lime );
                ImGui::Text( "Floor Type: Navigable" );
            }
            break;

            case FloorType::Unnavigable:
            {
                ImGuiX::ScopedFont sf( ImGuiX::ImColors::Red );
                ImGui::Text( "Floor Type: Unnavigable" );
            }
            break;

            case FloorType::NoFloor:
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
            ctx.DrawCapsule( m_debugPostMoveTransform, m_radius, m_halfHeight, Colors::Lime, 2.0f );
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
#include "EntitySystem_DebugCameraController.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    constexpr static float const g_mouseSensitivity = Math::DegreesToRadians * 0.25f; // Convert pixels to radians


    //-------------------------------------------------------------------------

    void DebugCameraController::RegisterComponent( EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<DebugCameraComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCameraComponent == nullptr );
            m_pCameraComponent = pCameraComponent;
        }
    }

    void DebugCameraController::UnregisterComponent( EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<DebugCameraComponent>( pComponent ) )
        {
            EE_ASSERT( m_pCameraComponent == pCameraComponent );
            m_pCameraComponent = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void DebugCameraController::Update( EntityWorldUpdateContext const& ctx )
    {
        if ( m_pCameraComponent == nullptr )
        {
            return;
        }

        if ( !m_pCameraComponent->IsEnabled() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pCameraComponent != nullptr );

        //-------------------------------------------------------------------------

        Seconds const deltaTime = ctx.GetRawDeltaTime();
        auto pInputSystem = ctx.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );

        auto const pMouseState = pInputSystem->GetMouseState();
        auto const pKeyboardState = pInputSystem->GetKeyboardState();

        // Speed Update
        //-------------------------------------------------------------------------

        if ( pKeyboardState->IsAltHeldDown() )
        {
            if ( pMouseState->WasReleased( Input::MouseButton::Middle ) )
            {
                m_pCameraComponent->ResetMoveSpeed();
            }
            else
            {
                m_pCameraComponent->AdjustMoveSpeed( pMouseState->GetWheelDelta() );
            }
        }

        // Position update
        //-------------------------------------------------------------------------

        bool const fwdButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_W );
        bool const backButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_S );
        bool const leftButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_A );
        bool const rightButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_D );
        bool const needsPositionUpdate = fwdButton || backButton || leftButton || rightButton;

        if ( needsPositionUpdate )
        {
            float LR = 0;
            if ( leftButton ) { LR -= 1.0f; }
            if ( rightButton ) { LR += 1.0f; }

            float FB = 0;
            if ( fwdButton ) { FB += 1.0f; }
            if ( backButton ) { FB -= 1.0f; }

            m_pCameraComponent->MoveCamera( deltaTime, FB, LR, 0.0f );
        }

        // Orientation update
        //-------------------------------------------------------------------------

        if ( pMouseState->IsHeldDown( Input::MouseButton::Right ) )
        {
            Vector const mouseDelta = pMouseState->GetMovementDelta();
            Float2 const directionDelta = ( mouseDelta.GetNegated() * g_mouseSensitivity ).ToFloat2();
            m_pCameraComponent->OrientCamera( deltaTime, directionDelta.m_x, directionDelta.m_y );
        }
    }
}
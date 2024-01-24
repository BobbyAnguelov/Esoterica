#include "EntitySystem_DebugCameraController.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Input/InputSystem.h"
#include "Base/Drawing/DebugDrawing.h"

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
        Seconds const deltaTime = ctx.GetRawDeltaTime();

        auto pInputSystem = ctx.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );
        auto const pKeyboardMouse = pInputSystem->GetKeyboardMouse();

        //-------------------------------------------------------------------------

        if ( m_pCameraComponent == nullptr )
        {
            return;
        }

        if ( !m_pCameraComponent->IsEnabled() )
        {
            return;
        }

        // Speed Update
        //-------------------------------------------------------------------------

        if ( pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_LAlt ) || pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_RAlt ) )
        {
            if ( pKeyboardMouse->WasReleased( Input::InputID::Mouse_Middle ) )
            {
                m_pCameraComponent->ResetMoveSpeed();
            }
            else
            {
                float const wheelDelta = pKeyboardMouse->GetValue( Input::InputID::Mouse_WheelVertical );
                m_pCameraComponent->AdjustMoveSpeed( wheelDelta );
            }
        }

        // Position update
        //-------------------------------------------------------------------------

        bool const fwdButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_W );
        bool const backButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_S );
        bool const leftButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_A );
        bool const rightButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_D );
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

        if ( pKeyboardMouse->IsHeldDown( Input::InputID::Mouse_Right ) )
        {
            Vector const mouseDelta( pKeyboardMouse->GetMouseDelta() );
            Float2 const directionDelta = ( mouseDelta.GetNegated() * g_mouseSensitivity ).ToFloat2();
            m_pCameraComponent->OrientCamera( deltaTime, directionDelta.m_x, directionDelta.m_y );
        }
    }
}
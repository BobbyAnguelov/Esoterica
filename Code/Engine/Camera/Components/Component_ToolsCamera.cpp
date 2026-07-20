#include "Component_ToolsCamera.h"
#include "Engine/Camera/Components/Component_Camera.h"
#include "Engine/Camera/CameraMath.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Input/InputSystem.h"
#include "imgui.h"

//-------------------------------------------------------------------------

namespace EE
{
    constexpr static float const g_mouseSensitivityOrientation = Math::DegreesToRadians * 0.25f; // Convert pixels to radians
    constexpr static float const g_mouseSensitivityPan = 0.1f; // Convert pixels to metres
    constexpr static float const g_orbitDistanceSensitivity = 0.5f; // Convert wheel steps to metres

    //-------------------------------------------------------------------------

    void ToolsCameraComponent::Initialize()
    {
        CameraComponent::Initialize();
    }

    void ToolsCameraComponent::Update( EntityWorldUpdateContext const& ctx )
    {
        if ( !m_isUpdateEnabled )
        {
            return;
        }

        // Mode
        //-------------------------------------------------------------------------

        auto pInputSystem = ctx.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );
        auto const pKeyboardMouse = pInputSystem->GetKeyboardMouse();

        if ( pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_LAlt ) || pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_RAlt ) )
        {

            m_mode = Mode::Orbit;
        }
        else
        {
            m_mode = Mode::FreeLook;
        }

        // Speed Update
        //-------------------------------------------------------------------------

        if ( pKeyboardMouse->IsHeldDown( Input::InputID::Mouse_Button5 ) )
        {
            if ( pKeyboardMouse->WasReleased( Input::InputID::Mouse_Middle ) )
            {
                ResetMoveSpeed();
            }
            else
            {
                float const wheelDelta = pKeyboardMouse->GetValue( Input::InputID::Mouse_WheelVertical );
                AdjustMoveSpeed( wheelDelta );
            }
        }

        //-------------------------------------------------------------------------

        m_bIsManipulatingView = false;

        if ( m_mode == Mode::FreeLook )
        {
            UpdateFreeLook( ctx, pKeyboardMouse );
        }
        else
        {
            UpdateOrbit( ctx, pKeyboardMouse );
        }
    }

    void ToolsCameraComponent::UpdateFreeLook( EntityWorldUpdateContext const& ctx, Input::KeyboardMouseDevice const* pKeyboardMouse )
    {
        Seconds const deltaTime = ctx.GetRawDeltaTime();

        // Position update
        //-------------------------------------------------------------------------

        bool const fwdButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_W );
        bool const backButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_S );
        bool const leftButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_A );
        bool const rightButton = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_D );

        bool const isModifierDown = pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_LCtrl ) || pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_RCtrl );
        bool const needsPositionUpdate = !isModifierDown && ( fwdButton || backButton || leftButton || rightButton );

        if ( needsPositionUpdate )
        {
            float LR = 0;
            if ( leftButton ) { LR -= 1.0f; }
            if ( rightButton ) { LR += 1.0f; }

            float FB = 0;
            if ( fwdButton ) { FB += 1.0f; }
            if ( backButton ) { FB -= 1.0f; }

            MoveCamera( deltaTime, FB, LR, 0.0f );
        }

        // Orientation update
        //-------------------------------------------------------------------------

        if ( pKeyboardMouse->IsHeldDown( Input::InputID::Mouse_Right ) )
        {
            Vector const mouseDelta( pKeyboardMouse->GetMouseDelta() );
            Float2 const directionDelta = ( mouseDelta.GetNegated() * g_mouseSensitivityOrientation ).ToFloat2();
            OrientCamera( deltaTime, directionDelta.m_x, directionDelta.m_y );
            m_bIsManipulatingView = true;
        }
        else if ( pKeyboardMouse->IsHeldDown( Input::InputID::Mouse_Middle ) )
        {
            Vector const mouseDelta( pKeyboardMouse->GetMouseDelta() );
            Float2 const directionDelta = ( mouseDelta * g_mouseSensitivityPan ).ToFloat2();
            PanCamera( deltaTime, -directionDelta.m_x, directionDelta.m_y );
            m_bIsManipulatingView = true;
        }
    }

    void ToolsCameraComponent::UpdateOrbit( EntityWorldUpdateContext const& ctx, Input::KeyboardMouseDevice const* pKeyboardMouse )
    {
        Seconds const deltaTime = ctx.GetRawDeltaTime();

        if ( pKeyboardMouse->IsHeldDown( Input::InputID::Mouse_Right ) )
        {
            Vector const mouseDelta( pKeyboardMouse->GetMouseDelta().m_x, -pKeyboardMouse->GetMouseDelta().m_y, 0 );
            Vector adjustedCameraInputs = ( mouseDelta * g_mouseSensitivityOrientation ) * 10;
            Radians const maxAngularVelocityForThisFrame = Math::Pi * deltaTime;
            adjustedCameraInputs *= (float) maxAngularVelocityForThisFrame;
            AdjustOrbitAngle( adjustedCameraInputs.GetX(), adjustedCameraInputs.GetY() );

            m_bIsManipulatingView = true;
        }

        float const wheelDelta = pKeyboardMouse->GetValue( Input::InputID::Mouse_WheelVertical );
        if ( !Math::IsNearZero( wheelDelta ) )
        {
            float const adjustedDistance = Math::Max( 0.0f, m_orbitDistance + ( wheelDelta * -g_orbitDistanceSensitivity ) );
            SetOrbitDistance( adjustedDistance );
        }
    }

    void ToolsCameraComponent::OnTeleport()
    {
        Math::Camera::CalculatePitchAndYaw( GetWorldTransform().GetForwardVector(), m_yaw, m_pitch );
    }

    void ToolsCameraComponent::FocusOn( Vector const& position, TInlineVector<Vector, 8> const& boundingPoints )
    {
        EE_ASSERT( !Math::IsNearZero( GetViewVolume().GetHorizontalFOV().ToFloat() ) );

        float const recipTanHalfFOV = 1.0f / Math::Tan( GetViewVolume().GetHorizontalFOV().ToFloat() / 2.0f );

        // Find the maximum distance at which every bound point is inside the view volume.
        float maxDist = 1.0f;
        for ( Vector const& bp : boundingPoints )
        {
            Vector const centerToCorner = bp - position;
            float const hDist = recipTanHalfFOV * Math::Abs( GetRightVector().GetDot3( centerToCorner ) );
            float const vDist = recipTanHalfFOV * GetViewVolume().GetAspectRatio() * Math::Abs( GetUpVector().GetDot3( centerToCorner ) );
            float const fDist = GetForwardVector().GetDot3( centerToCorner );
            maxDist = Math::Max( Math::Max( hDist, vDist ) + fDist, maxDist );
        }

        m_orbitDistance = maxDist;
        SetPositionAndLookAtDirection( position - GetForwardVector() * maxDist, GetForwardVector() );
        OnTeleport();
    }

    void ToolsCameraComponent::FocusOn( OBB const& bounds )
    {
        TInlineVector<Vector, 8> corners( 8 );
        bounds.GetCorners( corners.begin() );
        FocusOn( bounds.m_center, corners );
    }

    void ToolsCameraComponent::FocusOn( AABB const& bounds )
    {
        TInlineVector<Vector, 8> corners( 8 );
        bounds.GetCorners( corners.begin() );
        FocusOn( bounds.GetCenter(), corners );
    }

    void ToolsCameraComponent::FocusOn( Vector const& worldPoint, float viewDistance )
    {
        SetPositionAndLookAtDirection( worldPoint - GetForwardVector() * viewDistance, GetForwardVector() );
        m_orbitDistance = viewDistance;
        OnTeleport();
    }

    void ToolsCameraComponent::SetCameraWorldTransform( Transform const& worldTransform )
    {
        SetWorldTransform( worldTransform );
        OnTeleport();
    }

    void ToolsCameraComponent::SetPositionAndLookAtTarget( Vector const& cameraPosition, Vector const& lookAtTarget )
    {
        EE_ASSERT( !cameraPosition.IsNearEqual3( lookAtTarget ) );

        Vector const lookAtDir = ( lookAtTarget - cameraPosition ).GetNormalized3();

        Math::Camera::CalculatePitchAndYaw( lookAtDir, m_yaw, m_pitch );
        Transform const newCameraTransform = Math::Camera::CalculateCameraTransform( m_yaw, m_pitch, cameraPosition );
        SetLocalTransform( newCameraTransform );

        OnTeleport();
    }

    void ToolsCameraComponent::SetPositionAndLookAtDirection( Vector const& cameraPosition, Vector const& lookAtDir )
    {
        EE_ASSERT( lookAtDir.IsNormalized3() );

        Math::Camera::CalculatePitchAndYaw( lookAtDir, m_yaw, m_pitch );
        Transform const newCameraTransform = Math::Camera::CalculateCameraTransform( m_yaw, m_pitch, cameraPosition );
        SetLocalTransform( newCameraTransform );
        OnTeleport();
    }

    //-------------------------------------------------------------------------

    void ToolsCameraComponent::AdjustMoveSpeed( float steps )
    {
        float const delta = ( s_moveSpeedStepSize * steps );
        SetMoveSpeed( m_moveSpeed + delta );
    }

    void ToolsCameraComponent::MoveCamera( Seconds deltaTime, float forward, float right, float up )
    {
        Vector const spatialInput( right, forward, up, 0 );

        Transform cameraTransform = GetWorldTransform();
        Vector const forwardDirection = cameraTransform.GetForwardVector();
        Vector const rightDirection = cameraTransform.GetRightVector();
        Vector const upDirection = cameraTransform.GetUpVector();

        Vector const moveDelta = Vector( m_moveSpeed * deltaTime );
        Vector const deltaForward = spatialInput.GetSplatY() * moveDelta;
        Vector const deltaRight = spatialInput.GetSplatX() * moveDelta;
        Vector const deltaUp = spatialInput.GetSplatZ() * moveDelta;

        // Calculate position delta
        Vector positionDelta = Vector::Zero;
        positionDelta = deltaRight * rightDirection;
        positionDelta += deltaForward * forwardDirection;
        positionDelta += deltaUp * upDirection;
        positionDelta.SetW0();

        // Update camera transform
        cameraTransform.AddTranslation( positionDelta );
        SetWorldTransform( cameraTransform );
    }

    void ToolsCameraComponent::OrientCamera( Seconds deltaTime, Radians rightDelta, Radians upDelta )
    {
        // Invert left/right when flipped
        if ( m_pitch < -Math::PiDivTwo || m_pitch > Math::PiDivTwo )
        {
            rightDelta = -rightDelta;
        }

        m_yaw += rightDelta;
        m_yaw.Clamp180();

        m_pitch += upDelta;
        m_pitch.Clamp180();

        Transform const newCameraTransform = Math::Camera::CalculateCameraTransform( m_yaw, m_pitch, GetLocalTransform().GetTranslation() );
        SetLocalTransform( newCameraTransform );
    }

    //-------------------------------------------------------------------------

    void ToolsCameraComponent::AdjustOrbitAngle( Radians deltaHorizontalAngle, Radians deltaVerticalAngle )
    {
        if ( Math::IsNearZero( (float) deltaVerticalAngle ) && Math::IsNearZero( (float) deltaVerticalAngle ) )
        {
            return;
        }

        // Adjust yaw and pitch based on input
        m_yaw -= deltaHorizontalAngle;
        m_yaw.ClampPositive360();

        m_pitch += deltaVerticalAngle;
        m_pitch.Clamp( Degrees( -89 ), Degrees( 89 ) );

        // Update rotation
        auto const rotZ = Quaternion( Float3::WorldUp, m_yaw );
        auto const rotX = Quaternion( Float3::WorldRight, m_pitch );
        auto const rotTotal = rotX * rotZ;

        Transform const startWorldTransform = GetWorldTransform();
        Vector const orbitTarget = startWorldTransform.GetTranslation() + ( GetForwardVector() * m_orbitDistance );
        Vector const orbitCameraPosition = rotTotal.RotateVector( Vector::WorldBackward ) * m_orbitDistance;
        Transform camWorldTransform = Transform( rotTotal, orbitTarget + orbitCameraPosition );

        SetWorldTransform( camWorldTransform );
    }

    void ToolsCameraComponent::SetOrbitDistance( float newDistance )
    {
        newDistance = Math::Max( newDistance, 0.1f );
        float const deltaDistance = m_orbitDistance - newDistance;
        m_orbitDistance = newDistance;

        Transform camWorldTransform = GetWorldTransform();
        camWorldTransform.AddTranslation( GetForwardVector() * deltaDistance );
        SetWorldTransform( camWorldTransform );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ToolsCameraComponent::DrawDebugUI()
    {
        if ( m_mode == Mode::FreeLook )
        {
            ImGui::Text( "Camera Type: Free Look" );
            ImGui::Text( "Pitch: %.2f deg", m_pitch.ToDegrees().ToFloat() );
            ImGui::Text( "Yaw: %.2f deg ", m_yaw.ToDegrees().ToFloat() );
        }
        else
        {
            ImGui::Text( "Camera Type: Orbit" );
            ImGui::Text( "Orbit Distance: %.2fm", m_orbitDistance );
        }
    }
    #endif
}
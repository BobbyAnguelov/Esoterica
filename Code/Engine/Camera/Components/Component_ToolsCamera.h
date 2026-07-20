#pragma once

#include "Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Input { class KeyboardMouseDevice; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ToolsCameraComponent : public CameraComponent
    {
        EE_ENTITY_COMPONENT( ToolsCameraComponent );

    public:

        constexpr static float const                s_minSpeed = 0.5f; // m/s
        constexpr static float const                s_maxSpeed = 100.0f; // m/s
        constexpr static float const                s_moveSpeedStepSize = 0.5f; // m/s

        enum class Mode
        {
            FreeLook = 0,
            Orbit
        };

    public:

        ToolsCameraComponent() = default;
        ToolsCameraComponent( StringID nameID ) : CameraComponent( nameID ) {}

        inline void SetUpdateEnabled( bool enabled ) { m_isUpdateEnabled = enabled; }

        void Update( EntityWorldUpdateContext const& ctx );

        // General Camera
        //-------------------------------------------------------------------------

        Mode GetMode() const { return m_mode; }
        inline bool IsInFreeLookMode() const { return m_mode == Mode::FreeLook; }
        inline bool IsInOrbitMode() const { return m_mode == Mode::Orbit; }

        inline bool IsManipulatingView() const { return m_bIsManipulatingView; }

        inline Radians GetPitch() const { return m_pitch; }
        inline Radians GetYaw() const { return m_yaw; }

        // Direct Positioning
        //-------------------------------------------------------------------------

        // Directly set the camera world transform
        void SetCameraWorldTransform( Transform const& worldTransform );

        // Set the camera world position and world look at target
        void SetPositionAndLookAtTarget( Vector const& cameraPosition, Vector const& lookatTarget );

        // Set the camera world position and view direction
        void SetPositionAndLookAtDirection( Vector const& cameraPosition, Vector const& lookatDir );

        // Focus the camera to the specified bounds
        void FocusOn( Vector const& worldPoint, float viewDistance );

        // Focus the camera to the specified bounds
        void FocusOn( OBB const& bounds );

        // Focus the camera to the specified bounds
        void FocusOn( AABB const& bounds );

        // Free Look
        //-------------------------------------------------------------------------

        // Set the default move speed i.e. the value we will reset the camera to
        inline void SetDefaultMoveSpeed( float newDefaultSpeed ) { m_defaultMoveSpeed = Math::Clamp( newDefaultSpeed, s_minSpeed, s_maxSpeed ); }

        // Reset the move speed to the default value
        inline void ResetMoveSpeed() { m_moveSpeed = m_defaultMoveSpeed; }

        // Get the current move speed
        inline float GetMoveSpeed() const { return m_moveSpeed; }

        // Set the current move speed
        inline void SetMoveSpeed( float newSpeed ) { m_moveSpeed = Math::Clamp( newSpeed, s_minSpeed, s_maxSpeed ); }

        // Adjust the move speed by +/- X discrete steps
        void AdjustMoveSpeed( float steps );

        // Orbit
        //-------------------------------------------------------------------------

        void AdjustOrbitAngle( Radians deltaHorizontalAngle, Radians deltaVerticalAngle );
        void SetOrbitDistance( float newDistance );

    private:

        virtual void Initialize() override;


        void UpdateFreeLook( EntityWorldUpdateContext const& ctx, Input::KeyboardMouseDevice const* pKeyboardMouse );

        void UpdateOrbit( EntityWorldUpdateContext const& ctx, Input::KeyboardMouseDevice const* pKeyboardMouse );

        // Move the camera position. Input units are a multiplier of the move speed i.e. 1.0f = full speed. 
        // Note: the camera will not move faster than the move speed no matter the input value
        void MoveCamera( Seconds deltaTime, float forward, float right, float up );

        // Pan the view relative to the forward vector
        void PanCamera( Seconds deltaTime, float right, float up ) { MoveCamera( deltaTime, 0.0f, right, up ); }

        // Orient the camera. Input units are the actual angle delta
        void OrientCamera( Seconds deltaTime, Radians rightDelta, Radians upDelta );

        // Generic focus on function that takes in a set of point that define the extents of an area to focus on
        void FocusOn( Vector const& position, TInlineVector<Vector, 8> const& boundingPoints );

        // Called whenever we explicitly set the camera position (so that derived classes can clear any buffered data)
        virtual void OnTeleport();

        #if EE_DEVELOPMENT_TOOLS
        // Override this function to draw custom imgui controls in the camera debug view
        virtual void DrawDebugUI() override;
        #endif

    private:

        Mode                                        m_mode = Mode::FreeLook;
        bool                                        m_isUpdateEnabled = true;
        bool                                        m_bIsManipulatingView = false;

        float                                       m_defaultMoveSpeed = 5.0f;
        float                                       m_moveSpeed = m_defaultMoveSpeed;

        float                                       m_orbitDistance = 2.f;
    };
}
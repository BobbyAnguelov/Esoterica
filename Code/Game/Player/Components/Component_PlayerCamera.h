#pragma once

#include "Engine/Camera/Components/Component_Camera.h"
#include "Game/_Module/API.h"

//-------------------------------------------------------------------------
// Camera
//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API PlayerCameraComponent : public CameraComponent
    {
        EE_ENTITY_COMPONENT( PlayerCameraComponent );

    public:

        constexpr static float const                s_minSpeed = 0.5f; // m/s
        constexpr static float const                s_maxSpeed = 100.0f; // m/s
        constexpr static float const                s_moveSpeedStepSize = 0.5f; // m/s 
        constexpr static float const                s_maxAngularVelocity = Math::Pi * 5.0f;

    public:

        using CameraComponent::CameraComponent;

        //-------------------------------------------------------------------------

        void Update( EntityWorldUpdateContext const& ctx, Float2 const& cameraInputs );
        void FinalizeCamera();
        void ResetCamera();

        Vector const& GetCameraPosition() const;
        inline Vector const& GetCameraRelativeForwardVector() const { return m_cameraRelativeForwardVector; }
        inline Vector const& GetCameraRelativeRightVector() const { return m_cameraRelativeRightVector; }
        inline Vector const& GetCameraRelativeForwardVector2D() const { return m_cameraRelativeForwardVector2D; }
        inline Vector const& GetCameraRelativeRightVector2D() const { return m_cameraRelativeRightVector2D; }

        float GetDefaultOrbitDistance() const { return m_defaultOrbitDistance; }
        Float3 GetDefaultOrbitTargetOffset() const { return m_defaultOrbitTargetOffset; }

        void SetOrbitAngle( Radians deltaHorizontalAngle, Radians deltaVerticalAngle );

        void SetOrbitTargetOffset( Float3 newTargetOffset );
        void ResetOrbitTargetOffset() { SetOrbitTargetOffset( m_defaultOrbitTargetOffset ); }

        void SetOrbitDistance( float newDistance );
        void ResetOrbitDistance() { SetOrbitDistance( m_defaultOrbitDistance ); }

    private:

        virtual void Initialize() override;

        void CalculateCameraTransform();

        #if EE_DEVELOPMENT_TOOLS
        // Override this function to draw custom imgui controls in the camera debug view
        virtual void DrawDebugUI() override;
        #endif

    private:

        EE_REFLECT();
        Float3                  m_defaultOrbitTargetOffset = Float3::Zero;

        EE_REFLECT();
        float                   m_defaultOrbitDistance = 4.5f;

        Transform               m_orbitTransform; // The orbit transform (local to the orbit world space point)

        Float3                  m_orbitTargetOffset = Float3::Zero;
        float                   m_orbitDistance = 3.5f;

        Vector                  m_cameraRelativeForwardVector;
        Vector                  m_cameraRelativeRightVector;
        Vector                  m_cameraRelativeForwardVector2D;
        Vector                  m_cameraRelativeRightVector2D;
    };
}
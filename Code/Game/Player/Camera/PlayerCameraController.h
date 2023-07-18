#pragma once
#include "Base/Math/Vector.h"


//-------------------------------------------------------------------------

namespace EE
{
    class OrbitCameraComponent;
    class EntityWorldUpdateContext;
}

//-------------------------------------------------------------------------

namespace EE::Player
{
    class CameraController
    {
    public:

        CameraController( OrbitCameraComponent* pCamera );

        void UpdateCamera( EntityWorldUpdateContext const& ctx );
        void FinalizeCamera();

        Vector const& GetCameraPosition() const;
        inline Vector const& GetCameraRelativeForwardVector() const { return m_cameraRelativeForwardVector; }
        inline Vector const& GetCameraRelativeRightVector() const { return m_cameraRelativeRightVector; }
        inline Vector const& GetCameraRelativeForwardVector2D() const { return m_cameraRelativeForwardVector2D; }
        inline Vector const& GetCameraRelativeRightVector2D() const { return m_cameraRelativeRightVector2D; }

    private:

        OrbitCameraComponent*   m_pCamera = nullptr;
        Vector                  m_cameraRelativeForwardVector;
        Vector                  m_cameraRelativeRightVector;
        Vector                  m_cameraRelativeForwardVector2D;
        Vector                  m_cameraRelativeRightVector2D;
    };
}
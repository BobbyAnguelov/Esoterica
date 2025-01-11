#include "Component_FreeLookCamera.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE
{
    static Transform CalculateFreeLookCameraTransform( Vector const& position, Radians yaw, Radians pitch )
    {
        auto const rotZ = Quaternion( Float3::WorldUp, yaw );
        auto const rotX = Quaternion( Float3::WorldRight, pitch );
        auto const rotTotal = rotX * rotZ;
        return Transform( rotTotal, position );
    }

    static void GetYawPitchDeltas( Vector const& dir, Radians& outYaw, Radians& outPitch )
    {
        Vector lookDir = dir.GetNormalized3();
        outYaw = Math::GetYawAngleBetweenVectors( Vector::WorldForward, lookDir );
        outPitch = Math::GetPitchAngleBetweenNormalizedVectors( Vector::WorldForward, lookDir );
        EE_ASSERT( !Math::IsNaNOrInf( outYaw.ToFloat() ) && !Math::IsNaNOrInf( outPitch.ToFloat() ) );
    }

    //-------------------------------------------------------------------------

    void FreeLookCameraComponent::Initialize()
    {
        CameraComponent::Initialize();
        ResetView();
    }

    void FreeLookCameraComponent::OnWorldTransformUpdated()
    {
        CameraComponent::OnWorldTransformUpdated();

        // Ensure that the yaw/pitch are kept in sync
        Vector const currentForwardDir = GetForwardVector();
        GetYawPitchDeltas( currentForwardDir, m_yaw, m_pitch );
    }

    void FreeLookCameraComponent::AdjustPitchAndYaw( Radians yawDelta, Radians pitchDelta )
    {
        // Adjust yaw and pitch based on input
        m_yaw += yawDelta;
        m_yaw.ClampPositive360();

        m_pitch += pitchDelta;
        m_pitch.ClampPositive360();

        // We need to make a copy since the local transform update will re-generate the pitch/yaw values
        auto const yawCopy = m_yaw;
        auto const pitchCopy = m_pitch;

        // Update the camera transform
        Transform const newCameraTransform = CalculateFreeLookCameraTransform( GetLocalTransform().GetTranslation(), m_yaw, m_pitch );
        SetLocalTransform( newCameraTransform );

        // Set back the correct pitch and yaw values
        m_yaw = yawCopy;
        m_pitch = pitchCopy;
    }

    //-------------------------------------------------------------------------

    void FreeLookCameraComponent::ResetView()
    {
        SetPositionAndLookAtTarget( Vector( 0, -5, 5 ), Vector( 0, 0, 0 ) );
        OnTeleport();
    }

    void FreeLookCameraComponent::FocusOn( Vector const& position, TInlineVector<Vector, 8> const& boundingPoints )
    {
        EE_ASSERT( !Math::IsNearZero( GetViewVolume().GetFOV().ToFloat() ) );

        float const recipTanHalfFOV = 1.0f / Math::Tan( GetViewVolume().GetFOV().ToFloat() / 2.0f );

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
        SetPositionAndLookAtDirection( position - GetForwardVector() * maxDist, GetForwardVector() );

        OnTeleport();
    }

    void FreeLookCameraComponent::FocusOn( OBB const& bounds )
    {
        TInlineVector<Vector, 8> corners( 8 );
        bounds.GetCorners( corners.begin() );
        FocusOn( bounds.m_center, corners );
    }

    void FreeLookCameraComponent::FocusOn( AABB const& bounds )
    {
        TInlineVector<Vector, 8> corners( 8 );
        bounds.GetCorners( corners.begin() );
        FocusOn( bounds.GetCenter(), corners );
    }

    void FreeLookCameraComponent::SetPositionAndLookAtTarget( Vector const& cameraPosition, Vector const& lookAtTarget )
    {
        if ( cameraPosition.IsNearEqual3( lookAtTarget ) )
        {
            m_yaw = m_pitch = 0.0f;
        }
        else // If we have a valid target adjust view direction
        {
            Vector const lookAtDir = ( lookAtTarget - cameraPosition ).GetNormalized3();
            GetYawPitchDeltas( lookAtDir, m_yaw, m_pitch );
        }

        //-------------------------------------------------------------------------

        Transform const newCameraTransform = CalculateFreeLookCameraTransform( cameraPosition, m_yaw, m_pitch );
        SetLocalTransform( newCameraTransform );

        OnTeleport();
    }

    void FreeLookCameraComponent::SetPositionAndLookAtDirection( Vector const& cameraPosition, Vector const& lookAtDir )
    {
        EE_ASSERT( lookAtDir.IsNormalized3() );

        GetYawPitchDeltas( lookAtDir, m_yaw, m_pitch );
        Transform const newCameraTransform = CalculateFreeLookCameraTransform( cameraPosition, m_yaw, m_pitch );
        SetLocalTransform( newCameraTransform );

        //-------------------------------------------------------------------------

        OnTeleport();
    }
}
#include "CameraMath.h"

//-------------------------------------------------------------------------

namespace EE::Math::Camera
{
    void CalculatePitchAndYaw( Vector const& direction, Radians& outYaw, Radians& outPitch )
    {
        Vector lookDir = direction.GetNormalized3();
        outYaw = Math::CalculateYawAngleBetweenVectors( Vector::WorldForward, lookDir );
        outPitch = Math::CalculatePitchAngleBetweenUnitVectors( Vector::WorldForward, lookDir );
        EE_ASSERT( Math::IsFinite( outYaw.ToFloat() ) && Math::IsFinite( outPitch.ToFloat() ) );
    }

    void CalculatePitchAndYaw( Quaternion const& q, Radians& outYaw, Radians& outPitch )
    {
        EE_ASSERT( q.IsNormalized() );
        CalculatePitchAndYaw( q.GetForwardVector(), outPitch, outYaw );
    }

    Quaternion CalculateOrientationFromPitchAndYaw( Radians yaw, Radians pitch )
    {
        Quaternion const rotYaw = Quaternion( Vector::AxisYaw, yaw );
        Quaternion const rotPitch = Quaternion( Vector::AxisPitch, pitch );
        Quaternion const orientation = rotPitch * rotYaw;
        Vector v = orientation.GetForwardVector();
        return orientation;
    }

    Transform CalculateCameraTransform( Radians yaw, Radians pitch, Vector const& position )
    {
        return Transform( CalculateOrientationFromPitchAndYaw( yaw, pitch ), position );
    }
}
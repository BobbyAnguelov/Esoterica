#pragma once

#include "Base/Math/Transform.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Math::Camera
{
    void CalculatePitchAndYaw( Vector const& direction, Radians& outYaw, Radians& outPitch );
    void CalculatePitchAndYaw( Quaternion const& q, Radians& outYaw, Radians& outPitch );
    Quaternion CalculateOrientationFromPitchAndYaw( Radians yaw, Radians pitch );
    Transform CalculateCameraTransform( Radians yaw, Radians pitch, Vector const& position );
}
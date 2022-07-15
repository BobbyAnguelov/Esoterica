#pragma once
#include "Math.h"
#include "Vector.h"
#include "System/Types/String.h"
#include "Quaternion.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    EE_SYSTEM_API char const* ToString( Axis axis );

    //-------------------------------------------------------------------------

    inline InlineString ToString( Vector const& vector )
    {
        return InlineString( InlineString::CtorSprintf(), "x=%.3f, y=%.3f, z=%.3f", vector.m_x, vector.m_y, vector.m_z );
    }

    inline InlineString ToString( Quaternion const& q )
    {
        EulerAngles const angles = q.ToEulerAngles();
        Float3 const anglesInDegrees = angles.GetAsDegrees();
        return InlineString( InlineString::CtorSprintf(), "x=%.3f, y=%.3f, z=%.3f", anglesInDegrees.m_x, anglesInDegrees.m_y, anglesInDegrees.m_z );
    }
}
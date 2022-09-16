#pragma once
#include "Transform.h"
#include "System/Types/String.h"

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

    inline InlineString ToString( Transform const& t )
    {
        EulerAngles const angles = t.GetRotation().ToEulerAngles();
        Float3 const anglesInDegrees = angles.GetAsDegrees();
        return InlineString( InlineString::CtorSprintf(), "Rx=%.3f, Ry=%.3f, Rz=%.3f\nTx=%.3f, Ty=%.3f, Tz=%.3f\nS=%.3f", anglesInDegrees.m_x, anglesInDegrees.m_y, anglesInDegrees.m_z, t.GetTranslation().m_x, t.GetTranslation().m_y, t.GetTranslation().m_z, t.GetScale() );
    }
}
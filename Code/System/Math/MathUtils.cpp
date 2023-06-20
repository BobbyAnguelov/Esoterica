#include "MathUtils.h"
#include "System/Types/String.h"
#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    char const* ToString( Axis axis )
    {
        static char const* const axesStrings[] =
        {
            "X",
            "Y",
            "Z",
            "-X",
            "-Y",
            "-Z",
        };

        return axesStrings[(int32_t) axis];
    }

    inline InlineString ToString( Vector const& vector )
    {
        return InlineString( InlineString::CtorSprintf(), "x=%.3f, y=%.3f, z=%.3f", vector.GetX(), vector.GetY(), vector.GetZ() );
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
        Float3 const translation = t.GetTranslation().ToFloat3();
        return InlineString( InlineString::CtorSprintf(), "Rx=%.3f, Ry=%.3f, Rz=%.3f\nTx=%.3f, Ty=%.3f, Tz=%.3f\nS=%.3f", anglesInDegrees.m_x, anglesInDegrees.m_y, anglesInDegrees.m_z, translation.m_x, translation.m_y, translation.m_z, t.GetScale() );
    }
}
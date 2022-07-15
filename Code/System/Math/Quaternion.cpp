#include "Quaternion.h"
#include "Matrix.h"

//-------------------------------------------------------------------------

namespace EE
{
    Quaternion const Quaternion::Identity( 0, 0, 0, 1 );

    //-------------------------------------------------------------------------

    // EE Rotation order is XYZ
    EulerAngles Quaternion::ToEulerAngles() const
    {
        return Matrix( *this ).ToEulerAngles();
    }
}
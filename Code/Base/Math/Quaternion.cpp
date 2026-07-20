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

    Quaternion Quaternion::LookAt( Vector const &forwardVector, Vector const &upVector )
    {
        EE_ASSERT( forwardVector.IsNormalized3() );
        EE_ASSERT( upVector.IsNormalized3() );

        Vector const forwardProjectedOnUp = ( forwardVector * forwardVector.Dot3( upVector ) );
        Vector const up = ( upVector - forwardProjectedOnUp ).GetNormalized3();
        Vector const left = up.Cross3( forwardVector ).GetNormalized3();

        Matrix m( left, -forwardVector, up );
        return m.GetRotation();
    }

    Vector Quaternion::GetAxis( Axis axis ) const
    {
        switch ( axis )
        {
            case EE::Axis::X:
            {
                return GetAxisX();
            }
            break;

            case EE::Axis::Y:
            {
                return GetAxisY();
            }
            break;

            case EE::Axis::Z:
            {
                return GetAxisZ();
            }
            break;

            case EE::Axis::NegX:
            {
                return GetAxisX().GetNegated();
            }
            break;

            case EE::Axis::NegY:
            {
                return GetAxisY().GetNegated();
            }
            break;

            case EE::Axis::NegZ:
            {
                return GetAxisZ().GetNegated();
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
                return Vector::Zero;
            }
            break;
        }
    }
}
#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    Transform const Transform::Identity = Transform( Quaternion( 0, 0, 0, 1 ), Vector( 0, 0, 0, 1 ), 1.0f );

    //-------------------------------------------------------------------------

    Vector Transform::GetAxis( Axis axis ) const
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

    void Transform::SanitizeScaleValue()
    {
        if ( Math::IsNearEqual( GetScale(), 1.0f, Math::LargeEpsilon ) )
        {
            SetScale( 1.0f );
        }
    }
}
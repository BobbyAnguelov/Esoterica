#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    Transform const Transform::Identity = Transform( Quaternion( 0, 0, 0, 1 ), Vector( 0, 0, 0, 1 ), 1.0f );

    //-------------------------------------------------------------------------

    void Transform::SanitizeScaleValue()
    {
        if ( Math::IsNearEqual( GetScale(), 1.0f, Math::LargeEpsilon ) )
        {
            SetScale( 1.0f );
        }
    }
}
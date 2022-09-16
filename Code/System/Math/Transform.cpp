#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    Transform const Transform::Identity = Transform( Quaternion( 0, 0, 0, 1 ), Vector( 0, 0, 0, 1 ), 1.0f );

    //-------------------------------------------------------------------------

    void Transform::SanitizeScaleValue()
    {
        float s = m_scale.GetX();
        if ( Math::IsNearEqual( s, 1.0f, Math::LargeEpsilon ) )
        {
            s = 1.0f;
        }
        m_scale = Vector( s, s, s, 0.0f );
    }
}
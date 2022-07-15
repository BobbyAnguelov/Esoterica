#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    Transform const Transform::Identity = Transform( Quaternion( 0, 0, 0, 1 ), Vector( 0, 0, 0, 1 ), Vector( 1, 1, 1, 0 ) );

    //-------------------------------------------------------------------------

    void Transform::SanitizeScaleValues()
    {
        float sx = m_scale.GetX();
        float sy = m_scale.GetY();
        float sz = m_scale.GetZ();

        // Remove variance from values
        //-------------------------------------------------------------------------

        float averageScaleValue = sx + sy + sz;
        averageScaleValue /= 3.0f;

        float maxDeviation = Math::Max( Math::Abs( sx - averageScaleValue ), Math::Abs( sy - averageScaleValue ) );
        maxDeviation = Math::Max( maxDeviation, Math::Abs( sz - averageScaleValue ) );

        if ( maxDeviation < Math::LargeEpsilon )
        {
            sx = sy = sz = averageScaleValue;
        }

        // If nearly 1 - set to exactly 1
        //-------------------------------------------------------------------------

        if ( Math::IsNearEqual( sx, 1.0f, Math::LargeEpsilon ) )
        {
            sx = 1.0f;
        }

        if ( Math::IsNearEqual( sy, 1.0f, Math::LargeEpsilon ) )
        {
            sy = 1.0f;
        }

        if ( Math::IsNearEqual( sz, 1.0f, Math::LargeEpsilon ) )
        {
            sz = 1.0f;
        }

        // Super rough rounding to 4 decimal places
        //-------------------------------------------------------------------------

        sx = Math::Round( sx * 1000 ) / 1000.0f;
        sy = Math::Round( sy * 1000 ) / 1000.0f;
        sz = Math::Round( sz * 1000 ) / 1000.0f;

        //-------------------------------------------------------------------------

        m_scale = Vector( sx, sy, sz, 0.0f );
    }
}
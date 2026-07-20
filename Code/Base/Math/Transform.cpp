#include "Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    Transform const Transform::Identity = Transform( Quaternion( 0, 0, 0, 1 ), Vector( 0, 0, 0, 1 ), 1.0f );
    Transform const Transform::Zero = Transform( Quaternion( 0, 0, 0, 1 ), Vector( 0, 0, 0, 0 ), 0.0f );

    //-------------------------------------------------------------------------

    void Transform::Sanitize()
    {
        // Rotation
        //-------------------------------------------------------------------------

        Float4 F4 = m_rotation.ToFloat4();
        F4.m_x = Math::SanitizeFloat( F4.m_x );
        F4.m_y = Math::SanitizeFloat( F4.m_y );
        F4.m_z = Math::SanitizeFloat( F4.m_z );
        F4.m_w = Math::SanitizeFloat( F4.m_w );

        m_rotation = Quaternion( F4 );
        m_rotation.Normalize();

        // Translation/Scale
        //-------------------------------------------------------------------------

        F4 = m_translationScale.ToFloat4();
        F4.m_x = Math::SanitizeFloat( F4.m_x );
        F4.m_y = Math::SanitizeFloat( F4.m_y );
        F4.m_z = Math::SanitizeFloat( F4.m_z );
        F4.m_w = Math::SanitizeFloat( F4.m_w );

        m_translationScale = Vector( F4 );
    }

    void Transform::SanitizeScaleValue()
    {
        if ( Math::IsNearEqual( GetScale(), 1.0f, Math::LargeEpsilon ) )
        {
            SetScale( 1.0f );
        }
    }
}
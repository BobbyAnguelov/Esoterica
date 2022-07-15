#pragma once

#include "Matrix.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_SYSTEM_API Transform
    {
        EE_SERIALIZE( m_rotation, m_translation, m_scale );

    public:

        static Transform const Identity;

        inline static Transform Lerp( Transform const& from, Transform const& to, float t );
        inline static Transform Slerp( Transform const& from, Transform const& to, float t );
        inline static Transform Delta( Transform const& from, Transform const& to );
        inline static Transform DeltaNoScale( Transform const& from, Transform const& to );

        EE_FORCE_INLINE static Transform FromRotation( Quaternion const& rotation ) { return Transform( rotation ); }
        EE_FORCE_INLINE static Transform FromTranslation( Vector const& translation ) { return Transform( Quaternion::Identity, translation ); }
        EE_FORCE_INLINE static Transform FromScale( Vector const& scale ) { return Transform( Quaternion::Identity, Vector::Zero, scale ); }
        EE_FORCE_INLINE static Transform FromUniformScale( float uniformScale ) { return Transform( Quaternion::Identity, Vector::Zero, Vector( uniformScale, uniformScale, uniformScale, 1.0f ) ); }
        EE_FORCE_INLINE static Transform FromTranslationAndScale( Vector const& translation, Vector const& scale ) { return Transform( Quaternion::Identity, translation, scale ); }
        EE_FORCE_INLINE static Transform FromRotationBetweenVectors( Vector const sourceVector, Vector const targetVector ) { return Transform( Quaternion::FromRotationBetweenNormalizedVectors( sourceVector, targetVector ) ); }

    public:

        Transform() = default;

        explicit Transform( NoInit_t )
            : m_rotation( NoInit )
            , m_translation( NoInit )
            , m_scale( NoInit )
        {}

        explicit inline Transform( Matrix const& m )
        {
            m.Decompose( m_rotation, m_translation, m_scale );
        }

        explicit inline Transform( Quaternion const& rotation, Vector const& translation = Vector( 0, 0, 0, 0 ), Vector const& scale = Vector( 1, 1, 1, 0 ) )
            : m_rotation( rotation )
            , m_translation( translation.GetWithW0() )
            , m_scale( scale.GetWithW0() )
        {}

        explicit inline Transform( AxisAngle const& rotation )
            : m_rotation( rotation )
            , m_translation( 0, 0, 0, 0 )
            , m_scale( 1, 1, 1, 0 )
        {}

        inline Matrix ToMatrix() const { return Matrix( m_rotation, m_translation, m_scale ); }
        inline EulerAngles ToEulerAngles() const { return m_rotation.ToEulerAngles(); }

        //-------------------------------------------------------------------------

        inline Vector GetAxisX() const { return m_rotation.RotateVector( Vector::UnitX ); }
        inline Vector GetAxisY() const { return m_rotation.RotateVector( Vector::UnitY ); }
        inline Vector GetAxisZ() const { return m_rotation.RotateVector( Vector::UnitZ ); }

        inline Vector GetRightVector() const { return m_rotation.RotateVector( Vector::WorldRight ); }
        inline Vector GetForwardVector() const { return m_rotation.RotateVector( Vector::WorldForward ); }
        inline Vector GetUpVector() const { return m_rotation.RotateVector( Vector::WorldUp ); }

        inline bool IsIdentity() const { return m_rotation.IsIdentity() && m_translation.IsZero4() && m_scale.IsEqual3( Vector::One ); }
        inline bool IsRigidTransform() const { return m_scale.IsEqual3( Vector::One ); }
        inline void MakeRigidTransform() { m_scale = Vector::One; }

        inline Transform& Inverse();
        inline Transform GetInverse() const;

        // Rotation
        //-------------------------------------------------------------------------

        inline Quaternion const& GetRotation() const { return m_rotation; }
        inline void SetRotation( Quaternion const& rotation ) { EE_ASSERT( rotation.IsNormalized() ); m_rotation = rotation; }
        inline void AddRotation( Quaternion const& delta ) { m_rotation = delta * m_rotation; }

        // Translation
        //-------------------------------------------------------------------------

        inline Vector const& GetTranslation() const { return m_translation; }
        inline void SetTranslation( Vector const& translation ) { m_translation = translation.GetWithW0(); }
        inline void AddTranslation( Vector const& delta ) { m_translation += delta; }

        // Scale
        //-------------------------------------------------------------------------

        inline Vector const& GetScale() const { return m_scale; }
        inline void SetScale( Vector const& scale ) { m_scale = scale.GetWithW0(); }
        inline void SetScale( float uniformScale ) { m_scale = Vector( uniformScale, uniformScale, uniformScale, 0.0f ); }
        inline bool HasScale() const { return !m_scale.IsEqual3( Vector::One ); }
        inline bool HasNegativeScale() const { return m_scale.IsAnyLessThan( Vector::Zero ); }
        inline bool HasUniformScale() const { return m_scale.m_x == m_scale.m_y && m_scale.m_y == m_scale.m_z; }
        inline bool HasShearOrSkew() const { auto const abs = m_scale.GetAbs(); return abs.m_x != abs.m_y || abs.m_y != abs.m_z; }

        // This function will sanitize the scale values to remove any trailing values from scale factors i.e. 1.000000012 will be converted to 1
        // If values are really close to each other ( 1.0000012, 0.9999996, 1.0 ) this function will create a uniform scale (1, 1, 1)
        // This is primarily needed in import steps where scale values might be sampled from curves or have multiple conversions applied resulting in variance.
        void SanitizeScaleValues();

        // Transformations
        //-------------------------------------------------------------------------

        inline Vector TranslateVector( Vector const& vector ) const { return vector + m_translation; }
        inline Vector ScaleVector( Vector const& vector ) const { return vector * m_scale; }
        inline Vector RotateVector( Vector const& vector ) const { return m_rotation.RotateVector( vector ); }
        inline Vector TransformPoint( Vector const& vector ) const;
        inline Vector TransformPointNoScale( Vector const& vector ) const;
        inline Vector ApplyTransform( Vector const& vector ) const;

        // WARNING: The results from multiplying transforms with shear or skew is ill-defined
        inline Transform operator*( Transform const& rhs ) const;
        inline Transform& operator*=( Transform const& rhs );

        // Operators
        //-------------------------------------------------------------------------

        inline bool operator==( Transform const& rhs ) const
        {
            return m_translation.IsNearEqual3( rhs.m_translation ) && m_scale.IsNearEqual3( rhs.m_scale ) && m_rotation == rhs.m_rotation;
        }

        inline bool operator!=( Transform const& rhs ) const
        {
            return !operator==( rhs);
        }

    private:

        Quaternion  m_rotation = Quaternion( 0, 0, 0, 1 );
        Vector      m_translation = Vector( 0, 0, 0, 0 );
        Vector      m_scale = Vector( 1, 1, 1, 0 );
    };

    //-------------------------------------------------------------------------

    inline Transform& Transform::Inverse()
    {
        Vector inverseScale;

        // Handle zero scale case
        if ( m_scale.IsAnyEqualsZero() )
        {
            inverseScale.m_x = Math::IsNearZero( m_scale.m_x ) ? 0.0f : 1.0f / m_scale.m_x;
            inverseScale.m_y = Math::IsNearZero( m_scale.m_y ) ? 0.0f : 1.0f / m_scale.m_y;
            inverseScale.m_z = Math::IsNearZero( m_scale.m_z ) ? 0.0f : 1.0f / m_scale.m_z;
            inverseScale.m_w = 0.0f;
        }
        else // Just invert
        {
            inverseScale = m_scale.GetInverse();
            inverseScale.SetW0();
        }

        //-------------------------------------------------------------------------

        Quaternion const inverseRotation = m_rotation.GetInverse();
        Vector inverseTranslation = ( inverseRotation.RotateVector( inverseScale * m_translation ) ).GetNegated();
        inverseTranslation.SetW0();

        //-------------------------------------------------------------------------

        m_rotation = inverseRotation;
        m_translation = inverseTranslation;
        m_scale = inverseScale;

        //-------------------------------------------------------------------------

        return *this;
    }

    inline Transform Transform::GetInverse() const
    {
        Transform inverse = *this;
        return inverse.Inverse();
    }

    inline Transform Transform::Lerp( Transform const& from, Transform const& to, float t )
    {
        Quaternion const rotation = Quaternion::NLerp( Quaternion( from.m_rotation ), Quaternion( to.m_rotation ), t );
        Vector const translation = Vector::Lerp( from.m_translation, to.m_translation, t );
        Vector const scale = Vector::Lerp( from.m_scale, to.m_scale, t );
        return Transform( rotation, translation, scale );
    }

    inline Transform Transform::Slerp( Transform const& from, Transform const& to, float t )
    {
        Quaternion const rotation = Quaternion::SLerp( Quaternion( from.m_rotation ), Quaternion( to.m_rotation ), t );
        Vector const translation = Vector::Lerp( Vector( from.m_translation ), Vector( to.m_translation ), t );
        Vector const scale = Vector::Lerp( from.m_scale, to.m_scale, t );
        return Transform( rotation, translation, scale );
    }

    inline Transform Transform::Delta( Transform const& from, Transform const& to )
    {
        EE_ASSERT( from.m_rotation.IsNormalized() && to.m_rotation.IsNormalized() );

        Transform result;

        //-------------------------------------------------------------------------

        Vector inverseScale;

        // Handle zero scale case
        if ( from.m_scale.IsAnyEqualsZero() )
        {
            inverseScale.m_x = Math::IsNearZero( from.m_scale.m_x ) ? 0.0f : 1.0f / from.m_scale.m_x;
            inverseScale.m_y = Math::IsNearZero( from.m_scale.m_y ) ? 0.0f : 1.0f / from.m_scale.m_y;
            inverseScale.m_z = Math::IsNearZero( from.m_scale.m_z ) ? 0.0f : 1.0f / from.m_scale.m_z;
            inverseScale.m_w = 0.0f;
        }
        else // Just invert
        {
            inverseScale = from.m_scale.GetInverse();
            inverseScale.SetW0();
        }

        Vector const deltaScale = to.m_scale * inverseScale;

        //-------------------------------------------------------------------------

        Vector const minScale = Vector::Min( from.m_scale, to.m_scale );
        if ( minScale.IsAnyLessThan( Vector::Zero ) )
        {
            // Multiply the transforms using matrices to get the correct rotation and then remove the scale;
            Matrix const toMtx = to.ToMatrix();
            Matrix const fromMtx = from.ToMatrix();
            Matrix resultMtx = toMtx * fromMtx.GetInverse();
            resultMtx.RemoveScaleFast();

            // Apply back the signs from the final scale
            Vector const sign = deltaScale.GetSign();
            resultMtx[0] *= sign.GetSplatX();
            resultMtx[1] *= sign.GetSplatY();
            resultMtx[2] *= sign.GetSplatZ();

            result.m_rotation = resultMtx.GetRotation();
            EE_ASSERT( result.m_rotation.IsNormalized() );
            result.m_translation = resultMtx.GetTranslation();
            result.m_scale = deltaScale;
        }
        else
        {
            Quaternion const fromInverse = from.m_rotation.GetInverse();
            result.m_rotation = to.m_rotation * fromInverse;
            result.m_translation = fromInverse.RotateVector( to.m_translation - from.m_translation ) * inverseScale;
            result.m_scale = deltaScale;
        }

        //-------------------------------------------------------------------------

        return result;
    }

    inline Transform Transform::DeltaNoScale( Transform const& from, Transform const& to )
    {
        Transform delta;

        Quaternion const inverseFromRotation = from.m_rotation.GetInverse();
        delta.m_rotation = to.m_rotation * inverseFromRotation;

        Vector const deltaTranslation = to.GetTranslation() - from.GetTranslation();
        delta.m_translation = inverseFromRotation.RotateVector( deltaTranslation );

        delta.m_scale = Vector( 1, 1, 1, 0 );
        return delta;
    }

    inline Transform& Transform::operator*=( Transform const& rhs )
    {
        Vector const minScale = Vector::Min( m_scale, rhs.m_scale );
        if ( minScale.IsAnyLessThan( Vector::Zero ) )
        {
            Vector const finalScale = m_scale * rhs.m_scale;

            // Multiply the transforms using matrices to get the correct rotation and then remove the scale;
            Matrix const lhsMtx = ToMatrix();
            Matrix const rhsMtx = rhs.ToMatrix();
            Matrix resultMtx = lhsMtx * rhsMtx;
            resultMtx.RemoveScaleFast();

            // Apply back the signs from the final scale
            Vector const sign = finalScale.GetSign();
            resultMtx[0] *= sign.GetSplatX();
            resultMtx[1] *= sign.GetSplatY();
            resultMtx[2] *= sign.GetSplatZ();

            m_rotation = resultMtx.GetRotation();
            EE_ASSERT( m_rotation.IsNormalized() );
            m_translation = resultMtx.GetTranslation();
            m_scale = finalScale;
        }
        else // Normal case
        {
            m_rotation = m_rotation * rhs.m_rotation;
            m_rotation.Normalize();
            m_translation = rhs.m_rotation.RotateVector( m_translation * rhs.m_scale ) + rhs.m_translation;
            m_scale = m_scale * rhs.m_scale;
        }

        //-------------------------------------------------------------------------

        m_translation.SetW0();

        return *this;
    }

    inline Transform Transform::operator*( Transform const& rhs ) const
    {
        Transform transform = *this;
        transform *= rhs;
        return transform;
    }

    inline Vector Transform::TransformPoint( Vector const& point ) const
    {
        Vector transformedPoint = point * m_scale;
        transformedPoint = m_translation + m_rotation.RotateVector( transformedPoint );
        return transformedPoint;
    }

    inline Vector Transform::TransformPointNoScale( Vector const& point ) const
    {
        Vector transformedPoint = m_translation + m_rotation.RotateVector( point );
        return transformedPoint;
    }

    inline Vector Transform::ApplyTransform( Vector const& vector ) const
    {
        Vector transformedVector = vector * m_scale;
        transformedVector = m_rotation.RotateVector( transformedVector );
        transformedVector += m_translation;
        return transformedVector;
    }
}
#pragma once

#include "Matrix.h"
#include "System/Log.h"

//-------------------------------------------------------------------------
// VQS Transform
//-------------------------------------------------------------------------
// Does NOT support non-uniform scale
// We've kept translation and scale separate for performance reasons (this can be reevaluated later)

namespace EE
{

    class EE_SYSTEM_API Transform
    {
        EE_SERIALIZE( m_rotation, m_translationScale );

    public:

        static Transform const Identity;

        EE_FORCE_INLINE static Transform FromRotation( Quaternion const& rotation ) { return Transform( rotation ); }
        EE_FORCE_INLINE static Transform FromTranslation( Vector const& translation ) { return Transform( Quaternion::Identity, translation ); }
        EE_FORCE_INLINE static Transform FromScale( float uniformScale ) { return Transform( Quaternion::Identity, Vector::Zero, uniformScale ); }
        EE_FORCE_INLINE static Transform FromTranslationAndScale( Vector const& translation, float uniformScale ) { return Transform( Quaternion::Identity, translation, uniformScale ); }
        EE_FORCE_INLINE static Transform FromRotationBetweenVectors( Vector const sourceVector, Vector const targetVector ) { return Transform( Quaternion::FromRotationBetweenNormalizedVectors( sourceVector, targetVector ) ); }

        inline static Transform Lerp( Transform const& from, Transform const& to, float t );
        inline static Transform Slerp( Transform const& from, Transform const& to, float t );

        // Calculate a delta transform that you can concatenate to the 'from' transform to get the 'to' transform. Properly handles the non-uniform scaling case.
        inline static Transform Delta( Transform const& from, Transform const& to );

        // Calculates a delta transform that you can concatenate to the 'from' transform to get the 'to' transform (ignoring scale)
        inline static Transform DeltaNoScale( Transform const& from, Transform const& to );

    public:

        Transform() = default;

        explicit Transform( NoInit_t )
            : m_rotation( NoInit )
            , m_translationScale( NoInit )
        {}

        explicit inline Transform( Matrix const& m )
        {
            Vector mTranslation, mScale;
            m.Decompose( m_rotation, mTranslation, mScale );
            EE_ASSERT( mScale.GetX() == mScale.GetY() && mScale.GetY() == mScale.GetZ() );
            m_translationScale = Vector::Select( mTranslation, mScale, Vector::Select0001 );
        }

        explicit inline Transform( Quaternion const& rotation, Vector const& translation = Vector( 0, 0, 0, 0 ), float scale = 1.0f )
            : m_rotation( rotation )
            , m_translationScale( Vector::Select( translation, Vector( scale ), Vector::Select0001 ) )
        {}

        explicit inline Transform( AxisAngle const& rotation )
            : m_rotation( rotation )
            , m_translationScale( Vector::UnitW )
        {}

        inline Matrix ToMatrix() const { return Matrix( m_rotation, m_translationScale.GetWithW1(), m_translationScale.GetSplatW() ); }

        inline Matrix ToMatrixNoScale() const { return Matrix( m_rotation, m_translationScale.GetWithW1(), Vector::One ); }

        inline EulerAngles ToEulerAngles() const { return m_rotation.ToEulerAngles(); }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Vector GetAxisX() const { return m_rotation.RotateVector( Vector::UnitX ); }
        EE_FORCE_INLINE Vector GetAxisY() const { return m_rotation.RotateVector( Vector::UnitY ); }
        EE_FORCE_INLINE Vector GetAxisZ() const { return m_rotation.RotateVector( Vector::UnitZ ); }

        EE_FORCE_INLINE Vector GetRightVector() const { return m_rotation.RotateVector( Vector::WorldRight ); }
        EE_FORCE_INLINE Vector GetForwardVector() const { return m_rotation.RotateVector( Vector::WorldForward ); }
        EE_FORCE_INLINE Vector GetUpVector() const { return m_rotation.RotateVector( Vector::WorldUp ); }

        EE_FORCE_INLINE bool IsIdentity() const { return m_rotation.IsIdentity() && m_translationScale.IsEqual4( Vector::UnitW ); }
        EE_FORCE_INLINE bool IsRigidTransform() const { return GetScale() == 1.0f; }
        EE_FORCE_INLINE void MakeRigidTransform() { SetScale( 1.0f ); }

        // Inverse and Deltas
        //-------------------------------------------------------------------------

        // Invert this transform.
        // If you want a delta transform that you can concatenate, then you should use the 'Delta' functions
        inline Transform& Inverse();

        // Get the inverse of this transform.
        // If you want a delta transform that you can concatenate, then you should use the 'Delta' functions
        inline Transform GetInverse() const;

        // Return the delta required to a given target transform (i.e., what do we need to add to reach that transform)
        inline Transform GetDeltaToOther( Transform const& targetTransform ) const { return Transform::Delta( *this, targetTransform ); }

        // Return the delta relative from a given a start transform (i.e., how much do we differ from it)
        inline Transform GetDeltaFromOther( Transform const& startTransform ) const { return Transform::Delta( startTransform, *this ); }

        // Rotation
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Quaternion const& GetRotation() const { return m_rotation; }
        EE_FORCE_INLINE void SetRotation( Quaternion const& rotation ) { EE_ASSERT( rotation.IsNormalized() ); m_rotation = rotation; }
        EE_FORCE_INLINE void AddRotation( Quaternion const& delta ) { EE_ASSERT( delta.IsNormalized() ); m_rotation = delta * m_rotation; }

        // Translation
        //-------------------------------------------------------------------------

        // Get the translation for this transform - NOTE: you cannot rely on the W value as that will be the scale
        EE_FORCE_INLINE Vector const& GetTranslation() const { return m_translationScale; }

        // Set the translation
        EE_FORCE_INLINE void SetTranslation( Vector const& newTranslation ) { m_translationScale = Vector::Select( newTranslation, m_translationScale, Vector::Select0001 ); }

        // Add an offset to the current translation
        EE_FORCE_INLINE void AddTranslation( Vector const& translationDelta ) { m_translationScale += translationDelta.GetWithW0(); }

        // Get the translation as a homogeneous coordinates' vector (W=0)
        EE_FORCE_INLINE Vector GetTranslationAsVector() const { return m_translationScale.GetWithW0(); }

        // Get the translation as a homogeneous coordinates' point (W=1)
        EE_FORCE_INLINE Vector GetTranslationAsPoint() const { return m_translationScale.GetWithW1(); }

        // Scale
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE float GetScale() const { return m_translationScale.GetW(); }
        EE_FORCE_INLINE Vector GetScaleVector() const { return m_translationScale.GetSplatW(); }
        EE_FORCE_INLINE Vector GetInverseScaleVector() const { return m_translationScale.GetSplatW().GetInverse(); }
        EE_FORCE_INLINE void SetScale( float uniformScale ) { m_translationScale.SetW( uniformScale ); }
        EE_FORCE_INLINE bool HasScale() const { return m_translationScale.GetW() != 1.0f; }
        EE_FORCE_INLINE bool HasNegativeScale() const { return m_translationScale.GetW() < 0.0f; }

        // This function will sanitize the scale values to remove any trailing values from scale factors i.e. 1.000000012 will be converted to 1
        // This is primarily needed in import steps where scale values might be sampled from curves or have multiple conversions applied resulting in variance.
        void SanitizeScaleValue();

        // Transformations
        //-------------------------------------------------------------------------

        inline Vector TranslateVector( Vector const& vector ) const { return vector + m_translationScale.GetWithW0(); }
        inline Vector ScaleVector( Vector const& vector ) const { return vector * GetScaleVector(); }
        inline Vector TransformPoint( Vector const& vector ) const;
        inline Vector TransformPointNoScale( Vector const& vector ) const;

        // Rotate a vector (same as TransformVectorNoScale)
        inline Vector RotateVector( Vector const& vector ) const { return m_rotation.RotateVector( vector ); }

        // Rotate a vector (same as TransformVectorNoScale)
        EE_FORCE_INLINE Vector TransformNormal( Vector const& vector ) const { return RotateVector( vector ); }

        // Unrotate a vector (same as InverseTransformVectorNoScale)
        inline Vector InverseRotateVector( Vector const& vector ) const { return m_rotation.RotateVectorInverse( vector ); }

        // Invert the operation order when doing inverse transformation: first translation then rotation then scale
        inline Vector InverseTransformPoint( Vector const& point ) const;

        // Invert the operation order when doing inverse transformation: first translation then rotation
        inline Vector InverseTransformPointNoScale( Vector const& point ) const;

        // Applies scale and rotation to a vector (no translation)
        inline Vector TransformVector( Vector const& vector ) const;

        // Rotate a vector
        EE_FORCE_INLINE Vector TransformVectorNoScale( Vector const& vector ) const { return RotateVector( vector ); }

        // Invert the operation order when performing inverse transformation: first rotation then scale
        Vector InverseTransformVector( Vector const& vector ) const;

        // Unrotate a vector
        Vector InverseTransformVectorNoScale( Vector const& vector ) const { return m_rotation.RotateVectorInverse( vector ); }

        // WARNING: The results from multiplying transforms with shear or skew is ill-defined
        inline Transform operator*( Transform const& rhs ) const;

        // WARNING: The results from multiplying transforms with shear or skew is ill-defined
        inline Transform& operator*=( Transform const& rhs );

        // Operators
        //-------------------------------------------------------------------------

        inline bool operator==( Transform const& rhs ) const
        {
            return m_translationScale.IsNearEqual4( rhs.m_translationScale );
        }

        inline bool operator!=( Transform const& rhs ) const
        {
            return !operator==( rhs);
        }

    private:

        Quaternion  m_rotation = Quaternion( 0, 0, 0, 1 );
        Vector      m_translationScale = Vector( 0, 0, 0, 1 );
    };

    //-------------------------------------------------------------------------

    inline Transform& Transform::Inverse()
    {
        EE_ASSERT( !m_translationScale.IsW0() );

        //-------------------------------------------------------------------------

        Quaternion const inverseRotation = m_rotation.GetInverse();
        m_rotation = inverseRotation;

        //-------------------------------------------------------------------------

        Vector const inverseScale = GetInverseScaleVector();
        Vector const inverselyScaledTranslation = inverseScale * m_translationScale.GetWithW0();
        Vector const inverselyRotatedTranslation = inverseRotation.RotateVector( inverselyScaledTranslation );
        Vector const inverseTranslation = inverselyRotatedTranslation.GetNegated().SetW0();

        m_translationScale = Vector::Select( inverseTranslation, inverseScale, Vector::Select0001 );

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
        Vector const translationAndScale = Vector::Lerp( from.m_translationScale, to.m_translationScale, t );

        Transform lerped( NoInit );
        lerped.m_rotation = rotation;
        lerped.m_translationScale = translationAndScale;
        return lerped;
    }

    inline Transform Transform::Slerp( Transform const& from, Transform const& to, float t )
    {
        Quaternion const rotation = Quaternion::SLerp( Quaternion( from.m_rotation ), Quaternion( to.m_rotation ), t );
        Vector const translationAndScale = Vector::Lerp( Vector( from.m_translationScale ), Vector( to.m_translationScale ), t );

        Transform lerped( NoInit );
        lerped.m_rotation = rotation;
        lerped.m_translationScale = translationAndScale;
        return lerped;
    }

    inline Transform Transform::Delta( Transform const& from, Transform const& to )
    {
        EE_ASSERT( from.m_rotation.IsNormalized() && to.m_rotation.IsNormalized() );
        EE_ASSERT( !from.m_translationScale.IsW0() && !to.m_translationScale.IsW0() );

        Transform result;

        //-------------------------------------------------------------------------

        Vector const inverseScale = from.GetInverseScaleVector();
        Vector const deltaScale = to.GetScaleVector() * inverseScale;

        //-------------------------------------------------------------------------

        // If we have negative scaling, we need to use matrices to calculate the deltas
        Vector const minScale = Vector::Min( from.m_translationScale.GetSplatW(), to.m_translationScale.GetSplatW() );
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
            result.m_translationScale = Vector::Select( resultMtx.GetTranslation(), deltaScale, Vector::Select0001 );
        }
        else
        {
            Quaternion const fromInverseRotation = from.m_rotation.GetInverse();
            result.m_rotation = to.m_rotation * fromInverseRotation;

            Vector const deltaTranslation = to.m_translationScale - from.m_translationScale;
            Vector const translation = fromInverseRotation.RotateVector( deltaTranslation ) * inverseScale;
            result.m_translationScale = Vector::Select( translation, deltaScale, Vector::Select0001 );
        }

        //-------------------------------------------------------------------------

        return result;
    }

    inline Transform Transform::DeltaNoScale( Transform const& from, Transform const& to )
    {
        Quaternion const inverseFromRotation = from.m_rotation.GetInverse();
        Vector const deltaTranslation = to.GetTranslation() - from.GetTranslation();

        Transform delta;
        delta.m_rotation = to.m_rotation * inverseFromRotation;
        delta.m_translationScale = inverseFromRotation.RotateVector( deltaTranslation ).GetWithW1();
        return delta;
    }

    inline Transform& Transform::operator*=( Transform const& rhs )
    {
        Vector const scale = GetScaleVector();
        Vector const rhsScale = rhs.GetScaleVector();
        Vector const minScale = Vector::Min( scale, rhsScale );
        Vector const finalScale = scale * rhsScale;

        if ( minScale.IsAnyLessThan( Vector::Zero ) )
        {
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
            m_translationScale = Vector::Select( resultMtx.GetTranslation(), finalScale, Vector::Select0001 );
        }
        else // Normal case
        {
            m_rotation = m_rotation * rhs.m_rotation;
            m_rotation.Normalize();
            Vector const translation = rhs.m_rotation.RotateVector( m_translationScale * rhsScale ) + rhs.m_translationScale;
            m_translationScale = Vector::Select( translation, finalScale, Vector::Select0001 );
        }

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
        EE_ASSERT( !m_translationScale.IsW0() );
        Vector transformedPoint = point * m_translationScale.GetSplatW();
        transformedPoint = ( m_translationScale + m_rotation.RotateVector( transformedPoint ) ).GetWithW0();
        return transformedPoint;
    }

    inline Vector Transform::TransformPointNoScale( Vector const& point ) const
    {
        Vector transformedPoint = ( m_translationScale + m_rotation.RotateVector( point ) ).GetWithW0();;
        return transformedPoint;
    }

    inline Vector Transform::InverseTransformPoint( Vector const& point ) const
    {
        EE_ASSERT( !m_translationScale.IsW0() );
        Vector const shiftedPoint = point - m_translationScale;
        Vector const unrotatedShiftedPoint = m_rotation.RotateVectorInverse( shiftedPoint );
        Vector const inverseScale = GetInverseScaleVector();
        Vector const result = unrotatedShiftedPoint * inverseScale;
        return result;
    }

    inline Vector Transform::InverseTransformPointNoScale( Vector const& point ) const
    {
        Vector const shiftedPoint = point - m_translationScale;
        Vector const unrotatedShiftedPoint = m_rotation.RotateVectorInverse( shiftedPoint );
        return unrotatedShiftedPoint;
    }

    inline Vector Transform::TransformVector( Vector const& vector ) const
    {
        EE_ASSERT( !m_translationScale.IsW0() );
        Vector transformedVector = vector * GetScaleVector();
        transformedVector = m_rotation.RotateVector( transformedVector );
        return transformedVector;
    }

    inline Vector Transform::InverseTransformVector( Vector const& vector ) const
    {
        EE_ASSERT( !m_translationScale.IsW0() );
        Vector const unrotatedVector = m_rotation.RotateVectorInverse( vector );
        Vector const inverseScale = GetInverseScaleVector();
        Vector const result = unrotatedVector * inverseScale;
        return result;
    }
}
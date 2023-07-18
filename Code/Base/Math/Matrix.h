#pragma once

#include "Vector.h"
#include "Quaternion.h"

//-------------------------------------------------------------------------
// This math library is heavily based on the DirectX math library
//-------------------------------------------------------------------------
// https://github.com/Microsoft/DirectXMath
//-------------------------------------------------------------------------

namespace EE
{
    enum class CoordinateSpace : uint8_t
    {
        World,
        Local,
    };

    // Notes:
    //-------------------------------------------------------------------------
    // * EE Matrices are row-major
    // * Multiplication order is right to left -> ObjectWorldTransform = LocalObjectTransform * WorldTransform
    //-------------------------------------------------------------------------

    class EE_BASE_API alignas( 16 ) Matrix
    {
        EE_SERIALIZE( m_rows );

    public:

        static Matrix const Identity;

    public:

        inline static Matrix FromRotation( Quaternion const& rotation ) { return Matrix( rotation ); }
        inline static Matrix FromTranslation( Vector const& translation );
        inline static Matrix FromScale( Vector const& scale );
        inline static Matrix FromUniformScale( float uniformScale );
        inline static Matrix FromTranslationAndScale( Vector const& translation, Vector const& scale );
        inline static Matrix FromRotationBetweenVectors( Vector const sourceVector, Vector const targetVector ) { return Matrix( Quaternion::FromRotationBetweenNormalizedVectors( sourceVector, targetVector ) ); }

    public:

        explicit Matrix() { memcpy( this, &Matrix::Identity, sizeof( Matrix ) ); }
        explicit Matrix( NoInit_t ) {}
        explicit Matrix( ZeroInit_t ) { memset( this, 0, sizeof( Matrix ) ); }
        explicit Matrix( float v00, float v01, float v02, float v03, float v10, float v11, float v12, float v13, float v20, float v21, float v22, float v23, float v30, float v31, float v32, float v33 );
        explicit Matrix( float values[16] );
        explicit Matrix( Vector const& xAxis, Vector const& yAxis, Vector const& zAxis );
        explicit Matrix( Vector const& xAxis, Vector const& yAxis, Vector const& zAxis, Vector const& translation );

        inline Matrix( Vector const axis, Radians angleRadians );
        inline Matrix( AxisAngle const axisAngle ) : Matrix( Vector( axisAngle.m_axis ), axisAngle.m_angle ) {}

        explicit Matrix( Quaternion const& rotation );
        explicit Matrix( Quaternion const& rotation, Vector const& translation, Vector const& scale = Vector::One );
        explicit Matrix( Quaternion const& rotation, Vector const& translation, float scale = 1.0f ) : Matrix( rotation, translation, Vector(scale) ) {}
        explicit Matrix( EulerAngles const& eulerAngles, Vector const translation = Vector::UnitW );

        EulerAngles ToEulerAngles() const;

        inline float* AsFloatArray() { return &m_values[0][0]; }
        inline float const* AsFloatArray() const { return &m_values[0][0]; }
        inline Vector const& GetRow( uint32_t row ) const { return m_rows[row]; }

        inline Vector const& GetAxisX() const { return m_rows[0]; }
        inline Vector const& GetAxisY() const { return m_rows[1]; }
        inline Vector const& GetAxisZ() const { return m_rows[2]; }

        // Get the world forward vector (Note: this is not valid for camera transforms!)
        EE_FORCE_INLINE Float3 GetForwardVector() const { return GetAxisY().GetNegated(); }

        // Get the world right vector (Note: this is not valid for camera transforms!)
        EE_FORCE_INLINE Float3 GetRightVector() const { return GetAxisX().GetNegated(); }

        // Get the world up vector (Note: this is not valid for camera transforms!)
        EE_FORCE_INLINE Float3 GetUpVector() const { return GetAxisZ(); }

        inline Vector GetUnitAxisX() const { return m_rows[0].GetNormalized3(); }
        inline Vector GetUnitAxisY() const { return m_rows[1].GetNormalized3(); }
        inline Vector GetUnitAxisZ() const { return m_rows[2].GetNormalized3(); }

        inline bool IsIdentity() const;
        inline bool IsOrthogonal() const;
        inline bool IsOrthonormal() const;

        bool Decompose( Quaternion& outRotation, Vector& outTranslation, Vector& outScale ) const;

        //-------------------------------------------------------------------------

        inline Matrix& Transpose();
        inline Matrix GetTransposed() const;

        inline Matrix& Invert();
        inline Matrix GetInverse() const;

        inline Vector GetDeterminant() const;
        inline float GetDeterminantAsFloat() const { return GetDeterminant().GetX(); }

        // Translation
        //-------------------------------------------------------------------------

        inline Vector GetTranslation() const { return m_rows[3].GetWithW0(); }
        inline Vector const& GetTranslationWithW() const { return m_rows[3]; }
        inline Matrix& SetTranslation( Vector const& v ) { m_rows[3] = v.GetWithW1(); return *this; }
        inline Matrix& SetTranslation( Float3 const& v ) { m_rows[3] = Vector( v, 1.0f ); return *this; }
        inline Matrix& SetTranslation( Float4 const& v ) { m_rows[3] = Vector( v.m_x, v.m_y, v.m_z, 1.0f ); return *this; }

        // Rotation
        //-------------------------------------------------------------------------

        inline Quaternion GetRotation() const;

        // These function set the rotation portion of the matrix but will destroy the scale features
        inline Matrix& SetRotation( Matrix const& rotation );
        inline Matrix& SetRotation( Quaternion const& rotation );

        // These functions will set the rotation portion of the matrix and will keep the scale components
        inline Matrix& SetRotationMaintainingScale( Matrix const& rotation );
        inline Matrix& SetRotationMaintainingScale( Quaternion const& rotation );

        // Scale
        //-------------------------------------------------------------------------

        Vector GetScale() const;

        // These functions perform a full decomposition to extract the scale/shear from the matrix
        Matrix& RemoveScale();
        Matrix& SetScale( Vector const& scale );
        inline Matrix& SetScale( float uniformScale ) { SetScale( Vector( uniformScale ) ); return *this; }

        // These are naive and fast versions of the above functions (simply normalization and multiplication of the 3x3 rotation part)
        inline Matrix& RemoveScaleFast();
        inline Matrix& SetScaleFast( Vector const& scale );
        inline Matrix& SetScaleFast( float uniformScale ) { SetScaleFast( Vector( uniformScale ) ); return *this; }

        // Operators
        //-------------------------------------------------------------------------

        // Applies rotation and scale to a vector and returns a result with the W = 0
        inline Vector RotateVector( Vector const& vector ) const;

        // Applies rotation and scale to a vector and returns a result with the W = 0
        EE_FORCE_INLINE Vector TransformNormal( Vector const& vector ) const { return RotateVector( vector ); }

        // Applies the transformation to a given point and ensures the resulting W = 1
        inline Vector TransformPoint( Vector const& point ) const;

        // Applies the transformation to a vector ignoring the W value. Same as TransformPoint with the result W left unchanged
        inline Vector TransformVector3( Vector const& vector ) const;

        // Applies the transformation to a given vector with the result W left unchanged
        inline Vector TransformVector4( Vector const& vector ) const;

        Vector& operator[]( uint32_t i ) { EE_ASSERT( i < 4 ); return m_rows[i]; }
        Vector const operator[]( uint32_t i ) const { EE_ASSERT( i < 4 ); return m_rows[i]; }

        inline Matrix operator*( Matrix const& rhs ) const;
        inline Matrix& operator*=( Matrix const& rhs );

        inline Matrix operator*( Quaternion const& rhs ) const { return operator*( Matrix( rhs ) ); }
        inline Matrix operator*=( Quaternion const& rhs ) { return operator*=( Matrix( rhs ) ); }

        // Naive equality operator
        inline bool operator==( Matrix const& rhs ) const
        {
            for ( auto i = 0; i < 4; i++ )
            {
                for ( auto j = 0; j < 4; j++ )
                {
                    if ( m_values[i][j] != rhs.m_values[i][j] )
                    {
                        return false;
                    }
                }
            }

            return true;
        }

    public:

        union
        {
            Vector      m_rows[4];
            float       m_values[4][4];
        };
    };

    //-------------------------------------------------------------------------

    inline Matrix::Matrix( Quaternion const& rotation )
    {
        SetRotation( rotation );
        m_rows[3] = Vector::UnitW;
    }

    inline Matrix::Matrix( Quaternion const& rotation, Vector const& translation, Vector const& scale )
    {
        SetRotation( rotation );
        m_rows[0] = m_rows[0] * scale.GetSplatX();
        m_rows[1] = m_rows[1] * scale.GetSplatY();
        m_rows[2] = m_rows[2] * scale.GetSplatZ();
        m_rows[3] = translation.GetWithW1();
    }

    //-------------------------------------------------------------------------

    Matrix& Matrix::SetScaleFast( Vector const& scale )
    {
        m_rows[0] = m_rows[0].GetNormalized3() * scale.GetSplatX();
        m_rows[1] = m_rows[1].GetNormalized3() * scale.GetSplatY();
        m_rows[2] = m_rows[2].GetNormalized3() * scale.GetSplatZ();
        return *this;
    }

    Matrix& Matrix::RemoveScaleFast()
    {
        m_rows[0] = m_rows[0].GetNormalized4();
        m_rows[1] = m_rows[1].GetNormalized4();
        m_rows[2] = m_rows[2].GetNormalized4();
        return *this;
    }

    inline Matrix& Matrix::Transpose()
    {
        __m128 vTemp1 = _mm_shuffle_ps( m_rows[0], m_rows[1], _MM_SHUFFLE( 1, 0, 1, 0 ) );
        __m128 vTemp3 = _mm_shuffle_ps( m_rows[0], m_rows[1], _MM_SHUFFLE( 3, 2, 3, 2 ) );
        __m128 vTemp2 = _mm_shuffle_ps( m_rows[2], m_rows[3], _MM_SHUFFLE( 1, 0, 1, 0 ) );
        __m128 vTemp4 = _mm_shuffle_ps( m_rows[2], m_rows[3], _MM_SHUFFLE( 3, 2, 3, 2 ) );
        m_rows[0] = _mm_shuffle_ps( vTemp1, vTemp2, _MM_SHUFFLE( 2, 0, 2, 0 ) );
        m_rows[1] = _mm_shuffle_ps( vTemp1, vTemp2, _MM_SHUFFLE( 3, 1, 3, 1 ) );
        m_rows[2] = _mm_shuffle_ps( vTemp3, vTemp4, _MM_SHUFFLE( 2, 0, 2, 0 ) );
        m_rows[3] = _mm_shuffle_ps( vTemp3, vTemp4, _MM_SHUFFLE( 3, 1, 3, 1 ) );
        return *this;
    }

    inline Matrix Matrix::GetTransposed() const
    {
        Matrix m = *this;
        m.Transpose();
        return m;
    }

    inline Matrix& Matrix::Invert()
    {
        Matrix MT = GetTransposed();
        __m128 V00 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[2], _MM_SHUFFLE( 1, 1, 0, 0 ) );
        __m128 V10 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[3], _MM_SHUFFLE( 3, 2, 3, 2 ) );
        __m128 V01 = _mm_shuffle_ps( MT.m_rows[0], MT.m_rows[0], _MM_SHUFFLE( 1, 1, 0, 0 ) );
        __m128 V11 = _mm_shuffle_ps( MT.m_rows[1], MT.m_rows[1], _MM_SHUFFLE( 3, 2, 3, 2 ) );
        __m128 V02 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[0], _MM_SHUFFLE( 2, 0, 2, 0 ) );
        __m128 V12 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[1], _MM_SHUFFLE( 3, 1, 3, 1 ) );

        __m128 D0 = _mm_mul_ps( V00, V10 );
        __m128 D1 = _mm_mul_ps( V01, V11 );
        __m128 D2 = _mm_mul_ps( V02, V12 );

        V00 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[2], _MM_SHUFFLE( 3, 2, 3, 2 ) );
        V10 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[3], _MM_SHUFFLE( 1, 1, 0, 0 ) );
        V01 = _mm_shuffle_ps( MT.m_rows[0], MT.m_rows[0], _MM_SHUFFLE( 3, 2, 3, 2 ) );
        V11 = _mm_shuffle_ps( MT.m_rows[1], MT.m_rows[1], _MM_SHUFFLE( 1, 1, 0, 0 ) );
        V02 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[0], _MM_SHUFFLE( 3, 1, 3, 1 ) );
        V12 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[1], _MM_SHUFFLE( 2, 0, 2, 0 ) );

        V00 = _mm_mul_ps( V00, V10 );
        V01 = _mm_mul_ps( V01, V11 );
        V02 = _mm_mul_ps( V02, V12 );
        D0 = _mm_sub_ps( D0, V00 );
        D1 = _mm_sub_ps( D1, V01 );
        D2 = _mm_sub_ps( D2, V02 );
        // V11 = D0Y,D0W,D2Y,D2Y
        V11 = _mm_shuffle_ps( D0, D2, _MM_SHUFFLE( 1, 1, 3, 1 ) );
        V00 = _mm_shuffle_ps( MT.m_rows[1], MT.m_rows[1], _MM_SHUFFLE( 1, 0, 2, 1 ) );
        V10 = _mm_shuffle_ps( V11, D0, _MM_SHUFFLE( 0, 3, 0, 2 ) );
        V01 = _mm_shuffle_ps( MT.m_rows[0], MT.m_rows[0], _MM_SHUFFLE( 0, 1, 0, 2 ) );
        V11 = _mm_shuffle_ps( V11, D0, _MM_SHUFFLE( 2, 1, 2, 1 ) );
        // V13 = D1Y,D1W,D2W,D2W
        __m128 V13 = _mm_shuffle_ps( D1, D2, _MM_SHUFFLE( 3, 3, 3, 1 ) );
        V02 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[3], _MM_SHUFFLE( 1, 0, 2, 1 ) );
        V12 = _mm_shuffle_ps( V13, D1, _MM_SHUFFLE( 0, 3, 0, 2 ) );
        __m128 V03 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[2], _MM_SHUFFLE( 0, 1, 0, 2 ) );
        V13 = _mm_shuffle_ps( V13, D1, _MM_SHUFFLE( 2, 1, 2, 1 ) );

        __m128 C0 = _mm_mul_ps( V00, V10 );
        __m128 C2 = _mm_mul_ps( V01, V11 );
        __m128 C4 = _mm_mul_ps( V02, V12 );
        __m128 C6 = _mm_mul_ps( V03, V13 );

        // V11 = D0X,D0Y,D2X,D2X
        V11 = _mm_shuffle_ps( D0, D2, _MM_SHUFFLE( 0, 0, 1, 0 ) );
        V00 = _mm_shuffle_ps( MT.m_rows[1], MT.m_rows[1], _MM_SHUFFLE( 2, 1, 3, 2 ) );
        V10 = _mm_shuffle_ps( D0, V11, _MM_SHUFFLE( 2, 1, 0, 3 ) );
        V01 = _mm_shuffle_ps( MT.m_rows[0], MT.m_rows[0], _MM_SHUFFLE( 1, 3, 2, 3 ) );
        V11 = _mm_shuffle_ps( D0, V11, _MM_SHUFFLE( 0, 2, 1, 2 ) );
        // V13 = D1X,D1Y,D2Z,D2Z
        V13 = _mm_shuffle_ps( D1, D2, _MM_SHUFFLE( 2, 2, 1, 0 ) );
        V02 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[3], _MM_SHUFFLE( 2, 1, 3, 2 ) );
        V12 = _mm_shuffle_ps( D1, V13, _MM_SHUFFLE( 2, 1, 0, 3 ) );
        V03 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[2], _MM_SHUFFLE( 1, 3, 2, 3 ) );
        V13 = _mm_shuffle_ps( D1, V13, _MM_SHUFFLE( 0, 2, 1, 2 ) );

        V00 = _mm_mul_ps( V00, V10 );
        V01 = _mm_mul_ps( V01, V11 );
        V02 = _mm_mul_ps( V02, V12 );
        V03 = _mm_mul_ps( V03, V13 );
        C0 = _mm_sub_ps( C0, V00 );
        C2 = _mm_sub_ps( C2, V01 );
        C4 = _mm_sub_ps( C4, V02 );
        C6 = _mm_sub_ps( C6, V03 );

        V00 = _mm_shuffle_ps( MT.m_rows[1], MT.m_rows[1], _MM_SHUFFLE( 0, 3, 0, 3 ) );
        // V10 = D0Z,D0Z,D2X,D2Y
        V10 = _mm_shuffle_ps( D0, D2, _MM_SHUFFLE( 1, 0, 2, 2 ) );
        V10 = _mm_shuffle_ps( V10, V10, _MM_SHUFFLE( 0, 2, 3, 0 ) );
        V01 = _mm_shuffle_ps( MT.m_rows[0], MT.m_rows[0], _MM_SHUFFLE( 2, 0, 3, 1 ) );
        // V11 = D0X,D0W,D2X,D2Y
        V11 = _mm_shuffle_ps( D0, D2, _MM_SHUFFLE( 1, 0, 3, 0 ) );
        V11 = _mm_shuffle_ps( V11, V11, _MM_SHUFFLE( 2, 1, 0, 3 ) );
        V02 = _mm_shuffle_ps( MT.m_rows[3], MT.m_rows[3], _MM_SHUFFLE( 0, 3, 0, 3 ) );
        // V12 = D1Z,D1Z,D2Z,D2W
        V12 = _mm_shuffle_ps( D1, D2, _MM_SHUFFLE( 3, 2, 2, 2 ) );
        V12 = _mm_shuffle_ps( V12, V12, _MM_SHUFFLE( 0, 2, 3, 0 ) );
        V03 = _mm_shuffle_ps( MT.m_rows[2], MT.m_rows[2], _MM_SHUFFLE( 2, 0, 3, 1 ) );
        // V13 = D1X,D1W,D2Z,D2W
        V13 = _mm_shuffle_ps( D1, D2, _MM_SHUFFLE( 3, 2, 3, 0 ) );
        V13 = _mm_shuffle_ps( V13, V13, _MM_SHUFFLE( 2, 1, 0, 3 ) );

        V00 = _mm_mul_ps( V00, V10 );
        V01 = _mm_mul_ps( V01, V11 );
        V02 = _mm_mul_ps( V02, V12 );
        V03 = _mm_mul_ps( V03, V13 );
        __m128 C1 = _mm_sub_ps( C0, V00 );
        C0 = _mm_add_ps( C0, V00 );
        __m128 C3 = _mm_add_ps( C2, V01 );
        C2 = _mm_sub_ps( C2, V01 );
        __m128 C5 = _mm_sub_ps( C4, V02 );
        C4 = _mm_add_ps( C4, V02 );
        __m128 C7 = _mm_add_ps( C6, V03 );
        C6 = _mm_sub_ps( C6, V03 );

        C0 = _mm_shuffle_ps( C0, C1, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C2 = _mm_shuffle_ps( C2, C3, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C4 = _mm_shuffle_ps( C4, C5, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C6 = _mm_shuffle_ps( C6, C7, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C0 = _mm_shuffle_ps( C0, C0, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C2 = _mm_shuffle_ps( C2, C2, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C4 = _mm_shuffle_ps( C4, C4, _MM_SHUFFLE( 3, 1, 2, 0 ) );
        C6 = _mm_shuffle_ps( C6, C6, _MM_SHUFFLE( 3, 1, 2, 0 ) );

        __m128 vTemp = Vector::Dot4( C0, MT.m_rows[0] );
        vTemp = _mm_div_ps( Vector::One, vTemp );
        m_rows[0] = _mm_mul_ps( C0, vTemp );
        m_rows[1] = _mm_mul_ps( C2, vTemp );
        m_rows[2] = _mm_mul_ps( C4, vTemp );
        m_rows[3] = _mm_mul_ps( C6, vTemp );
        return *this;
    }

    inline Matrix Matrix::GetInverse() const
    {
        Matrix m = *this;
        m.Invert();
        return m;
    }

    inline Matrix& Matrix::SetRotation( Matrix const& rotation )
    {
        EE_ASSERT( Math::Abs( rotation.GetDeterminant().GetX() ) == 1.0f );
        m_rows[0] = rotation.m_rows[0];
        m_rows[1] = rotation.m_rows[1];
        m_rows[2] = rotation.m_rows[2];
        return *this;
    }

    inline Matrix& Matrix::SetRotation( Quaternion const& rotation )
    {
        static __m128 const constant1110 = { 1.0f, 1.0f, 1.0f, 0.0f };

        __m128 Q0 = _mm_add_ps( rotation, rotation );
        __m128 Q1 = _mm_mul_ps( rotation, Q0 );

        __m128 V0 = _mm_shuffle_ps( Q1, Q1, _MM_SHUFFLE( 3, 0, 0, 1 ) );
        V0 = _mm_and_ps( V0, SIMD::g_maskXYZ0 );
        __m128 V1 = _mm_shuffle_ps( Q1, Q1, _MM_SHUFFLE( 3, 1, 2, 2 ) );
        V1 = _mm_and_ps( V1, SIMD::g_maskXYZ0 );
        __m128 R0 = _mm_sub_ps( constant1110, V0 );
        R0 = _mm_sub_ps( R0, V1 );

        V0 = _mm_shuffle_ps( rotation, rotation, _MM_SHUFFLE( 3, 1, 0, 0 ) );
        V1 = _mm_shuffle_ps( Q0, Q0, _MM_SHUFFLE( 3, 2, 1, 2 ) );
        V0 = _mm_mul_ps( V0, V1 );

        V1 = _mm_shuffle_ps( rotation, rotation, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        __m128 V2 = _mm_shuffle_ps( Q0, Q0, _MM_SHUFFLE( 3, 0, 2, 1 ) );
        V1 = _mm_mul_ps( V1, V2 );

        __m128 R1 = _mm_add_ps( V0, V1 );
        __m128 R2 = _mm_sub_ps( V0, V1 );

        V0 = _mm_shuffle_ps( R1, R2, _MM_SHUFFLE( 1, 0, 2, 1 ) );
        V0 = _mm_shuffle_ps( V0, V0, _MM_SHUFFLE( 1, 3, 2, 0 ) );
        V1 = _mm_shuffle_ps( R1, R2, _MM_SHUFFLE( 2, 2, 0, 0 ) );
        V1 = _mm_shuffle_ps( V1, V1, _MM_SHUFFLE( 2, 0, 2, 0 ) );

        Q1 = _mm_shuffle_ps( R0, V0, _MM_SHUFFLE( 1, 0, 3, 0 ) );
        Q1 = _mm_shuffle_ps( Q1, Q1, _MM_SHUFFLE( 1, 3, 2, 0 ) );

        m_rows[0] = Q1;

        Q1 = _mm_shuffle_ps( R0, V0, _MM_SHUFFLE( 3, 2, 3, 1 ) );
        Q1 = _mm_shuffle_ps( Q1, Q1, _MM_SHUFFLE( 1, 3, 0, 2 ) );
        m_rows[1] = Q1;

        Q1 = _mm_shuffle_ps( V1, R0, _MM_SHUFFLE( 3, 2, 1, 0 ) );
        m_rows[2] = Q1;
        return *this;
    }

    inline Matrix& Matrix::SetRotationMaintainingScale( Matrix const& rotation )
    {
        Vector const scale = GetScale();
        SetRotation( rotation );
        return SetScale( scale );
    }

    inline Matrix& Matrix::SetRotationMaintainingScale( Quaternion const& rotation )
    {
        Vector const scale = GetScale();
        SetRotation( rotation );
        return SetScale( scale );
    }

    inline Quaternion Matrix::GetRotation() const
    {
        // based on RTM: https://github.com/nfrechette/rtm

        //-------------------------------------------------------------------------

        Vector const& axisX = m_rows[0];
        Vector const& axisY = m_rows[1];
        Vector const& axisZ = m_rows[2];

        // Zero scale is not supported
        if ( axisX.IsNearZero4() || axisY.IsNearZero4() || axisZ.IsNearZero4() )
        {
            EE_HALT();
        }

        float const axisX_X = axisX.GetX();
        float const axisY_Y = axisY.GetY();
        float const axisZ_Z = axisZ.GetZ();

        float const mtx_trace = axisX_X + axisY_Y + axisZ_Z;
        if ( mtx_trace > 0.0 )
        {
            float const axisX_y = axisX.GetY();
            float const axisX_z = axisX.GetZ();

            float const axisY_x = axisY.GetX();
            float const axisY_z = axisY.GetZ();

            float const axisZ_x = axisZ.GetX();
            float const axisZ_y = axisZ.GetY();

            float const inv_trace = Math::Reciprocal( Math::Sqrt( mtx_trace + 1.0f ) );
            float const half_inv_trace = inv_trace * 0.5f;

            float const m_x = ( axisY_z - axisZ_y ) * half_inv_trace;
            float const m_y = ( axisZ_x - axisX_z ) * half_inv_trace;
            float const m_z = ( axisX_y - axisY_x ) * half_inv_trace;
            float const m_w = Math::Reciprocal( inv_trace ) * 0.5f;

            return Quaternion( m_x, m_y, m_z, m_w ).GetNormalized();
        }
        else
        {
            // Find the axis with the highest diagonal value
            int32_t axisIdx0 = 0;
            if ( axisY_Y > axisX_X )
            {
                axisIdx0 = 1;
            }

            if ( axisZ_Z > m_rows[axisIdx0][axisIdx0] )
            {
                axisIdx0 = 2;
            }

            int32_t const axisIdx1 = ( axisIdx0 + 1 ) % 3;
            int32_t const axisIdx2 = ( axisIdx1 + 1 ) % 3;

            float const pseudoTrace = 1.0f + m_rows[axisIdx0][axisIdx0] - m_rows[axisIdx1][axisIdx1] - m_rows[axisIdx2][axisIdx2];
            float const inversePseudoTrace = Math::Reciprocal( Math::Sqrt( pseudoTrace ) );
            float const halfInversePseudoTrace = inversePseudoTrace * 0.5f;

            //-------------------------------------------------------------------------

            Float4 rawQuatValues;
            rawQuatValues[axisIdx0] = Math::Reciprocal( inversePseudoTrace ) * 0.5f;
            rawQuatValues[axisIdx1] = halfInversePseudoTrace * ( m_rows[axisIdx0][axisIdx1] + m_rows[axisIdx1][axisIdx0] );
            rawQuatValues[axisIdx2] = halfInversePseudoTrace * ( m_rows[axisIdx0][axisIdx2] + m_rows[axisIdx2][axisIdx0] );
            rawQuatValues[3] = halfInversePseudoTrace * ( m_rows[axisIdx1][axisIdx2] - m_rows[axisIdx2][axisIdx1] );
            return Quaternion( rawQuatValues ).GetNormalized();
        }
    }

    inline Vector Matrix::GetDeterminant() const
    {
        Vector V0 = m_rows[2].Shuffle( 1, 0, 0, 0 );
        Vector V1 = m_rows[3].Shuffle( 2, 2, 1, 1 );
        Vector V2 = m_rows[2].Shuffle( 1, 0, 0, 0 );
        Vector V3 = m_rows[3].Shuffle( 3, 3, 3, 2 );
        Vector V4 = m_rows[2].Shuffle( 2, 2, 1, 1 );
        Vector V5 = m_rows[3].Shuffle( 3, 3, 3, 2 );

        Vector P0 = V0 * V1;
        Vector P1 = V2 * V3;
        Vector P2 = V4 * V5;

        V0 = m_rows[2].Shuffle( 2, 2, 1, 1 );
        V1 = m_rows[3].Shuffle( 1, 0, 0, 0 );
        V2 = m_rows[2].Shuffle( 3, 3, 3, 2 );
        V3 = m_rows[3].Shuffle( 1, 0, 0, 0 );
        V4 = m_rows[2].Shuffle( 3, 3, 3, 2 );
        V5 = m_rows[3].Shuffle( 2, 2, 1, 1 );

        P0 = Vector::NegativeMultiplySubtract( V0, V1, P0 );
        P1 = Vector::NegativeMultiplySubtract( V2, V3, P1 );
        P2 = Vector::NegativeMultiplySubtract( V4, V5, P2 );

        V0 = m_rows[1].Shuffle( 3, 3, 3, 2 );
        V1 = m_rows[1].Shuffle( 2, 2, 1, 1 );
        V2 = m_rows[1].Shuffle( 1, 0, 0, 0 );

        static Vector const Sign( 1.0f, -1.0f, 1.0f, -1.0f );
        Vector S = m_rows[0] * Sign;
        Vector R = V0 * P0;
        R = Vector::NegativeMultiplySubtract( V1, P1, R );
        R = Vector::MultiplyAdd( V2, P2, R );

        return Vector::Dot4( S, R );
    }

    //-------------------------------------------------------------------------

    inline bool Matrix::IsOrthonormal() const
    {
        static const Vector three( 3 );
        auto dotCheck = Vector::Dot3( m_rows[0], m_rows[1] ) + Vector::Dot3( m_rows[0], m_rows[2] ) + Vector::Dot3( m_rows[1], m_rows[2] );
        auto magnitudeCheck = m_rows[0].LengthSquared3() + m_rows[1].LengthSquared3() + m_rows[2].LengthSquared3();
        auto result = dotCheck + magnitudeCheck;
        return result.IsNearEqual3( three );
    }

    inline bool Matrix::IsOrthogonal() const
    {
        Matrix const transpose = GetTransposed();
        Matrix result = *this * transpose;
        return result.IsIdentity();
    }

    inline bool Matrix::IsIdentity() const
    {
        __m128 vTemp1 = _mm_cmpeq_ps( m_rows[0], Vector::UnitX );
        __m128 vTemp2 = _mm_cmpeq_ps( m_rows[1], Vector::UnitY );
        __m128 vTemp3 = _mm_cmpeq_ps( m_rows[2], Vector::UnitZ );
        __m128 vTemp4 = _mm_cmpeq_ps( m_rows[3], Vector::UnitW );
        vTemp1 = _mm_and_ps( vTemp1, vTemp2 );
        vTemp3 = _mm_and_ps( vTemp3, vTemp4 );
        vTemp1 = _mm_and_ps( vTemp1, vTemp3 );
        return ( _mm_movemask_ps( vTemp1 ) == 0x0f );
    }

    //-------------------------------------------------------------------------

    inline Vector Matrix::RotateVector( Vector const& vector ) const
    {
        Vector const X = vector.GetSplatX();
        Vector const Y = vector.GetSplatY();
        Vector const Z = vector.GetSplatZ();

        Vector Result = Z * m_rows[2];
        Result = Vector::MultiplyAdd( Y, m_rows[1], Result );
        Result = Vector::MultiplyAdd( X, m_rows[0], Result );

        return Result;
    }

    inline Vector Matrix::TransformPoint( Vector const& point ) const
    {
        Vector const X = point.GetSplatX();
        Vector const Y = point.GetSplatY();
        Vector const Z = point.GetSplatZ();

        Vector result = Vector::MultiplyAdd( Z, m_rows[2], m_rows[3] );
        result = Vector::MultiplyAdd( Y, m_rows[1], result );
        result = Vector::MultiplyAdd( X, m_rows[0], result );

        Vector const W = result.GetSplatW();
        return result / W;
    }

    inline Vector Matrix::TransformVector3( Vector const& V ) const
    {
        Vector const X = V.GetSplatX();
        Vector const Y = V.GetSplatY();
        Vector const Z = V.GetSplatZ();

        Vector result = Vector::MultiplyAdd( Z, m_rows[2], m_rows[3] );
        result = Vector::MultiplyAdd( Y, m_rows[1], result );
        result = Vector::MultiplyAdd( X, m_rows[0], result );

        return result;
    }

    inline Vector Matrix::TransformVector4( Vector const& V ) const
    {
        // Splat m_x,m_y,m_z and m_w
        Vector vTempX = V.GetSplatX();
        Vector vTempY = V.GetSplatY();
        Vector vTempZ = V.GetSplatZ();
        Vector vTempW = V.GetSplatW();

        // Mul by the matrix
        vTempX = _mm_mul_ps( vTempX, m_rows[0] );
        vTempY = _mm_mul_ps( vTempY, m_rows[1] );
        vTempZ = _mm_mul_ps( vTempZ, m_rows[2] );
        vTempW = _mm_mul_ps( vTempW, m_rows[3] );

        // Add them all together
        vTempX = _mm_add_ps( vTempX, vTempY );
        vTempZ = _mm_add_ps( vTempZ, vTempW );
        vTempX = _mm_add_ps( vTempX, vTempZ );

        return vTempX;
    }

    inline Matrix Matrix::operator*( Matrix const& rhs ) const
    {
        Matrix result = *this;
        result *= rhs;
        return result;
    }

    inline Matrix& Matrix::operator*= ( Matrix const& rhs )
    {
        Vector vX, vY, vZ, vW;

        // Use vW to hold the original row
        vW = m_rows[0];
        vX = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        vY = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        vZ = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        vW = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        vX = _mm_mul_ps( vX, rhs.m_rows[0] );
        vY = _mm_mul_ps( vY, rhs.m_rows[1] );
        vZ = _mm_mul_ps( vZ, rhs.m_rows[2] );
        vW = _mm_mul_ps( vW, rhs.m_rows[3] );
        vX = _mm_add_ps( vX, vZ );
        vY = _mm_add_ps( vY, vW );
        vX = _mm_add_ps( vX, vY );
        m_rows[0] = vX;

        // Repeat for the other 3 rows
        vW = m_rows[1];
        vX = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        vY = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        vZ = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        vW = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        vX = _mm_mul_ps( vX, rhs.m_rows[0] );
        vY = _mm_mul_ps( vY, rhs.m_rows[1] );
        vZ = _mm_mul_ps( vZ, rhs.m_rows[2] );
        vW = _mm_mul_ps( vW, rhs.m_rows[3] );
        vX = _mm_add_ps( vX, vZ );
        vY = _mm_add_ps( vY, vW );
        vX = _mm_add_ps( vX, vY );
        m_rows[1] = vX;

        vW = m_rows[2];
        vX = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        vY = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        vZ = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        vW = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        vX = _mm_mul_ps( vX, rhs.m_rows[0] );
        vY = _mm_mul_ps( vY, rhs.m_rows[1] );
        vZ = _mm_mul_ps( vZ, rhs.m_rows[2] );
        vW = _mm_mul_ps( vW, rhs.m_rows[3] );
        vX = _mm_add_ps( vX, vZ );
        vY = _mm_add_ps( vY, vW );
        vX = _mm_add_ps( vX, vY );
        m_rows[2] = vX;

        vW = m_rows[3];
        vX = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        vY = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        vZ = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        vW = _mm_shuffle_ps( vW, vW, _MM_SHUFFLE( 3, 3, 3, 3 ) );
        vX = _mm_mul_ps( vX, rhs.m_rows[0] );
        vY = _mm_mul_ps( vY, rhs.m_rows[1] );
        vZ = _mm_mul_ps( vZ, rhs.m_rows[2] );
        vW = _mm_mul_ps( vW, rhs.m_rows[3] );
        vX = _mm_add_ps( vX, vZ );
        vY = _mm_add_ps( vY, vW );
        vX = _mm_add_ps( vX, vY );
        m_rows[3] = vX;
        return *this;
    }

    //-------------------------------------------------------------------------

    inline Matrix::Matrix( Vector const axis, Radians angleRadians )
    {
        Vector normal = axis.GetNormalized3();

        Vector C0, C1;
        Vector::SinCos( C0, C1, Vector( (float) angleRadians ) );
        Vector C2 = Vector::One - C1;

        __m128 N0 = _mm_shuffle_ps( normal, normal, _MM_SHUFFLE( 3, 0, 2, 1 ) );
        __m128 N1 = _mm_shuffle_ps( normal, normal, _MM_SHUFFLE( 3, 1, 0, 2 ) );

        __m128 V0 = _mm_mul_ps( C2, N0 );
        V0 = _mm_mul_ps( V0, N1 );

        __m128 R0 = _mm_mul_ps( C2, normal );
        R0 = _mm_mul_ps( R0, normal );
        R0 = _mm_add_ps( R0, C1 );

        __m128 R1 = _mm_mul_ps( C0, normal );
        R1 = _mm_add_ps( R1, V0 );
        __m128 R2 = _mm_mul_ps( C0, normal );
        R2 = _mm_sub_ps( V0, R2 );

        V0 = _mm_and_ps( R0, SIMD::g_maskXYZ0 );
        __m128 V1 = _mm_shuffle_ps( R1, R2, _MM_SHUFFLE( 2, 1, 2, 0 ) );
        V1 = _mm_shuffle_ps( V1, V1, _MM_SHUFFLE( 0, 3, 2, 1 ) );
        __m128 V2 = _mm_shuffle_ps( R1, R2, _MM_SHUFFLE( 0, 0, 1, 1 ) );
        V2 = _mm_shuffle_ps( V2, V2, _MM_SHUFFLE( 2, 0, 2, 0 ) );

        R2 = _mm_shuffle_ps( V0, V1, _MM_SHUFFLE( 1, 0, 3, 0 ) );
        R2 = _mm_shuffle_ps( R2, R2, _MM_SHUFFLE( 1, 3, 2, 0 ) );

        //-------------------------------------------------------------------------

        m_rows[0] = R2;

        R2 = _mm_shuffle_ps( V0, V1, _MM_SHUFFLE( 3, 2, 3, 1 ) );
        R2 = _mm_shuffle_ps( R2, R2, _MM_SHUFFLE( 1, 3, 0, 2 ) );
        m_rows[1] = R2;

        V2 = _mm_shuffle_ps( V2, V0, _MM_SHUFFLE( 3, 2, 1, 0 ) );
        m_rows[2] = V2;
        m_rows[3] = Vector::UnitW;
    }

    inline Matrix Matrix::FromTranslation( Vector const& translation )
    {
        Matrix M;
        M.m_rows[0] = Vector::UnitX;
        M.m_rows[1] = Vector::UnitY;
        M.m_rows[2] = Vector::UnitZ;
        M.m_rows[3] = translation.GetWithW1();
        return M;
    }

    inline Matrix Matrix::FromScale( Vector const& scale )
    {
        Matrix M;
        M.m_rows[0] = _mm_and_ps( scale, SIMD::g_maskX000 );
        M.m_rows[1] = _mm_and_ps( scale, SIMD::g_mask0Y00 );
        M.m_rows[2] = _mm_and_ps( scale, SIMD::g_mask00Z0 );
        M.m_rows[3] = Vector::UnitW;
        return M;
    }

    inline Matrix Matrix::FromUniformScale( float uniformScale )
    {
        Matrix M;
        M.m_rows[0] = _mm_set_ps( 0, 0, 0, uniformScale );
        M.m_rows[1] = _mm_set_ps( 0, 0, uniformScale, 0 );
        M.m_rows[2] = _mm_set_ps( 0, uniformScale, 0, 0 );
        M.m_rows[3] = Vector::UnitW;
        return M;
    }

    inline Matrix Matrix::FromTranslationAndScale( Vector const& translation, Vector const& scale )
    {
        Matrix M;
        M.m_rows[0] = _mm_and_ps( scale, SIMD::g_maskX000 );
        M.m_rows[1] = _mm_and_ps( scale, SIMD::g_mask0Y00 );
        M.m_rows[2] = _mm_and_ps( scale, SIMD::g_mask00Z0 );
        M.m_rows[3] = translation.GetWithW1();
        return M;
    }
}
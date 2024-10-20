#include "Matrix.h"

//-------------------------------------------------------------------------

namespace EE
{
    Matrix const Matrix::Identity( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );

    //-------------------------------------------------------------------------

    Matrix::Matrix( float v00, float v01, float v02, float v03, float v10, float v11, float v12, float v13, float v20, float v21, float v22, float v23, float v30, float v31, float v32, float v33 )
    {
        m_rows[0] = Vector( v00, v01, v02, v03 );
        m_rows[1] = Vector( v10, v11, v12, v13 );
        m_rows[2] = Vector( v20, v21, v22, v23 );
        m_rows[3] = Vector( v30, v31, v32, v33 );
    }

    Matrix::Matrix( float values[16] )
    {
        m_rows[0] = Vector( values[0], values[1], values[2], values[3] );
        m_rows[1] = Vector( values[4], values[5], values[6], values[7] );
        m_rows[2] = Vector( values[8], values[9], values[10], values[11] );
        m_rows[3] = Vector( values[12], values[13], values[14], values[15] );
    }

    Matrix::Matrix( Vector const& xAxis, Vector const& yAxis, Vector const& zAxis )
    {
        m_rows[0] = xAxis;
        m_rows[1] = yAxis;
        m_rows[2] = zAxis;
        m_rows[3] = Vector::UnitW;
    }

    Matrix::Matrix( Vector const& xAxis, Vector const& yAxis, Vector const& zAxis, Vector const& translation )
    {
        m_rows[0] = xAxis;
        m_rows[1] = yAxis;
        m_rows[2] = zAxis;
        m_rows[3] = translation.GetWithW1();
    }

    Matrix::Matrix( EulerAngles const& eulerAngles, Vector const translation )
    {
        float cx, cy, cz, sx, sy, sz, czsx, cxcz, sysz;

        sx = sinf( (float) eulerAngles.m_x ); cx = cosf( (float) eulerAngles.m_x );
        sy = sinf( (float) eulerAngles.m_y ); cy = cosf( (float) eulerAngles.m_y );
        sz = sinf( (float) eulerAngles.m_z ); cz = cosf( (float) eulerAngles.m_z );

        czsx = cz * sx;
        cxcz = cx * cz;
        sysz = sy * sz;

        // Order is XYZ
        m_values[0][0] = cy * cz;
        m_values[0][1] = cy * sz;
        m_values[0][2] = -sy;
        m_values[1][0] = czsx * sy - cx * sz;
        m_values[1][1] = cxcz + sx * sysz;
        m_values[1][2] = cy * sx;
        m_values[2][0] = cxcz * sy + sx * sz;
        m_values[2][1] = -czsx + cx * sysz;
        m_values[2][2] = cx * cy;
        m_values[0][3] = 0.0f;
        m_values[1][3] = 0.0f;
        m_values[2][3] = 0.0f;

        // Translation
        m_rows[3] = translation.GetWithW1();
    }

    EulerAngles Matrix::ToEulerAngles() const
    {
        EulerAngles result;

        result.m_x = Radians( Math::ATan2( m_values[1][2], m_values[2][2] ) );

        float const c2 = Math::Sqrt( ( m_values[0][0] * m_values[0][0] ) + ( m_values[0][1] * m_values[0][1] ) );
        result.m_y = Radians( Math::ATan2( -m_values[0][2], c2 ) );

        float const s1 = Math::Sin( (float) result.m_x );
        float const c1 = Math::Cos( (float) result.m_x );
        result.m_z = Radians( Math::ATan2( ( s1 * m_values[2][0] ) - ( c1 * m_values[1][0] ), ( c1 * m_values[1][1] ) - ( s1 * m_values[2][1] ) ) );

        return result;
    }

    Vector Matrix::GetAxis( Axis axis ) const
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

    //-------------------------------------------------------------------------
    // Matrix Decomposition
    //-------------------------------------------------------------------------

    static bool CheckForZeroScaleInRow( float scale, Vector const& row )
    {
        float const absScale = Math::Abs( scale );

        for ( int i = 0; i < 3; i++ )
        {
            if ( absScale < 1 && Math::Abs( row[i] ) >= FLT_MAX * absScale )
            {
                return false;
            }
        }

        return true;
    }

    // Copied from the IlmBase math library and modified for EE
    static bool ExtractAndRemoveScalingAndShear( Matrix& matrix, Vector& scale, Vector& shear )
    {
        scale = Vector::Zero;
        shear = Vector::Zero;

        Float3 scaleValues = Float3::Zero;
        Float3 shearValues = Float3::Zero;

        // This implementation follows the technique described in the paper by
        // Spencer W. Thomas in the Graphics Gems II article: "Decomposing a 
        // Matrix into Simple Transformations", p. 320.

        Vector row[3];
        row[0] = Vector( matrix[0][0], matrix[0][1], matrix[0][2] );
        row[1] = Vector( matrix[1][0], matrix[1][1], matrix[1][2] );
        row[2] = Vector( matrix[2][0], matrix[2][1], matrix[2][2] );

        float maxVal = 0;
        for ( int i = 0; i < 3; i++ )
        {
            for ( int j = 0; j < 3; j++ )
            {
                if ( Math::Abs( row[i][j] ) > maxVal )
                {
                    maxVal = Math::Abs( row[i][j] );
                }
            }
        }

        // We normalize the 3x3 matrix here.
        // It was noticed that this can improve numerical stability significantly,
        // especially when many of the upper 3x3 matrix's coefficients are very
        // close to zero; we correct for this step at the end by multiplying the 
        // scaling factors by maxVal at the end (shear and rotation are not 
        // affected by the normalization).

        if ( maxVal != 0 )
        {
            for ( int i = 0; i < 3; i++ )
            {
                if ( !CheckForZeroScaleInRow( maxVal, row[i] ) )
                {
                    return false;
                }
                else
                {
                    row[i] /= maxVal;
                }
            }
        }

        // Compute X scale factor.
        scaleValues.m_x = row[0].Length3().ToFloat();
        if ( !CheckForZeroScaleInRow( scaleValues.m_x, row[0] ) )
        {
            return false;
        }

        // Normalize first row.
        row[0] /= scaleValues.m_x;

        // An XY shear factor will shear the X coord. as the Y coord. changes.
        // There are 6 combinations (XY, XZ, YZ, YX, ZX, ZY), although we only
        // extract the first 3 because we can effect the last 3 by shearing in
        // XY, XZ, YZ combined rotations and scales.
        //
        // shear matrix <   1,  YX,  ZX,  0,
        //                 XY,   1,  ZY,  0,
        //                 XZ,  YZ,   1,  0,
        //                  0,   0,   0,  1 >

        // Compute XY shear factor and make 2nd row orthogonal to 1st.
        shearValues[0] = Vector::Dot3( row[0], row[1] ).ToFloat();
        row[1] -= row[0] * shearValues[0];

        // Now, compute Y scale.
        scaleValues.m_y = row[1].Length3().ToFloat();
        if ( !CheckForZeroScaleInRow( scaleValues.m_y, row[1] ) )
        {
            return false;
        }

        // Normalize 2nd row and correct the XY shear factor for Y scaling.
        row[1] /= scaleValues.m_y;
        shearValues[0] /= scaleValues.m_y;

        // Compute XZ and YZ shears, orthogonalize 3rd row.
        shearValues[1] = Vector::Dot3( row[0], row[2] ).ToFloat();
        row[2] -= row[0] * shearValues[1];
        shearValues[2] = Vector::Dot3( row[1], row[2] ).ToFloat();
        row[2] -= row[1] * shearValues[2];

        // Next, get Z scale.
        scaleValues.m_z = row[2].Length3().ToFloat();
        if ( !CheckForZeroScaleInRow( scaleValues.m_z, row[2] ) )
        {
            return false;
        }

        // Normalize 3rd row and correct the XZ and YZ shear factors for Z scaling.
        row[2] /= scaleValues.m_z;
        shearValues[1] /= scaleValues.m_z;
        shearValues[2] /= scaleValues.m_z;

        // At this point, the upper 3x3 matrix in mat is orthonormal.
        // Check for a coordinate system flip. If the determinant
        // is less than zero, then negate the matrix and the scaling factors.
        if ( Vector::Dot3( row[0], Vector::Cross3( row[1], row[2] ) ).ToFloat() < 0 )
        {
            for ( int i = 0; i < 3; i++ )
            {
                scaleValues[i] *= -1;
                row[i] *= -1;
            }
        }

        // Copy over the orthonormal rows into the returned matrix.
        // The upper 3x3 matrix in mat is now a rotation matrix.
        for ( int i = 0; i < 3; i++ )
        {
            matrix[i].SetX( row[i][0] );
            matrix[i].SetY( row[i][1] );
            matrix[i].SetZ( row[i][2] );
        }

        // Correct the scaling factors for the normalization step that we 
        // performed above; shear and rotation are not affected by the 
        // normalization.
        scaleValues *= maxVal;

        //-------------------------------------------------------------------------

        scale = Vector( scaleValues );
        shear = Vector( shearValues );

        return true;
    }

    bool Matrix::Decompose( Quaternion& outRotation, Vector& outTranslation, Vector& outScale ) const
    {
        Matrix copy = *this;
        Vector shr = Vector::Zero;
        outScale = Vector::Zero;

        // Extract and remove scale and shear from matrix
        if ( ExtractAndRemoveScalingAndShear( copy, outScale, shr ) )
        {
            // Extract rotation and translation from unscaled matrix
            outRotation = copy.GetRotation();
            outTranslation = copy.GetTranslation().GetWithW0();
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    Vector Matrix::GetScale() const
    {
        Matrix copy = *this;
        Vector scale = Vector::Zero, shear;
        if ( !ExtractAndRemoveScalingAndShear( copy, scale, shear ) )
        {
            float const lengthX = m_rows[0].Length3().ToFloat();
            float const lengthY = m_rows[1].Length3().ToFloat();
            float const lengthZ = m_rows[2].Length3().ToFloat();
            scale = Vector( lengthX, lengthY, lengthZ, 0.0f );
        }

        return scale;
    }

    Matrix& Matrix::SetScale( Vector const& newScale )
    {
        Vector scale, shear;
        bool result = ExtractAndRemoveScalingAndShear( *this, scale, shear );
        EE_ASSERT( result ); // Cannot set scale on matrix that contains zero-scale

        //-------------------------------------------------------------------------

        m_rows[0] = m_rows[0] * newScale.GetSplatX();
        m_rows[1] = m_rows[1] * newScale.GetSplatY();
        m_rows[2] = m_rows[2] * newScale.GetSplatZ();
        return *this;
    }

    Matrix& Matrix::RemoveScale()
    {
        Vector scale, shear;
        bool result = ExtractAndRemoveScalingAndShear( *this, scale, shear );
        EE_ASSERT( result ); // Cannot remove zero scale from matrix
        return *this;
    }
}
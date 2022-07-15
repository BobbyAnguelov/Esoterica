#pragma once

#include "Vector.h"

//-------------------------------------------------------------------------
// Copied from DirectX Math

namespace EE::Math
{
    inline bool SolveCubic( float e, float f, float g, float& t, float& u, float& v )
    {
        float p, q, h, rc, d, theta, costh3, sinth3;

        p = f - e * e / 3.0f;
        q = g - e * f / 3.0f + e * e * e * 2.0f / 27.0f;
        h = q * q / 4.0f + p * p * p / 27.0f;

        if ( h > 0 )
        {
            t = u = v = 0.f;
            return false; // only one real root
        }

        if ( ( h == 0 ) && ( q == 0 ) ) // all the same root
        {
            t = -e / 3;
            u = -e / 3;
            v = -e / 3;

            return true;
        }

        d = sqrtf( q * q / 4.0f - h );
        if ( d < 0 )
        {
            rc = -powf( -d, 1.0f / 3.0f );
        }
        else
        {
            rc = powf( d, 1.0f / 3.0f );
        }

        theta = Math::ACos( -q / ( 2.0f * d ) );
        costh3 = Math::Cos( theta / 3.0f );
        sinth3 = sqrtf( 3.0f ) * Math::Sin( theta / 3.0f );

        t = 2.0f * rc * costh3 - e / 3.0f;
        u = -rc * ( costh3 + sinth3 ) - e / 3.0f;
        v = -rc * ( costh3 - sinth3 ) - e / 3.0f;

        return true;
    }

    //-----------------------------------------------------------------------------

    inline Vector CalculateEigenVector( float m11, float m12, float m13, float m22, float m23, float m33, float e )
    {
        Float3 fTmp;
        fTmp[0] = m12 * m23 - m13 * ( m22 - e );
        fTmp[1] = m13 * m12 - m23 * ( m11 - e );
        fTmp[2] = ( m11 - e ) * ( m22 - e ) - m12 * m12;

        Vector vTmp( fTmp );
        if ( vTmp.IsZero3() ) // planar or linear
        {
            float f1, f2, f3;

            // we only have one equation - find a valid one
            if ( ( m11 - e != 0 ) || ( m12 != 0 ) || ( m13 != 0 ) )
            {
                f1 = m11 - e; f2 = m12; f3 = m13;
            }
            else if ( ( m12 != 0 ) || ( m22 - e != 0 ) || ( m23 != 0 ) )
            {
                f1 = m12; f2 = m22 - e; f3 = m23;
            }
            else if ( ( m13 != 0 ) || ( m23 != 0 ) || ( m33 - e != 0 ) )
            {
                f1 = m13; f2 = m23; f3 = m33 - e;
            }
            else
            {
                // error, we'll just make something up - we have NO context
                f1 = 1.0f; f2 = 0.0f; f3 = 0.0f;
            }

            if ( f1 == 0 )
            {
                vTmp.m_x = 0.0f;
            }
            else
            {
                vTmp.m_x = 1.0f;
            }

            if ( f2 == 0 )
            {
                vTmp.m_y = 0.0f;
            }
            else
            {
                vTmp.m_y = 1.0f;
            }

            if ( f3 == 0 )
            {
                vTmp.m_z = 0.0f;

                // recalculate y to make equation work
                if ( m12 != 0 )
                {
                    vTmp.m_y = -f1 / f2;
                }
            }
            else
            {
                vTmp.m_z = ( f2 - f1 ) / f3;
            }
        }

        if ( vTmp.LengthSquared3().GetX() > 1e-5f )
        {
            return vTmp.GetNormalized3();
        }
        else // Multiply by a value large enough to make the vector non-zero.
        {
            vTmp = vTmp * 1e5f;
            return vTmp.GetNormalized3();
        }
    }

    //-----------------------------------------------------------------------------

    inline bool CalculateEigenVectors( float m11, float m12, float m13, float m22, float m23, float m33, float e1, float e2, float e3, Vector& V1, Vector& V2, Vector& V3 ) noexcept
    {
        V1 = CalculateEigenVector( m11, m12, m13, m22, m23, m33, e1 );
        V2 = CalculateEigenVector( m11, m12, m13, m22, m23, m33, e2 );
        V3 = CalculateEigenVector( m11, m12, m13, m22, m23, m33, e3 );

        bool v1z = false;
        bool v2z = false;
        bool v3z = false;

        Vector Zero = Vector::Zero;

        if ( V1.IsZero3() )
        {
            v1z = true;
        }

        if ( V2.IsZero3() )
        {
            v2z = true;
        }

        if ( V3.IsZero3() )
        {
            v3z = true;
        }

        bool e12 = ( Math::Abs( Vector::Dot3( V1, V2 ).GetX() ) > 0.1f ); // check for non-orthogonal vectors
        bool e13 = ( Math::Abs( Vector::Dot3( V1, V3 ).GetX() ) > 0.1f );
        bool e23 = ( Math::Abs( Vector::Dot3( V2, V3 ).GetX() ) > 0.1f );

        if ( ( v1z && v2z && v3z ) || ( e12 && e13 && e23 ) ||
            ( e12 && v3z ) || ( e13 && v2z ) || ( e23 && v1z ) ) // all eigenvectors are 0- any basis set
        {
            V1 = Vector::UnitX;
            V2 = Vector::UnitY;
            V3 = Vector::UnitZ;
            return true;
        }

        if ( v1z && v2z )
        {
            Vector vTmp = Vector::Cross3( Vector::UnitY, V3 );
            if ( vTmp.LengthSquared3().GetX() < 1e-5f )
            {
                vTmp = Vector::Cross3( Vector::UnitX, V3 );
            }
            V1 = vTmp.GetNormalized3();
            V2 = Vector::Cross3( V3, V1 );
            return true;
        }

        if ( v3z && v1z )
        {
            Vector vTmp = Vector::Cross3( Vector::UnitY, V2 );
            if ( vTmp.LengthSquared3().GetX() < 1e-5f )
            {
                vTmp = Vector::Cross3( Vector::UnitX, V2 );
            }
            V3 = vTmp.GetNormalized3();
            V1 = Vector::Cross3( V2, V3 );
            return true;
        }

        if ( v2z && v3z )
        {
            Vector vTmp = Vector::Cross3( Vector::UnitY, V1 );
            if ( vTmp.LengthSquared3().GetX() < 1e-5f )
            {
                vTmp = Vector::Cross3( Vector::UnitX, V1 );
            }
            V2 = vTmp.GetNormalized3();
            V3 = Vector::Cross3( V1, V2 );
            return true;
        }

        if ( ( v1z ) || e12 )
        {
            V1 = Vector::Cross3( V2, V3 );
            return true;
        }

        if ( ( v2z ) || e23 )
        {
            V2 = Vector::Cross3( V3, V1 );
            return true;
        }

        if ( ( v3z ) || e13 )
        {
            V3 = Vector::Cross3( V1, V2 );
            return true;
        }

        return true;
    }

    //-----------------------------------------------------------------------------

    inline bool CalculateEigenVectorsFromCovarianceMatrix( float Cxx, float Cyy, float Czz, float Cxy, float Cxz, float Cyz, Vector& V1, Vector& V2, Vector& V3 )
    {
        // Calculate the eigenvalues by solving a cubic equation.
        float e = -( Cxx + Cyy + Czz );
        float f = Cxx * Cyy + Cyy * Czz + Czz * Cxx - Cxy * Cxy - Cxz * Cxz - Cyz * Cyz;
        float g = Cxy * Cxy * Czz + Cxz * Cxz * Cyy + Cyz * Cyz * Cxx - Cxy * Cyz * Cxz * 2.0f - Cxx * Cyy * Czz;

        float ev1, ev2, ev3;
        if ( !SolveCubic( e, f, g, ev1, ev2, ev3 ) )
        {
            // set them to arbitrary orthonormal basis set
            V1 = Vector::UnitX;
            V2 = Vector::UnitY;
            V3 = Vector::UnitZ;
            return false;
        }

        return CalculateEigenVectors( Cxx, Cxy, Cxz, Cyy, Cyz, Czz, ev1, ev2, ev3, V1, V2, V3 );
    }
}
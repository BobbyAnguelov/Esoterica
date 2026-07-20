#include "Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    Int2 const Int2::Zero = Int2( 0, 0 );

    //-------------------------------------------------------------------------

    Int3 const Int3::Zero = Int3( 0, 0, 0 );

    //-------------------------------------------------------------------------

    Int4 const Int4::Zero = Int4( 0, 0, 0, 0 );

    //-------------------------------------------------------------------------

    Float2 const Float2::Zero = Float2( 0, 0 );
    Float2 const Float2::One = Float2( 1, 1 );
    Float2 const Float2::UnitX = Float2( 1, 0 );
    Float2 const Float2::UnitY = Float2( 0, 1 );

    //-------------------------------------------------------------------------

    Float3 const Float3::Zero = Float3( 0, 0, 0 );
    Float3 const Float3::One = Float3( 1, 1, 1 );
    Float3 const Float3::UnitX = Float3( 1, 0, 0 );
    Float3 const Float3::UnitY = Float3( 0, 1, 0 );
    Float3 const Float3::UnitZ = Float3( 0, 0, 1 );

    Float3 const Float3::WorldForward = Float3( 0, -1, 0 );
    Float3 const Float3::WorldUp = Float3( 0, 0, 1 );
    Float3 const Float3::WorldRight = Float3( -1, 0, 0 );

    //-------------------------------------------------------------------------

    Float4 const Float4::Zero = Float4( 0, 0, 0, 0 );
    Float4 const Float4::One = Float4( 1, 1, 1, 1 );
    Float4 const Float4::UnitX = Float4( 1, 0, 0, 0 );
    Float4 const Float4::UnitY = Float4( 0, 1, 0, 0 );
    Float4 const Float4::UnitZ = Float4( 0, 0, 1, 0 );
    Float4 const Float4::UnitW = Float4( 0, 0, 0, 1 );

    Float4 const Float4::WorldForward = Float4( 0, -1, 0, 0 );
    Float4 const Float4::WorldUp = Float4( 0, 0, 1, 0 );
    Float4 const Float4::WorldRight = Float4( -1, 0, 0, 0 );

    //-------------------------------------------------------------------------

    Radians const Radians::Pi = Radians( Math::Pi );
    Radians const Radians::TwoPi = Radians( Math::TwoPi );
    Radians const Radians::OneDivPi = Radians( Math::OneDivPi );
    Radians const Radians::OneDivTwoPi = Radians( Math::OneDivTwoPi );
    Radians const Radians::PiDivTwo = Radians( Math::PiDivTwo );
    Radians const Radians::PiDivFour = Radians( Math::PiDivFour );

    //-------------------------------------------------------------------------

    Axis GetOrthogonalAxis( Axis axis1, Axis axis2 )
    {
        EE_ASSERT( axis1 != axis2 );
        EE_ASSERT( axis1 != GetOppositeAxis( axis2 ) );

        //-------------------------------------------------------------------------

        if ( axis1 == Axis::Y && axis2 == Axis::Z )
        {
            return Axis::X;
        }

        if ( axis1 == Axis::Z && axis2 == Axis::Y )
        {
            return Axis::NegX;
        }

        if ( axis1 == Axis::NegY && axis2 == Axis::Z )
        {
            return Axis::NegX;
        }

        if ( axis1 == Axis::Z && axis2 == Axis::NegY )
        {
            return Axis::X;
        }

        if ( axis1 == Axis::Y && axis2 == Axis::NegZ )
        {
            return Axis::NegX;
        }

        if ( axis1 == Axis::NegZ && axis2 == Axis::Y )
        {
            return Axis::X;
        }

        if ( axis1 == Axis::NegY && axis2 == Axis::NegZ )
        {
            return Axis::X;
        }

        if ( axis1 == Axis::NegZ && axis2 == Axis::NegY )
        {
            return Axis::NegX;
        }

        //-------------------------------------------------------------------------

        if ( axis1 == Axis::Z && axis2 == Axis::X )
        {
            return Axis::Y;
        }

        if ( axis1 == Axis::X && axis2 == Axis::Z )
        {
            return Axis::NegY;
        }

        if ( axis1 == Axis::NegZ && axis2 == Axis::X )
        {
            return Axis::NegY;
        }

        if ( axis1 == Axis::X && axis2 == Axis::NegZ )
        {
            return Axis::Y;
        }

        if ( axis1 == Axis::Z && axis2 == Axis::NegX )
        {
            return Axis::NegY;
        }

        if ( axis1 == Axis::NegX && axis2 == Axis::Z )
        {
            return Axis::Y;
        }

        if ( axis1 == Axis::NegZ && axis2 == Axis::NegX )
        {
            return Axis::Y;
        }

        if ( axis1 == Axis::NegX && axis2 == Axis::NegZ )
        {
            return Axis::NegY;
        }

        //-------------------------------------------------------------------------

        if ( axis1 == Axis::X && axis2 == Axis::Y )
        {
            return Axis::Z;
        }

        if ( axis1 == Axis::Y && axis2 == Axis::X )
        {
            return Axis::NegZ;
        }

        if ( axis1 == Axis::NegX && axis2 == Axis::Y )
        {
            return Axis::NegZ;
        }

        if ( axis1 == Axis::Y && axis2 == Axis::NegX )
        {
            return Axis::Z;
        }

        if ( axis1 == Axis::X && axis2 == Axis::NegY )
        {
            return Axis::NegZ;
        }

        if ( axis1 == Axis::NegY && axis2 == Axis::X )
        {
            return Axis::Z;
        }

        if ( axis1 == Axis::NegX && axis2 == Axis::NegY )
        {
            return Axis::Z;
        }

        if ( axis1 == Axis::NegY && axis2 == Axis::NegX )
        {
            return Axis::NegZ;
        }

        //-------------------------------------------------------------------------

        EE_UNREACHABLE_CODE();
        return Axis::X;
    }

    //-------------------------------------------------------------------------

    namespace Math
    {

        // Half precision floats based on // https://gist.github.com/rygorous/2156668

        union FP32
        {
            uint32_t u;
            float    f;
            struct
            {
                uint32_t m_mantissa : 23;
                uint32_t m_exponent : 8;
                uint32_t m_sign : 1;
            };
        };

        union FP16
        {
            uint16_t u;
            struct
            {
                uint32_t m_mantissa : 10;
                uint32_t m_exponent : 5;
                uint32_t m_sign : 1;
            };
        };

        uint16_t FloatToHalf( float const value )
        {
            FP32 f = { 0 };
            FP16 o = { 0 };

            f.f = value;

            // Based on ISPC reference code (with minor modifications)
            if ( f.m_exponent == 0 ) // Signed zero/denormal (which will underflow)
            {
                o.m_exponent = 0;
            }
            else if ( f.m_exponent == 255 ) // Inf or NaN (all exponent bits set)
            {
                o.m_exponent = 31;
                o.m_mantissa = f.m_mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
            }
            else // Normalized number
            {
                // Exponent unbias the single, then bias the halfp
                int const newExponent = f.m_exponent - 127 + 15;
                if ( newExponent >= 31 ) // Overflow, return signed infinity
                {
                    o.m_exponent = 31;
                }
                else if ( newExponent <= 0 ) // Underflow
                {
                    if ( ( 14 - newExponent ) <= 24 ) // Mantissa might be non-zero
                    {
                        uint32_t const mantissa = f.m_mantissa | 0x800000; // Hidden 1 bit
                        o.m_mantissa = mantissa >> ( 14 - newExponent );
                        if ( ( mantissa >> ( 13 - newExponent ) ) & 1 ) // Check for rounding
                        {
                            o.u++; // Round, might overflow into exp bit, but this is OK
                        }
                    }
                }
                else
                {
                    o.m_exponent = newExponent;
                    o.m_mantissa = f.m_mantissa >> 13;
                    if ( f.m_mantissa & 0x1000 ) // Check for rounding
                    {
                        o.u++; // Round, might overflow to inf, this is OK
                    }
                }
            }

            o.m_sign = f.m_sign;
            return o.u;
        }

        float HalfToFloat( uint16_t const value )
        {
            FP16 h = { 0 };
            h.u = value;

            static constexpr FP32     MAGIC = { 113 << 23 };
            static constexpr uint32_t SHIFTED_EXP = 0x7c00 << 13; // exponent mask after shift

            FP32 o;
            o.u = ( h.u & 0x7fff ) << 13;           // exponent/mantissa bits
            uint32_t const exp = SHIFTED_EXP & o.u; // just the exponent
            o.u += ( 127 - 15 ) << 23;              // exponent adjust

            // handle exponent special cases
            if ( exp == SHIFTED_EXP ) // Inf/NaN?
            {
                o.u += ( 128 - 16 ) << 23; // extra exp adjust
            }
            else if ( exp == 0 ) // Zero/Denormal?
            {
                o.u += 1 << 23; // extra exp adjust
                o.f -= MAGIC.f; // renormalize
            }

            o.u |= ( h.u & 0x8000 ) << 16; // sign bit
            return o.f;
        }
    }
}

#include "Percentage.h"

//-------------------------------------------------------------------------

namespace EE
{
    Percentage Percentage::Clamp( Percentage value, bool allowLooping )
    {
        Percentage clampedValue = value;
        if ( allowLooping )
        {
            float integerPortion;
            clampedValue = Percentage( Math::ModF( float( value ), integerPortion ) );
            if ( clampedValue < 0.0f ) // wrapping backwards
            {
                clampedValue = Percentage( 1.0f - clampedValue );
            }
        }
        else // Clamp
        {
            if ( value < 0.0f )
            {
                clampedValue = Percentage( 0.0f );
            }
            else if ( value > 1.0f )
            {
                clampedValue = Percentage( 1.0f );
            }
        }

        return clampedValue;
    }
}
#include "MathStringHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    char const* ToString( Axis axis )
    {
        static char const* const axesStrings[] =
        {
            "X",
            "Y",
            "Z",
            "-X",
            "-Y",
            "-Z",
        };

        return axesStrings[(int32_t) axis];
    }
}
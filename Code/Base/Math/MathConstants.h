#pragma once
#include <limits>

//-------------------------------------------------------------------------

namespace EE::Math
{
    // Note: Math globals do not follow the general coding convention (g_XXX) since they are treated as special types

    static constexpr float const Epsilon = 1.0e-06f;
    static constexpr float const LargeEpsilon = 1.0e-04f;
    static constexpr float const HugeEpsilon = 1.0e-02f;
    static constexpr float const Pi = 3.141592654f;
    static constexpr float const TwoPi = 6.283185307f;
    static constexpr float const OneDivPi = 0.318309886f;
    static constexpr float const OneDivTwoPi = 0.159154943f;
    static constexpr float const PiDivTwo = 1.570796327f;
    static constexpr float const PiDivFour = 0.785398163f;

    static constexpr float const SqrtTwo = 1.4142135623730950488016887242097f;
    static constexpr float const OneDivSqrtTwo = 1.0f / SqrtTwo;

    static constexpr float const DegreesToRadians = 0.0174532925f;
    static constexpr float const RadiansToDegrees = 57.2957795f;

    static constexpr float const Infinity = std::numeric_limits<float>::infinity();
    static constexpr float const QNaN = std::numeric_limits<float>::quiet_NaN();
}
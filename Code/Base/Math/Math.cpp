#include "Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    Int2 const Int2::Zero = Int2( 0, 0 );

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
}
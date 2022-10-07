#include "AnimationEvent_Warp.h"
#include "System/Imgui/MaterialDesignIcons.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    Color GetDebugForWarpRule( TargetWarpRule rule )
    {
        Color color;

        switch ( rule )
        {
            case TargetWarpRule::WarpXY:
            color = Colors::Violet;
            break;

            case TargetWarpRule::WarpZ:
            color = Colors::CornflowerBlue;
            break;

            case TargetWarpRule::WarpXYZ:
            color = Colors::BlueViolet;
            break;

            case TargetWarpRule::RotationOnly:
            color = Colors::OrangeRed;
            break;
        }

        return color;
    }

    //-------------------------------------------------------------------------

    InlineString TargetWarpEvent::GetDebugText() const
    {
        constexpr static char const* labels[] = 
        {
            EE_ICON_AXIS_X_ARROW"WarpXY",
            EE_ICON_AXIS_Z_ARROW"WarpZ",
            EE_ICON_AXIS_ARROW"WarpXYZ",
            EE_ICON_AXIS_Z_ROTATE_CLOCKWISE"Rot",
        };

        return labels[(uint8_t) m_rule];
    }
    #endif
}
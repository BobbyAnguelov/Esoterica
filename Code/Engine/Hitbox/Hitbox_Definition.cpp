#include "Hitbox_Definition.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    Color HitboxShape::GetSeverityColor( HitboxDamageSeverity type )
    {
        static Color const colors[] =
        {
            Colors::Red, // Critical
            Colors::Orange, // High
            Colors::Yellow, // Medium
            Colors::Chartreuse, // Low
        };

        return colors[(uint8_t) type];
    }
    #endif
}
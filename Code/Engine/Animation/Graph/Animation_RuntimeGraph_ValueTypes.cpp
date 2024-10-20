#include "Animation_RuntimeGraph_ValueTypes.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    Color GetColorForValueType( GraphValueType type )
    {
        /*static*/ Color const colors[] =
        {
            Colors::GhostWhite, // unknown
            Colors::LightPink, // bool
            Colors::MediumPurple, // ID
            Colors::LightSteelBlue, // float
            Colors::SkyBlue, // vector
            Colors::CornflowerBlue, // target
            Colors::Thistle, // bone mask
            Colors::MediumSeaGreen, // pose
            Colors::White, // special
        };

        return colors[(uint8_t) type];
    }

    char const* GetNameForValueType( GraphValueType type )
    {
        constexpr static char const* const names[] =
        {
            "Unknown",
            "Bool",
            "ID",
            "Float",
            "Vector",
            "Target",
            "Bone Mask",
            "Pose",
            "Special",
        };

        return names[(uint8_t) type];
    }

    StringID GetIDForValueType( GraphValueType type )
    {
        static StringID IDs[] =
        {
            StringID( GetNameForValueType( GraphValueType::Unknown ) ),
            StringID( GetNameForValueType( GraphValueType::Bool ) ),
            StringID( GetNameForValueType( GraphValueType::ID ) ),
            StringID( GetNameForValueType( GraphValueType::Float ) ),
            StringID( GetNameForValueType( GraphValueType::Vector ) ),
            StringID( GetNameForValueType( GraphValueType::Target ) ),
            StringID( GetNameForValueType( GraphValueType::BoneMask ) ),
            StringID( GetNameForValueType( GraphValueType::Pose ) ),
            StringID( GetNameForValueType( GraphValueType::Special ) )
        };

        return IDs[(int32_t) type];
    }

    GraphValueType GetValueTypeForID( StringID ID )
    {
        for ( uint8_t i = 0; i <= (uint8_t) GraphValueType::Special; i++ )
        {
            GraphValueType const gvt = (GraphValueType) i;

            if ( GetIDForValueType( gvt ) == ID )
            {
                return gvt;
            }
        }

        EE_UNREACHABLE_CODE();
        return GraphValueType::Unknown;
    }
    #endif
}
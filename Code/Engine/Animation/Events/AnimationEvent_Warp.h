#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE{ struct Color; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API OrientationWarpEvent final : public Event
    {
        EE_REFLECT_TYPE( OrientationWarpEvent );

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return "Warp"; }
        #endif
    };

    //-------------------------------------------------------------------------

    enum class TargetWarpRule : uint8_t
    {
        EE_REFLECT_ENUM

        WarpXY = 0,
        WarpZ,
        WarpXYZ,
        RotationOnly
    };

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API Color GetDebugForWarpRule( TargetWarpRule rule );
    #endif

    enum class TargetWarpAlgorithm : uint8_t
    {
        EE_REFLECT_ENUM

        Lerp,
        Hermite,
        HermiteFeaturePreserving,
        Bezier,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetWarpEvent final : public Event
    {
        EE_REFLECT_TYPE( TargetWarpEvent );

    public:

        inline TargetWarpRule GetRule() const { return m_rule; }
        inline TargetWarpAlgorithm GetTranslationAlgorithm() const { return m_algorithm; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override;
        #endif

    private:

        EE_REFLECT() TargetWarpRule        m_rule = TargetWarpRule::WarpXYZ;
        EE_REFLECT() TargetWarpAlgorithm   m_algorithm = TargetWarpAlgorithm::Bezier;
    };
}
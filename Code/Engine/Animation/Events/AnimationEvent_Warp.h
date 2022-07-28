#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API WarpEvent final : public Event
    {
        EE_REGISTER_TYPE( WarpEvent );

    public:

        // The type of warping we are allowed to perform for this event
        enum class Type : uint8_t
        {
            EE_REGISTER_ENUM

            Full = 0, // Allows both rotating and stretching/compressing the original motion
            RotationOnly, // Only allows for rotation adjustment of the original motion
        };

        enum class TranslationWarpMode
        {
            EE_REGISTER_ENUM

            Hermite,
            Bezier,
            FeaturePreserving
        };

    public:

        inline Type GetWarpAdjustmentType() const { return m_type; }
        inline TranslationWarpMode GetTranslationWarpMode() const { return m_translationMode; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override;
        #endif

    private:

        EE_EXPOSE Type                 m_type = Type::Full;
        EE_EXPOSE TranslationWarpMode  m_translationMode = TranslationWarpMode::Hermite;
        EE_EXPOSE bool                 m_allowWarpInZ = true;
    };
} 
#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct Color;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class TransitionRule : uint8_t
    {
        EE_REFLECT_ENUM

        AllowTransition = 0,
        ConditionallyAllowTransition,
        BlockTransition,
    };

    enum class TransitionRuleCondition : uint8_t
    {
        EE_REFLECT_ENUM

        AnyAllowed = 0,
        FullyAllowed,
        ConditionallyAllowed,
        Blocked,
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API char const* GetTransitionRuleName( TransitionRule marker );
    EE_ENGINE_API char const* GetTransitionRuleConditionName( TransitionRuleCondition markerCondition );
    EE_ENGINE_API Color GetTransitionMarkerColor( TransitionRule marker );
    #endif

    //-------------------------------------------------------------------------

    // Helper event to provide easy flags for use within the animation graph
    class EE_ENGINE_API TransitionEvent final : public Event
    {
        EE_REFLECT_TYPE( TransitionEvent );

    public:

        inline TransitionRule GetRule() const { return m_rule; }
        inline StringID GetOptionalID() const { return m_optionalMarkerID; }
        inline bool IsTransitioningAllowed() const { return m_rule != TransitionRule::BlockTransition; }
        inline bool IsTransitionFullyAllowed() const { return m_rule == TransitionRule::AllowTransition; }
        inline bool IsTransitionConditionallyAllowed() const { return m_rule == TransitionRule::ConditionallyAllowTransition; }
        inline bool IsTransitionBlocked() const { return m_rule == TransitionRule::BlockTransition; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return GetTransitionRuleName( m_rule ); }
        #endif

    private:

        EE_REFLECT() TransitionRule     m_rule;
        EE_REFLECT() StringID           m_optionalMarkerID;
    };
}
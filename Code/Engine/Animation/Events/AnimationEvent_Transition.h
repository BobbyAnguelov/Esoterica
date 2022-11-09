#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct Color;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class TransitionMarker : uint8_t
    {
        EE_REGISTER_ENUM

        AllowTransition = 0,
        ConditionallyAllowTransition,
        BlockTransition,
    };

    enum class TransitionMarkerCondition : uint8_t
    {
        EE_REGISTER_ENUM

        AnyAllowed = 0,
        FullyAllowed,
        ConditionallyAllowed,
        Blocked,
    };

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API char const* GetTransitionMarkerName( TransitionMarker marker );
    EE_ENGINE_API char const* GetTransitionMarkerConditionName( TransitionMarkerCondition markerCondition );
    EE_ENGINE_API Color GetTransitionMarkerColor( TransitionMarker marker );
    #endif

    //-------------------------------------------------------------------------

    // Helper event to provide easy flags for use within the animation graph
    class EE_ENGINE_API TransitionEvent final : public Event
    {
        EE_REGISTER_TYPE( TransitionEvent );

    public:

        inline TransitionMarker GetMarker() const { return m_marker; }
        inline StringID GetMarkerID() const { return m_optionalMarkerID; }
        inline bool IsTransitioningAllowed() const { return m_marker != TransitionMarker::BlockTransition; }
        inline bool IsTransitionFullyAllowed() const { return m_marker == TransitionMarker::AllowTransition; }
        inline bool IsTransitionConditionallyAllowed() const { return m_marker == TransitionMarker::ConditionallyAllowTransition; }
        inline bool IsTransitionBlocked() const { return m_marker == TransitionMarker::BlockTransition; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return GetTransitionMarkerName( m_marker ); }
        #endif

    private:

        EE_EXPOSE TransitionMarker      m_marker;
        EE_EXPOSE StringID              m_optionalMarkerID;
    };
}
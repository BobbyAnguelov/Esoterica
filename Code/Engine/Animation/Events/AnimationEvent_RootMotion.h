#pragma once
#include "Engine/Animation/AnimationEvent.h"

//-------------------------------------------------------------------------
// Helper event to provide rules for procedural vs root motion within an animation
//-------------------------------------------------------------------------
// If this event exists within an animation and the root motion override node is set to respect event, 
// then the override will be disabled for the duration of the event

namespace EE::Animation
{
    class EE_ENGINE_API RootMotionEvent final : public Event
    {
        EE_REFLECT_TYPE( RootMotionEvent );

    public:

        inline Seconds GetBlendTime() const { return m_blendTime; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return InlineString( InlineString::CtorSprintf(), "Disable Override - %.2f", m_blendTime.ToFloat() ); }
        #endif

    private:

        EE_REFLECT() Seconds   m_blendTime = 0.1f;
    };
}
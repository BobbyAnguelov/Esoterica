#pragma once
#include "Engine/Animation/AnimationEvent.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API SnapToFrameEvent final : public Event
    {
        EE_REFLECT_TYPE( SnapToFrameEvent );

    public:

        enum class SelectionMode
        {
            EE_REFLECT_ENUM

            Floor = 0,
            Round,
        };

    public:

        inline SelectionMode GetFrameSelectionMode() const { return m_frameSelection; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override
        {
            return InlineString( InlineString::CtorSprintf(), "Snap Frame (%s)", m_frameSelection == SelectionMode::Round ? "Round" : "Floor" );
        }
        #endif

    private:

        EE_REFLECT()
        SelectionMode      m_frameSelection;
    };
}
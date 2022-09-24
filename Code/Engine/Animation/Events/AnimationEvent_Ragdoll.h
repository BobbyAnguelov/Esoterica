#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "System/Math/FloatCurve.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API RagdollEvent final : public Event
    {
        friend class RagdollEventTrack;

        EE_REGISTER_TYPE( RagdollEvent );

    public:

         virtual bool IsValid() const override{ return m_physicsWeightCurve.GetNumPoints() > 0; }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return ""; }
        #endif

    private:

        EE_EXPOSE FloatCurve         m_physicsWeightCurve;
    };
}
#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "Base/Math/FloatCurve.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API RagdollEvent final : public Event
    {
        friend class RagdollEventTrack;

        EE_REFLECT_TYPE( RagdollEvent );

    public:

        virtual bool IsValid() const override{ return m_physicsWeightCurve.GetNumPoints() > 0; }

        // Evaluate the curve to the get the desired weight
        inline float GetPhysicsBlendWeight( Percentage percentageThrough ) const
        {
            EE_ASSERT( m_physicsWeightCurve.GetNumPoints() > 0 );
            return m_physicsWeightCurve.Evaluate( percentageThrough.ToFloat() );
        }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return ""; }
        #endif

    private:

        EE_REFLECT() FloatCurve         m_physicsWeightCurve;
    };
}
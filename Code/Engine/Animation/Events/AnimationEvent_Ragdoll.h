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
        virtual InlineString GetDebugText( Percentage percentageThroughEvent ) const override 
        {
            InlineString str;

            if ( percentageThroughEvent >= 0.0f )
            {
                if ( m_physicsWeightCurve.HasPoints() )
                {
                    str.append_sprintf( "Physics Curve: %f", m_physicsWeightCurve.EvaluateAtPercentageThroughParameterRange( percentageThroughEvent ) );
                }
            }

            return str;
        }
        #endif

    private:

        EE_REFLECT() FloatCurve         m_physicsWeightCurve;
    };
}
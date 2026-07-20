#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "Base/Types/StringID.h"
#include "Base/Math/FloatCurve.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API FloatCurveEvent final : public Event
    {
        EE_REFLECT_TYPE( FloatCurveEvent );

    public:

        inline StringID const& GetID() const { return m_ID; }
        inline FloatCurve const& GetCurve() const { return m_curve; }
        virtual StringID GetSyncEventID() const override { return m_ID; }
        virtual bool IsValid() const override{ return true; }

        inline float EvaluateCurve( float percentageThroughCurve ) const
        {
            return m_curve.EvaluateAtPercentageThroughParameterRange( percentageThroughCurve );
        }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText( Percentage percentageThroughEvent ) const override
        {
            InlineString str;

            if ( m_ID.IsValid() )
            {
                str.append_sprintf( "ID: %s ", m_ID.c_str() );
            }

            if ( percentageThroughEvent >= 0.0f )
            {
                if ( m_curve.HasPoints() )
                {
                    str.append_sprintf( "Value: %f", m_curve.EvaluateAtPercentageThroughParameterRange( percentageThroughEvent ) );
                }
            }

            return str;
        }
        #endif

    private:

        EE_REFLECT();
        StringID         m_ID;

        EE_REFLECT();
        FloatCurve       m_curve;
    };
}
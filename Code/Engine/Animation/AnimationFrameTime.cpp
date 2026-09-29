#include "AnimationFrameTime.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    FrameTime::FrameTime( int32_t frameIndex, Percentage percentageThrough )
        : m_frameIndex( frameIndex )
        , m_percentageThrough( percentageThrough )
    {
        EE_ASSERT( percentageThrough.ToFloat() >= 0.0f && percentageThrough < 1.0f );
    }

    FrameTime::FrameTime( Percentage percent, int32_t numFrames )
    {
        EE_ASSERT( numFrames > 0 && percent.ToFloat() >= 0.0f );
        int32_t const lastFrameIdx = numFrames - 1;

        if ( percent == 0.0f )
        {
            m_frameIndex = 0;
            m_percentageThrough = 0.0f;
        }
        else if ( percent == 1.0f )
        {
            m_frameIndex = lastFrameIdx;
            m_percentageThrough = 0.0f;
        }
        else
        {
            percent.Clamp( true );
            float integerPortion = 0;
            m_percentageThrough = Percentage( Math::ModF( percent.ToFloat() * lastFrameIdx, integerPortion ) );
            if ( Math::IsNearZero( m_percentageThrough, Math::LargeEpsilon ) )
            {
                m_percentageThrough = 0.0f;
            }
            m_frameIndex = (int32_t) integerPortion;
        }
    }
}
#include "Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    RootMotionDebugger::RootMotionDebugger()
    {
        m_recordedRootTransforms.reserve( s_recordingBufferSize );
    }

    void RootMotionDebugger::SetDebugMode( RootMotionDebugMode mode )
    {
        // If we started recording, reset the transform buffers
        if ( m_debugMode < RootMotionDebugMode::DrawRecordedRootMotion && mode >= RootMotionDebugMode::DrawRecordedRootMotion )
        {
            m_freeBufferIdx = 0;
            m_recordedRootTransforms.clear();
        }

        m_debugMode = mode;
    }

    void RootMotionDebugger::StartCharacterUpdate( Transform const& characterWorldTransform )
    {
        m_startWorldTransform = characterWorldTransform;
        m_recordedActions.clear();
    }

    void RootMotionDebugger::EndCharacterUpdate( Transform const& characterWorldTransform )
    {
        m_endWorldTransform = characterWorldTransform;

        //-------------------------------------------------------------------------

        if ( m_debugMode >= RootMotionDebugMode::DrawRecordedRootMotion )
        {
            EE_ASSERT( m_freeBufferIdx < s_recordingBufferSize );

            // If we havent filled the entire buffer, add a new element
            if ( m_freeBufferIdx == m_recordedRootTransforms.size() )
            {
                m_recordedRootTransforms.push_back();
            }

            // Record root transforms
            if ( m_recordedActions.empty() )
            {
                m_recordedRootTransforms[m_freeBufferIdx].m_actualTransform = m_endWorldTransform;
                m_recordedRootTransforms[m_freeBufferIdx].m_expectedTransform = m_endWorldTransform;
            }
            else
            {
                m_recordedRootTransforms[m_freeBufferIdx].m_actualTransform = m_endWorldTransform;
                m_recordedRootTransforms[m_freeBufferIdx].m_expectedTransform = m_recordedActions.back().m_rootMotionDelta * m_startWorldTransform;
            }

            // Update circular buffer indices
            //-------------------------------------------------------------------------

            m_freeBufferIdx++;
            if ( m_freeBufferIdx >= s_recordingBufferSize )
            {
                m_freeBufferIdx = 0;
            }
        }
    }

    void RootMotionDebugger::DrawDebug( Drawing::DrawContext& drawingContext )
    {
        if ( m_debugMode == RootMotionDebugMode::DrawRoot )
        {
            DrawRootBone( drawingContext, m_endWorldTransform );
        }
        else if ( m_debugMode == RootMotionDebugMode::DrawRecordedRootMotion || m_debugMode == RootMotionDebugMode::DrawRecordedRootMotionAdvanced )
        {
            bool const isAdvancedDisplayEnabled = m_debugMode == RootMotionDebugMode::DrawRecordedRootMotionAdvanced;

            auto DrawSegment = [this, &drawingContext, isAdvancedDisplayEnabled] ( int32_t idx0, int32_t idx1, int32_t c )
            {
                static Vector const verticalOffset( 0, 0, 0.2f );
                static Color const actualColors[2] = { Colors::Blue, Colors::Red };
                static Color const expectedColors[2] = { Colors::Cyan, Colors::Magenta };

                drawingContext.DrawLine( m_recordedRootTransforms[idx0].m_actualTransform.GetTranslation(), m_recordedRootTransforms[idx1].m_actualTransform.GetTranslation(), actualColors[c], 3.0f );
                drawingContext.DrawAxis( m_recordedRootTransforms[idx1].m_actualTransform, 0.1f, 2.0f );

                if ( isAdvancedDisplayEnabled )
                {
                    drawingContext.DrawLine( m_recordedRootTransforms[idx0].m_expectedTransform.GetTranslation() + verticalOffset, m_recordedRootTransforms[idx1].m_expectedTransform.GetTranslation() + verticalOffset, expectedColors[c], 3.0f );

                    Transform axisTransform = m_recordedRootTransforms[idx1].m_expectedTransform;
                    axisTransform.AddTranslation( verticalOffset );
                    drawingContext.DrawAxis( axisTransform, 0.1f, 2.0f );
                }
            };

            //-------------------------------------------------------------------------

            int32_t const numRecordedTransforms = (int32_t) m_recordedRootTransforms.size();
            bool const isBufferFull = ( numRecordedTransforms == s_recordingBufferSize );
            if ( !isBufferFull )
            {
                for ( int32_t i = 1; i < m_freeBufferIdx; i++ )
                {
                    int32_t const c = Math::IsEven( i ) ? 0 : 1;
                    DrawSegment( i - 1, i, c );
                }
            }
            else // Draw all transforms
            {
                // Draw all the transforms from the start buffer to either the end of the array or the first free idx
                for ( int32_t i = m_freeBufferIdx + 1; i < numRecordedTransforms; i++ )
                {
                    int32_t const c = Math::IsEven( i ) ? 0 : 1;
                    DrawSegment( i - 1, i, c );
                }

                // Draw looped range
                for ( int32_t i = 1; i < m_freeBufferIdx; i++ )
                {
                    int32_t const c = Math::IsEven( i ) ? 0 : 1;
                    DrawSegment( i - 1, i, c );
                }

                // Connect the two ranges
                if ( m_freeBufferIdx != 0 )
                {
                    DrawSegment( 0, s_recordingBufferSize - 1, 0 );
                }
            }
        }
    }
}
#endif
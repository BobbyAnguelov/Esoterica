#include "AnimationRootMotion.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void RootMotionData::Clear()
    {
        m_transforms.clear();
        m_averageLinearVelocity = 0.0f;
        m_averageAngularVelocity = 0.0f;
        m_totalDelta = Transform::Identity;
    }

    #if EE_DEVELOPMENT_TOOLS
    void RootMotionData::DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform ) const
    {
        constexpr static float const axisSize = 0.025f;
        constexpr static float const axisThickness = 4.0f;

        if ( m_transforms.empty() )
        {
            return;
        }

        auto previousWorldRootMotionTransform = m_transforms[0] * worldTransform;
        ctx.DrawAxis( previousWorldRootMotionTransform, axisSize, axisThickness );

        auto const numTransforms = m_transforms.size();
        for ( auto i = 1; i < numTransforms; i++ )
        {
            auto const worldRootMotionTransform = m_transforms[i] * worldTransform;
            ctx.DrawLine( previousWorldRootMotionTransform.GetTranslation(), worldRootMotionTransform.GetTranslation(), ( i % 2 == 0 ) ? Colors::Yellow : Colors::Cyan, 1.5f );
            ctx.DrawAxis( worldRootMotionTransform, axisSize, axisThickness );
            previousWorldRootMotionTransform = worldRootMotionTransform;
        }
    }
    #endif
}
#include "Animation_Task_Sample.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    SampleTask::SampleTask( AnimationClip const* pAnimation, Percentage time )
        : PoseTask()
        , m_pAnimation( pAnimation )
        , m_time( time )
    {
        EE_ASSERT( m_pAnimation != nullptr );
        EE_ASSERT( Math::IsFinite( time.ToFloat() ) );
        EE_ASSERT( time.ToFloat() >= 0.0f && time.ToFloat() <= 1.0f );
    }

    void SampleTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        EE_ASSERT( m_pAnimation != nullptr );

        auto pResultBuffer = GetNewPoseBuffer( context );

        // Sample primary pose
        //-------------------------------------------------------------------------

        m_pAnimation->GetPose( m_time, pResultBuffer->GetPrimaryPose(), context.m_skeletonLOD, context.m_sampleFloatChannels );

        // Sample secondary poses
        //-------------------------------------------------------------------------

        int32_t const numPoses = (int32_t) pResultBuffer->m_poses.size();
        for ( auto i = 1; i < numPoses; i++ )
        {
            AnimationClip const* pSecondaryAnimation = m_pAnimation->GetSecondaryAnimation( pResultBuffer->m_poses[i].GetSkeleton() );
            if ( pSecondaryAnimation != nullptr )
            {
                pSecondaryAnimation->GetPose( m_time, &pResultBuffer->m_poses[i], context.m_skeletonLOD, context.m_sampleFloatChannels );
            }
            else
            {
                pResultBuffer->m_poses[i].Reset( Pose::Init::None );
            }
        }

        MarkTaskComplete( context );
    }

    void SampleTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteResourcePtr( m_pAnimation );
        serializer.WriteNormalizedFloat16Bit( m_time );
    }

    void SampleTask::Deserialize( TaskSerializer& serializer )
    {
        m_pAnimation = serializer.ReadResourcePtr<AnimationClip>();
        m_time = serializer.ReadNormalizedFloat16Bit();
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString SampleTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        float const percentageThrough = m_time.ToFloat() * 100;
        float const timeFrames = m_pAnimation->GetFrameTime( m_time ).ToFloat();

        InlineString str;
        str.sprintf( "%s - %.1f%%, frame %.1f", m_pAnimation->GetResourceID().GetFilenameWithoutExtension().c_str(), percentageThrough, timeFrames, m_pAnimation->IsAdditive() ? " - (Additive)" : "" );
        return str;
    }
    #endif
}
#include "Animation_Task_Sample.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    SampleTask::SampleTask( AnimationClip const* pAnimation, Percentage time )
        : Task()
        , m_pAnimation( pAnimation )
        , m_time( time )
    {
        EE_ASSERT( m_pAnimation != nullptr );
    }

    void SampleTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        EE_ASSERT( m_pAnimation != nullptr );

        auto pResultBuffer = GetNewPoseBuffer( context );

        // Sample primary pose
        //-------------------------------------------------------------------------

        m_pAnimation->GetPose( m_time, pResultBuffer->GetPrimaryPose(), context.m_skeletonLOD );

        // Sample secondary poses
        //-------------------------------------------------------------------------

        int32_t const numPoses = (int32_t) pResultBuffer->m_poses.size();
        for ( auto i = 1; i < numPoses; i++ )
        {
            AnimationClip const* pSecondaryAnimation = m_pAnimation->GetSecondaryAnimation( pResultBuffer->m_poses[i].GetSkeleton() );
            if ( pSecondaryAnimation != nullptr )
            {
                pSecondaryAnimation->GetPose( m_time, &pResultBuffer->m_poses[i], context.m_skeletonLOD );
            }
            else
            {
                pResultBuffer->m_poses[i].Reset( Pose::Type::None );
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
    InlineString SampleTask::GetDebugTextInfo() const
    {
        InlineString str;
        str.sprintf( "%s %s", m_pAnimation->GetResourceID().GetFilenameWithoutExtension().c_str(), m_pAnimation->IsAdditive() ? "(Additive)" : "" );
        return str;
    }
    #endif
}
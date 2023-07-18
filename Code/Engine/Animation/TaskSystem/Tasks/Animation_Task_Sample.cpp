#include "Animation_Task_Sample.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    SampleTask::SampleTask( TaskSourceID sourceID, AnimationClip const* pAnimation, Percentage time )
        : Task( sourceID )
        , m_pAnimation( pAnimation )
        , m_time( time )
    {
        EE_ASSERT( m_pAnimation != nullptr );
    }

    void SampleTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        EE_ASSERT( m_pAnimation != nullptr );

        auto pResultBuffer = GetNewPoseBuffer( context );
        m_pAnimation->GetPose( m_time, &pResultBuffer->m_pose );
        MarkTaskComplete( context );
    }

    void SampleTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteResourcePtr( m_pAnimation );
        serializer.WriteNormalizedFloat( m_time );
    }

    void SampleTask::Deserialize( TaskSerializer& serializer )
    {
        m_pAnimation = serializer.ReadResourcePtr<AnimationClip>();
        m_time = serializer.ReadNormalizedFloat();
    }

    #if EE_DEVELOPMENT_TOOLS
    String SampleTask::GetDebugText() const
    {
        if ( m_pAnimation->IsAdditive() )
        {
            return String( String::CtorSprintf(), "Sample (Additive): %s, %.2f%%, Fr: %.2f", m_pAnimation->GetResourceID().GetFileNameWithoutExtension().c_str(), (float) m_time * 100, m_pAnimation->GetFrameTime( m_time ).ToFloat() );
        }
        else
        {
            return String( String::CtorSprintf(), "Sample: %s, %.2f%%, Fr: %.2f", m_pAnimation->GetResourceID().GetFileNameWithoutExtension().c_str(), (float) m_time * 100, m_pAnimation->GetFrameTime( m_time ).ToFloat() );
        }
    }
    #endif
}
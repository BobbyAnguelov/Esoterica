#include "Animation_Task_Sample.h"

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
        EE_ASSERT( m_pAnimation != nullptr );

        auto pResultBuffer = GetNewPoseBuffer( context );
        m_pAnimation->GetPose( m_time, &pResultBuffer->m_pose );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    String SampleTask::GetDebugText() const
    {
        if ( m_pAnimation->IsAdditive() )
        {
            return String( String::CtorSprintf(), "Sample (Additive): %s, %.2f%%", m_pAnimation->GetResourceID().GetFileNameWithoutExtension().c_str(), (float) m_time * 100);
        }
        else
        {
            return String( String::CtorSprintf(), "Sample: %s, %.2f%%", m_pAnimation->GetResourceID().GetFileNameWithoutExtension().c_str(), (float) m_time * 100 );
        }
    }
    #endif
}
#include "Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    DefaultPoseTask::DefaultPoseTask( TaskSourceID sourceID, Pose::Type type )
        : Task( sourceID )
        , m_type( type )
    {}

    void DefaultPoseTask::Execute( TaskContext const& context )
    {
        auto pResultBuffer = GetNewPoseBuffer( context );
        pResultBuffer->m_pose.Reset( m_type );
        MarkTaskComplete( context );
    }
}
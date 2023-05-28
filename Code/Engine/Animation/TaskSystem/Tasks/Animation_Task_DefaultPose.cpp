#include "Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"

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

    void DefaultPoseTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteUInt( (uint32_t) m_type, 3 );
    }

    void DefaultPoseTask::Deserialize( TaskSerializer& serializer )
    {
        m_type = (Pose::Type) serializer.ReadUInt( 3 );
    }
}
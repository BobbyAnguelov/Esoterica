#include "Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    void ReferencePoseTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        auto pResultBuffer = GetNewPoseBuffer( context );
        pResultBuffer->ResetPose( Pose::Type::ReferencePose );
        MarkTaskComplete( context );
    }

    //-------------------------------------------------------------------------

    void ZeroPoseTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        auto pResultBuffer = GetNewPoseBuffer( context );
        pResultBuffer->ResetPose( Pose::Type::ZeroPose );
        MarkTaskComplete( context );
    }
}
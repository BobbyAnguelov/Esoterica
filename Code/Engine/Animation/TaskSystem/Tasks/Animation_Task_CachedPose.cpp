#include "Animation_Task_CachedPose.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    CachedPoseWriteTask::CachedPoseWriteTask( int8_t sourceTaskIdx, UUID cachedPoseID )
        : Task( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_cachedPoseID( cachedPoseID )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_cachedPoseID.IsValid() );
    }

    void CachedPoseWriteTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        auto pCachedPoseBuffer = context.m_posePool.GetCachedPoseBuffer( m_cachedPoseID );
        PoseBuffer const* pPoseBuffer = TransferDependencyPoseBuffer( context, 0 );
        EE_ASSERT( pPoseBuffer->IsPoseSet() );
        pCachedPoseBuffer->CopyFrom( pPoseBuffer );
        EE_ASSERT( pCachedPoseBuffer->IsPoseSet() );
        MarkTaskComplete( context );
    }

    //-------------------------------------------------------------------------

    CachedPoseReadTask::CachedPoseReadTask( UUID cachedPoseID )
        : Task()
        , m_cachedPoseID( cachedPoseID )
    {
        EE_ASSERT( m_cachedPoseID.IsValid() );
    }

    void CachedPoseReadTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();
        auto pCachedPoseBuffer = context.m_posePool.GetCachedPoseBuffer( m_cachedPoseID );
        auto pPoseBuffer = GetNewPoseBuffer( context );

        // If the cached buffer contains a valid pose copy it
        if ( pCachedPoseBuffer->IsPoseSet() )
        {
            pPoseBuffer->CopyFrom( pCachedPoseBuffer );
        }
        else // Set the result buffer to reference
        {
            pPoseBuffer->ResetPose( Pose::Type::ReferencePose );
        }

        MarkTaskComplete( context );
    }
}
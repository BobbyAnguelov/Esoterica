#include "Animation_Task_CachedPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    CachedPoseWriteTask::CachedPoseWriteTask( int8_t sourceTaskIdx, CachedPoseID cachedPoseID )
        : Task( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_cachedPoseID( cachedPoseID )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_cachedPoseID.IsValid() );
    }

    void CachedPoseWriteTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        // Transfer the dependency buffer
        PoseBuffer const* pPoseBuffer = TransferDependencyPoseBuffer( context, 0 );
        EE_ASSERT( pPoseBuffer->IsPoseSet() );

        // Get a cached buffer so we can copy the current pose
        PoseBuffer* pCachedPoseBuffer = nullptr;
        if ( m_isDeserializedTask )
        {
            pCachedPoseBuffer = context.m_posePool.GetOrCreateCachedPoseBuffer( m_cachedPoseID, true );
        }
        else [[likely]]
        {
            pCachedPoseBuffer = context.m_posePool.GetCachedPoseBuffer( m_cachedPoseID );
        }

        // Make a copy of the transferred buffer
        EE_ASSERT( pCachedPoseBuffer != nullptr );
        pCachedPoseBuffer->CopyFrom( pPoseBuffer );
        EE_ASSERT( pCachedPoseBuffer->IsPoseSet() );

        MarkTaskComplete( context );
    }

    void CachedPoseWriteTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteUInt( m_cachedPoseID.m_ID, CachedPoseID::s_requiredBitsToSerialize );
    }

    void CachedPoseWriteTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 1 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_cachedPoseID.m_ID = (uint8_t) serializer.ReadUInt( CachedPoseID::s_requiredBitsToSerialize );
        EE_ASSERT( m_cachedPoseID.IsValid() );
        m_isDeserializedTask = true;
    }

    //-------------------------------------------------------------------------

    CachedPoseReadTask::CachedPoseReadTask( CachedPoseID cachedPoseID )
        : Task()
        , m_cachedPoseID( cachedPoseID )
    {
        EE_ASSERT( m_cachedPoseID.IsValid() );
    }

    void CachedPoseReadTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        auto pPoseBuffer = GetNewPoseBuffer( context );
        auto pCachedPoseBuffer = context.m_posePool.GetCachedPoseBuffer( m_cachedPoseID );

        // In normal operation, the cached pose buffer CANNOT be null!
        if ( !m_isDeserializedTask )
        {
            EE_ASSERT( pCachedPoseBuffer != nullptr );
        }

        // If the cached buffer contains a valid pose copy it
        if ( pCachedPoseBuffer != nullptr && pCachedPoseBuffer->IsPoseSet() )
        {
            pPoseBuffer->CopyFrom( pCachedPoseBuffer );
        }
        else // Clear the result buffer
        {
            pPoseBuffer->ResetPose( Pose::Type::None );
        }

        MarkTaskComplete( context );
    }

    void CachedPoseReadTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteUInt( m_cachedPoseID.m_ID, CachedPoseID::s_requiredBitsToSerialize );
    }

    void CachedPoseReadTask::Deserialize( TaskSerializer& serializer )
    {
        m_cachedPoseID.m_ID = (uint8_t) serializer.ReadUInt( CachedPoseID::s_requiredBitsToSerialize );
        EE_ASSERT( m_cachedPoseID.IsValid() );
        m_isDeserializedTask = true;
    }
}
#include "Animation_TaskPosePool.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    //-------------------------------------------------------------------------
    // Pose Buffer
    //-------------------------------------------------------------------------

    PoseBuffer::PoseBuffer( Skeleton const* pSkeleton, SecondarySkeletonList const& secondarySkeletons )
    {
        m_poses.emplace_back( pSkeleton );
        UpdateSecondarySkeletonList( secondarySkeletons );
    }

    void PoseBuffer::ResetPose( Pose::Init poseInit, bool calculateGlobalPose )
    {
        for ( Pose& pose : m_poses )
        {
            pose.Reset( poseInit, calculateGlobalPose );
        }
    }

    void PoseBuffer::Release( Pose::Init poseInit, bool calculateGlobalPose )
    {
        ResetPose( poseInit, calculateGlobalPose );
        m_isUsed = false;
    }

    void PoseBuffer::CalculateModelSpaceTransforms()
    {
        for ( Pose& pose : m_poses )
        {
            pose.CalculateModelSpaceTransforms();
        }
    }

    Pose *PoseBuffer::GetSecondaryPose( Skeleton const *pSkeleton )
    {
        int32_t const numPoses = (int32_t) m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            if ( m_poses[poseIdx].GetSkeleton() == pSkeleton )
            {
                return &m_poses[poseIdx];
            }
        }

        return nullptr;
    }

    void PoseBuffer::CopyFrom( PoseBuffer const& rhs )
    {
        EE_ASSERT( rhs.m_poses.size() == m_poses.size() );

        int8_t const numPoses = (int8_t) m_poses.size();
        for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
        {
            EE_ASSERT( m_poses[poseIdx].GetSkeleton() == rhs.m_poses[poseIdx].GetSkeleton());
            m_poses[poseIdx].CopyFrom( rhs.m_poses[poseIdx] );
        }
    }

    void PoseBuffer::UpdateSecondarySkeletonList( SecondarySkeletonList const& secondarySkeletons )
    {
        int32_t const numExistingSecondarySkeletons = int32_t( m_poses.size() ) - 1;
        int32_t const numRequiredSecondarySkeletons = int32_t( secondarySkeletons.size() );
        int32_t const numSecondarySkeletonsToUpdate = Math::Min( numRequiredSecondarySkeletons, numExistingSecondarySkeletons );

        for ( int32_t secondarySkeletonIdx = 0; secondarySkeletonIdx < numSecondarySkeletonsToUpdate; secondarySkeletonIdx++ )
        {
            // If the skeleton differs, then change it
            if ( m_poses[secondarySkeletonIdx + 1].GetSkeleton() != secondarySkeletons[secondarySkeletonIdx] )
            {
                m_poses[secondarySkeletonIdx + 1].ChangeSkeleton( secondarySkeletons[secondarySkeletonIdx] );
            }
        }

        // If we need less poses, then just destroy the extras
        if ( numExistingSecondarySkeletons > numRequiredSecondarySkeletons )
        {
            int32_t const numPosesToRemove = numExistingSecondarySkeletons - numRequiredSecondarySkeletons;
            for( int32_t i = 0; i < numPosesToRemove; i++ )
            {
                m_poses.pop_back();
            }
        }
        // If we need more poses, then create the excess
        else if( numExistingSecondarySkeletons < numRequiredSecondarySkeletons )
        {
            for ( int32_t secondarySkeletonIdx = numExistingSecondarySkeletons; secondarySkeletonIdx < numRequiredSecondarySkeletons; secondarySkeletonIdx++ )
            {
                m_poses.emplace_back( secondarySkeletons[secondarySkeletonIdx] );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Cached Pose Buffer
    //-------------------------------------------------------------------------

    void CachedPoseBuffer::Release( Pose::Init poseInit )
    {
        PoseBuffer::Release( poseInit );
        m_ID.Clear();
        m_isPendingDestroy = false;
        m_wasAccessed = false;
    }

    //-------------------------------------------------------------------------
    // Pose Buffer Pool
    //-------------------------------------------------------------------------

    PoseBufferPool::PoseBufferPool( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons )
        : m_pPrimarySkeleton( pPrimarySkeleton )
        , m_secondarySkeletons( secondarySkeletons )
    {
        EE_ASSERT( m_pPrimarySkeleton != nullptr );
        EE_ASSERT( Skeleton::ValidateSkeletonSetup( m_pPrimarySkeleton, secondarySkeletons ) );

        //-------------------------------------------------------------------------

        for ( auto i = 0; i < s_numInitialBuffers; i++ )
        {
            m_poseBuffers.emplace_back( PoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );
            m_cachedBuffers.emplace_back( CachedPoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );

            #if EE_DEVELOPMENT_TOOLS
            m_debugPoseBuffers.emplace_back( PoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );
            m_debugBufferTaskIdxMapping.emplace_back( int8_t( 0 ) );
            #endif
        }
    }

    PoseBufferPool::~PoseBufferPool()
    {
        Reset();
    }

    void PoseBufferPool::ResetInternal( bool resetForNewUpdate )
    {
        // Reset all buffers
        for ( auto& poseBuffer : m_poseBuffers )
        {
            poseBuffer.Release();
        }

        m_firstFreeBuffer = 0;

        if ( resetForNewUpdate )
        {
            // Update cached buffer states
            int8_t const numCachedBuffers = (int8_t) m_cachedBuffers.size();
            for ( int8_t i = 0; i < numCachedBuffers; i++ )
            {
                // Persistent buffers are ignored
                if ( m_cachedBuffers[i].m_isPersistent )
                {
                    continue;
                }

                // Release any used but unaccessed buffers
                if ( m_cachedBuffers[i].m_isUsed )
                {
                    // Release any buffers that are no longer needed
                    if ( m_cachedBuffers[i].m_isPendingDestroy && !m_cachedBuffers[i].m_wasAccessed )
                    {
                        m_cachedBuffers[i].Release();
                        m_firstFreeCachedBuffer = Math::Min( m_firstFreeCachedBuffer, i );
                    }
                    else
                    {
                        m_cachedBuffers[i].m_wasAccessed = false;
                    }
                }
            }
        }
        else // Full reset
        {
            for ( auto& cachedBuffer : m_cachedBuffers )
            {
                if ( cachedBuffer.m_isPersistent )
                {
                    continue;
                }

                cachedBuffer.Release();
            }

            m_nextCachedPoseID.Clear();

            // Find the first free buffer
            int32_t const numCachedBuffers = (int32_t) m_cachedBuffers.size();
            for ( m_firstFreeCachedBuffer = 0; m_firstFreeCachedBuffer < numCachedBuffers; m_firstFreeCachedBuffer++ )
            {
                if ( !m_cachedBuffers[m_firstFreeCachedBuffer].m_isUsed )
                {
                    break;
                }
            }
        }

        //-------------------------------------------------------------------------

        // Dont reset the actual debug buffers as we want to still access them this frame, only reset the free index
        #if EE_DEVELOPMENT_TOOLS
        m_firstFreeDebugBuffer = 0;
        for ( auto& taskIdx : m_debugBufferTaskIdxMapping ) { taskIdx = InvalidIndex; }
        #endif
    }

    void PoseBufferPool::SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons )
    {
        EE_ASSERT( Skeleton::ValidateSkeletonSetup( m_pPrimarySkeleton, secondarySkeletons ) );

        m_secondarySkeletons = secondarySkeletons;

        for ( PoseBuffer& poseBuffer : m_poseBuffers )
        {
            poseBuffer.UpdateSecondarySkeletonList( m_secondarySkeletons );
        }

        #if EE_DEVELOPMENT_TOOLS
        for ( PoseBuffer& debugPoseBuffer : m_debugPoseBuffers )
        {
            debugPoseBuffer.UpdateSecondarySkeletonList( m_secondarySkeletons );
        }
        #endif
    }

    int32_t PoseBufferPool::GetPoseIndexForSkeleton( Skeleton const* pSkeleton ) const
    {
        EE_ASSERT( pSkeleton != nullptr );

        int32_t poseIdx = InvalidIndex;
        int32_t const numSecondarySkeletons = (int32_t) m_secondarySkeletons.size();
        for ( int32_t i = 0; i < numSecondarySkeletons; i++ )
        {
            if ( m_secondarySkeletons[i] == pSkeleton )
            {
                poseIdx = i;
                break;
            }
        }

        return poseIdx;
    }

    int8_t PoseBufferPool::RequestPoseBuffer()
    {
        if ( m_firstFreeBuffer == m_poseBuffers.size() )
        {
            for ( auto i = 0; i < s_bufferGrowAmount; i++ )
            {
                m_poseBuffers.emplace_back( PoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );
            }
            EE_ASSERT( m_poseBuffers.size() < INT8_MAX );
        }

        int8_t const freeBufferIdx = m_firstFreeBuffer;
        EE_ASSERT( !m_poseBuffers[freeBufferIdx].m_isUsed );
        m_poseBuffers[freeBufferIdx].m_isUsed = true;

        // Update free index
        int8_t const numPoseBuffers = (int8_t) m_poseBuffers.size();
        for ( ; m_firstFreeBuffer < numPoseBuffers; m_firstFreeBuffer++ )
        {
            if ( !m_poseBuffers[m_firstFreeBuffer].m_isUsed )
            {
                break;
            }
        }

        return freeBufferIdx;
    }

    void PoseBufferPool::ReleasePoseBuffer( int8_t bufferIdx )
    {
        EE_ASSERT( m_poseBuffers[bufferIdx].m_isUsed );
        m_poseBuffers[bufferIdx].m_isUsed = false;
        m_firstFreeBuffer = Math::Min( bufferIdx, m_firstFreeBuffer );
    }

    //-------------------------------------------------------------------------

    bool PoseBufferPool::IsValidCachedPose( CachedPoseID cachedPoseID ) const
    {
        for ( auto& cachedBuffer : m_cachedBuffers )
        {
            if ( cachedBuffer.m_ID == cachedPoseID )
            {
                return true;
            }
        }

        return false;
    }

    CachedPoseID PoseBufferPool::CreatePersistentCachedPoseBuffer()
    {
        auto pPoseBuffer = CreateCachedPoseBufferInternal();
        pPoseBuffer->m_isPersistent = true;
        return pPoseBuffer->m_ID;
    }

    CachedPoseID PoseBufferPool::CreateCachedPoseBuffer()
    {
        return CreateCachedPoseBufferInternal()->m_ID;
    }

    CachedPoseBuffer* PoseBufferPool::CreateCachedPoseBufferInternal( CachedPoseID bufferID )
    {
        CachedPoseBuffer* pCachedPoseBuffer = nullptr;

        // If we are asking to create a cached pose for a specific ID, that ID MUST not be in use
        if ( bufferID.IsValid() )
        {
            EE_ASSERT( !IsValidCachedPose( bufferID ) );
        }

        // Find/create a free buffer
        //-------------------------------------------------------------------------

        if ( m_firstFreeCachedBuffer == m_cachedBuffers.size() )
        {
            for ( auto i = 0; i < s_bufferGrowAmount; i++ )
            {
                pCachedPoseBuffer = &m_cachedBuffers.emplace_back( CachedPoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );
            }

            EE_ASSERT( m_cachedBuffers.size() < INT8_MAX );
        }
        else
        {
            pCachedPoseBuffer = &m_cachedBuffers[m_firstFreeCachedBuffer];
            EE_ASSERT( !pCachedPoseBuffer->m_isUsed );
        }

        // Create a new ID for the cached pose
        //-------------------------------------------------------------------------

        if ( bufferID.IsValid() )
        {
            EE_ASSERT( !IsValidCachedPose( bufferID ) );
            pCachedPoseBuffer->m_ID = bufferID;
            m_nextCachedPoseID = bufferID.m_ID;
        }
        else [[likely]] // Generate a new ID
        {
            pCachedPoseBuffer->m_ID = GenerateNewCachedPoseID();
        }

        // We can only allow 64 cached poses due to the serialization limits on the ID.
        // This limit is really high and we should NEVER need 64 cached poses at a given time.
        // There is no way to gracefully handle this so just crash!
        EE_ASSERT( pCachedPoseBuffer->m_ID.IsValid() );

        // Update free buffer index
        //-------------------------------------------------------------------------

        pCachedPoseBuffer->m_isUsed = true;

        int32_t const numCachedBuffers = (int32_t) m_cachedBuffers.size();
        for ( ; m_firstFreeCachedBuffer < numCachedBuffers; m_firstFreeCachedBuffer++ )
        {
            if ( !m_cachedBuffers[m_firstFreeCachedBuffer].m_isUsed )
            {
                break;
            }
        }

        // Flag as accessed
        pCachedPoseBuffer->m_wasAccessed = true;

        return pCachedPoseBuffer;
    }

    CachedPoseID PoseBufferPool::GenerateNewCachedPoseID()
    {
        if ( !m_nextCachedPoseID.IsValid() )
        {
            m_nextCachedPoseID = 0;
        }

        // Ensure the new ID is unused
        int32_t numBuffersChecked = 0;
        while ( true )
        {
            bool isUsedID = false;
            for ( auto const &buffer : m_cachedBuffers )
            {
                if ( buffer.m_isUsed && buffer.m_ID == m_nextCachedPoseID )
                {
                    isUsedID = true;
                    break;
                }
            }

            if ( isUsedID )
            {
                m_nextCachedPoseID++;
                numBuffersChecked++;
                EE_ASSERT( numBuffersChecked < CachedPoseID::s_maxNumberOfCachedPoses );
            }
            else
            {
                break;
            }
        }

        // Return the free ID and increment the next ID tracker
        CachedPoseID const ID = m_nextCachedPoseID;
        m_nextCachedPoseID++;
        return ID;
    }

    void PoseBufferPool::DestroyCachedPoseBuffer( CachedPoseID cachedPoseID )
    {
        EE_ASSERT( cachedPoseID.IsValid() );

        for ( auto& cachedBuffer : m_cachedBuffers )
        {
            if ( cachedBuffer.m_ID == cachedPoseID )
            {
                EE_ASSERT( cachedBuffer.m_isUsed && !cachedBuffer.m_isPersistent );

                // Cached buffer destruction is deferred to the first frame where we have stopped accessing the buffer since we may already have tasks reading from it already
                // Transfer the ownership to the cache pose pool so the buffer lifetime is based on its usage
                cachedBuffer.m_isPendingDestroy = true;
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    CachedPoseBuffer* PoseBufferPool::GetCachedPoseBufferInternal( CachedPoseID cachedPoseID )
    {
        EE_ASSERT( cachedPoseID.IsValid() );
        CachedPoseBuffer* pFoundCachedPoseBuffer = nullptr;

        for ( auto& cachedBuffer : m_cachedBuffers )
        {
            if ( cachedBuffer.m_ID == cachedPoseID )
            {
                pFoundCachedPoseBuffer = &cachedBuffer;
                cachedBuffer.m_wasAccessed = true;
                break;
            }
        }

        return pFoundCachedPoseBuffer;
    }

    PoseBuffer* PoseBufferPool::CreateBufferForSpecificID( CachedPoseID cachedPoseID )
    {
        EE_ASSERT( cachedPoseID.IsValid() );
        return CreateCachedPoseBufferInternal( cachedPoseID );
    }

    PoseBuffer* PoseBufferPool::GetOrCreateTemporaryBufferForSpecificID( CachedPoseID cachedPoseID )
    {
        EE_ASSERT( cachedPoseID.IsValid() );

        auto pBuffer = GetCachedPoseBufferInternal( cachedPoseID );
        if ( pBuffer == nullptr )
        {
            pBuffer = CreateCachedPoseBufferInternal( cachedPoseID );
            pBuffer->m_isPendingDestroy = true;
        }

        return pBuffer;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PoseBufferPool::RecordPose( int8_t taskIdx, int8_t poseBufferIdx )
    {
        if ( !m_isDebugRecordingEnabled )
        {
            return;
        }

        // If we are out of buffers, add additional debug buffers
        if ( m_firstFreeDebugBuffer == m_debugPoseBuffers.size() )
        {
            for ( auto i = 0; i < s_bufferGrowAmount; i++ )
            {
                m_debugPoseBuffers.emplace_back( m_pPrimarySkeleton, m_secondarySkeletons );
                m_debugBufferTaskIdxMapping.emplace_back( int8_t( -1 ) );
            }

            EE_ASSERT( m_debugPoseBuffers.size() < INT8_MAX );
        }

        EE_ASSERT( m_poseBuffers[poseBufferIdx].m_isUsed );
        m_debugPoseBuffers[m_firstFreeDebugBuffer].CopyFrom( m_poseBuffers[poseBufferIdx] );
        m_debugBufferTaskIdxMapping[m_firstFreeDebugBuffer] = taskIdx;
        m_firstFreeDebugBuffer++;
    }

    bool PoseBufferPool::HasRecordedPoseBufferForTask( int8_t nTaskIdx ) const
    {
        for ( size_t i = 0; i < m_debugBufferTaskIdxMapping.size(); i++ )
        {
            if ( m_debugBufferTaskIdxMapping[i] == nTaskIdx )
            {
                return true;
            }
        }
        return false;
    }

    PoseBuffer* PoseBufferPool::GetRecordedPoseBufferForTask( int8_t taskIdx ) const
    {
        EE_ASSERT( taskIdx >= 0 && taskIdx < m_debugBufferTaskIdxMapping.size() );

        for ( auto i = 0u; i < m_debugBufferTaskIdxMapping.size(); i++ )
        {
            if ( m_debugBufferTaskIdxMapping[i] == taskIdx )
            {
                return const_cast<PoseBuffer*>( &m_debugPoseBuffers[i] );
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }
    #endif
}
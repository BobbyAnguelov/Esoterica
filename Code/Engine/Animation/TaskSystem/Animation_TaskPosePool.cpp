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

    void PoseBuffer::ResetPose( Pose::Type poseType, bool calculateGlobalPose )
    {
        for ( Pose& pose : m_poses )
        {
            pose.Reset( poseType, calculateGlobalPose );
        }
    }

    void PoseBuffer::Release( Pose::Type poseType, bool calculateGlobalPose )
    {
        ResetPose( poseType, calculateGlobalPose );
        m_isUsed = false;
    }

    void PoseBuffer::CalculateModelSpaceTransforms()
    {
        for ( Pose& pose : m_poses )
        {
            pose.CalculateModelSpaceTransforms();
        }
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
        int32_t const numSkeletonsToUpdate = Math::Min( numRequiredSecondarySkeletons, numExistingSecondarySkeletons );

        for ( int32_t i = 1; i < numSkeletonsToUpdate; i++ )
        {
            // If the skeleton differs, then change it
            if ( m_poses[i].GetSkeleton() != secondarySkeletons[i - 1] )
            {
                m_poses[i].ChangeSkeleton( secondarySkeletons[i - 1] );
            }
        }

        // If we need less poses, then just destroy the extras
        if ( numExistingSecondarySkeletons > numRequiredSecondarySkeletons )
        {
            while( m_poses.size() > numRequiredSecondarySkeletons )
            {
                m_poses.pop_back();
            }
        }
        // If we need more poses, then create the excess
        else if( numExistingSecondarySkeletons < numRequiredSecondarySkeletons )
        {
            for ( int32_t i = numExistingSecondarySkeletons; i < numRequiredSecondarySkeletons; i++ )
            {
                m_poses.emplace_back( secondarySkeletons[i] );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Cached Pose Buffer
    //-------------------------------------------------------------------------

    void CachedPoseBuffer::Release( Pose::Type poseType )
    {
        PoseBuffer::Release( poseType );
        m_ID.Clear();
        m_isLifetimeInternallyManaged = false;
        m_wasAccessed = false;
        m_shouldBeReset = false;
    }

    //-------------------------------------------------------------------------
    // Pose Buffer Pool
    //-------------------------------------------------------------------------

    PoseBufferPool::PoseBufferPool( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons )
        : m_pPrimarySkeleton( pPrimarySkeleton )
        , m_secondarySkeletons( secondarySkeletons )
    {
        EE_ASSERT( m_pPrimarySkeleton != nullptr );

        #if EE_DEVELOPMENT_TOOLS
        ValidateSetOfSecondarySkeletons( m_pPrimarySkeleton, secondarySkeletons );
        #endif

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

    void PoseBufferPool::Reset()
    {
        // Reset all buffers
        for ( auto& poseBuffer : m_poseBuffers )
        {
            poseBuffer.Release();
        }

        m_firstFreeBuffer = 0;

        // Update cached buffer states
        int8_t const numCachedBuffers = (int8_t) m_cachedBuffers.size();
        for ( int8_t i = 0; i < numCachedBuffers; i++ )
        {
            // Release any used but unaccessed buffers
            if ( m_cachedBuffers[i].m_isUsed )
            {
                if ( m_cachedBuffers[i].m_isLifetimeInternallyManaged && !m_cachedBuffers[i].m_wasAccessed )
                {
                    m_cachedBuffers[i].m_shouldBeReset = true;
                }
                else
                {
                    m_cachedBuffers[i].m_wasAccessed = false;
                }
            }

            // Release any buffers that are no longer needed
            if ( m_cachedBuffers[i].m_shouldBeReset )
            {
                EE_ASSERT( m_cachedBuffers[i].m_isUsed );
                m_cachedBuffers[i].Release();
                m_firstFreeCachedBuffer = Math::Min( m_firstFreeCachedBuffer, i );
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
        #if EE_DEVELOPMENT_TOOLS
        ValidateSetOfSecondarySkeletons( m_pPrimarySkeleton, secondarySkeletons );
        #endif

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
            EE_ASSERT( m_poseBuffers.size() < 255 );
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

    CachedPoseID PoseBufferPool::CreateCachedPoseBuffer()
    {
        return CreateCachedPoseBufferInternal()->m_ID;
    }

    CachedPoseBuffer* PoseBufferPool::CreateCachedPoseBufferInternal( CachedPoseID bufferID )
    {
        CachedPoseBuffer* pCachedPoseBuffer = nullptr;

        // Find/create a free buffer
        //-------------------------------------------------------------------------

        if ( m_firstFreeCachedBuffer == m_cachedBuffers.size() )
        {
            for ( auto i = 0; i < s_bufferGrowAmount; i++ )
            {
                pCachedPoseBuffer = &m_cachedBuffers.emplace_back( CachedPoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );
            }

            EE_ASSERT( m_cachedBuffers.size() < 255 );
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
            pCachedPoseBuffer->m_ID = CachedPoseID( m_nextCachedPoseID );
        }

        // Update ID
        //-------------------------------------------------------------------------

        m_nextCachedPoseID++;

        // If we need to wrap around the IDs
        if ( m_nextCachedPoseID >= CachedPoseID::s_maxAllowableValue )
        {
            m_nextCachedPoseID = 0;
        }

        // Ensure the new ID is unused
        while ( true )
        {
            bool isUsedID = false;
            for ( auto const& buffer : m_cachedBuffers )
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
            }
            else
            {
                break;
            }
        }

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

    void PoseBufferPool::DestroyCachedPoseBuffer( CachedPoseID cachedPoseID )
    {
        EE_ASSERT( cachedPoseID.IsValid() );

        for ( auto& cachedBuffer : m_cachedBuffers )
        {
            if ( cachedBuffer.m_ID == cachedPoseID )
            {
                // Cached buffer destruction is deferred to the next frame since we may already have tasks reading from it already
                cachedBuffer.m_shouldBeReset = true;
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void PoseBufferPool::ResetCachedPoseBuffer( CachedPoseID cachedPoseID )
    {
        for ( auto& cachedBuffer : m_cachedBuffers )
        {
            if ( cachedBuffer.m_ID == cachedPoseID )
            {
                // Cached buffer destruction is deferred to the next frame since we may already have tasks reading from it already
                cachedBuffer.Release();
                return;
            }
        }
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

    PoseBuffer* PoseBufferPool::GetOrCreateCachedPoseBuffer( CachedPoseID cachedPoseID, bool isLifetimeManagedByPosePool )
    {
        CachedPoseBuffer* pBuffer = GetCachedPoseBufferInternal( cachedPoseID );
        if ( pBuffer == nullptr )
        {
            pBuffer = CreateCachedPoseBufferInternal( cachedPoseID );
            pBuffer->m_isLifetimeInternallyManaged = isLifetimeManagedByPosePool;
        }
        return pBuffer;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PoseBufferPool::ValidateSetOfSecondarySkeletons( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons )
    {
        EE_ASSERT( pPrimarySkeleton != nullptr );

        int32_t const numSkeletons = (int32_t) secondarySkeletons.size();
        for ( auto i = 0; i < numSkeletons; i++ )
        {
            EE_ASSERT( secondarySkeletons[i] != nullptr );
            EE_ASSERT( secondarySkeletons[i] != pPrimarySkeleton );

            for ( auto j = i + 1; j < numSkeletons; j++ )
            {
                EE_ASSERT( secondarySkeletons[j] != secondarySkeletons[i] );
            }
        }
    }

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
                m_debugPoseBuffers.emplace_back( PoseBuffer( m_pPrimarySkeleton, m_secondarySkeletons ) );
                m_debugBufferTaskIdxMapping.emplace_back( int8_t( -1 ) );
            }

            EE_ASSERT( m_debugPoseBuffers.size() < 255 );
        }

        EE_ASSERT( m_poseBuffers[poseBufferIdx].m_isUsed );
        m_debugPoseBuffers[m_firstFreeDebugBuffer].CopyFrom( m_poseBuffers[poseBufferIdx] );
        m_debugBufferTaskIdxMapping[m_firstFreeDebugBuffer] = taskIdx;
        m_firstFreeDebugBuffer++;
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
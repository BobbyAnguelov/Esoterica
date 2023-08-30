#pragma once

#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINE_API PoseBuffer
    {
        friend class PoseBufferPool;

    public:

        PoseBuffer( Skeleton const* pSkeleton );

        void Reset();

        void CopyFrom( PoseBuffer const& RHS );
        inline void CopyFrom( PoseBuffer const* RHS ) { CopyFrom( *RHS ); }

    public:

        Pose								m_pose;

    private:

        bool								m_isUsed = false;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API CachedPoseBuffer : public PoseBuffer
    {
        friend class PoseBufferPool;

    public:

        using PoseBuffer::PoseBuffer;

        inline void Reset()
        {
            PoseBuffer::Reset();
            m_ID.Clear();
            m_shouldBeDestroyed = false;
        }

    public:

        UUID							m_ID;
        bool							m_shouldBeDestroyed = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PoseBufferPool
    {
        constexpr static int8_t const s_numInitialBuffers = 6;
        constexpr static int8_t const s_bufferGrowAmount = 3;

    public:

        PoseBufferPool( Skeleton const* pSkeleton );
        PoseBufferPool( PoseBufferPool const& ) = delete;
        ~PoseBufferPool();

        PoseBufferPool& operator=( PoseBufferPool const& rhs ) = delete;

        void Reset();

        // Poses
        //-------------------------------------------------------------------------

        int8_t RequestPoseBuffer();
        void ReleasePoseBuffer( int8_t bufferIdx );

        inline PoseBuffer* GetBuffer( int8_t bufferIdx )
        {
            EE_ASSERT( m_poseBuffers[bufferIdx].m_isUsed );
            return &m_poseBuffers[bufferIdx];
        }

        // Cached Poses
        //-------------------------------------------------------------------------

        UUID CreateCachedPoseBuffer();
        void DestroyCachedPoseBuffer( UUID const& cachedPoseID );
        void ResetCachedPoseBuffer( UUID const& cachedPoseID );
        PoseBuffer* GetCachedPoseBuffer( UUID const& cachedPoseID );

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline bool IsRecordingEnabled() const { return m_isDebugRecordingEnabled; }
        inline void EnableRecording( bool enabled ) const { m_isDebugRecordingEnabled = enabled; }
        inline bool HasRecordedData() const { return m_firstFreeDebugBuffer != 0; }
        void RecordPose( int8_t taskIdx, int8_t poseBufferIdx );
        Pose const* GetRecordedPoseForTask( int8_t taskIdx ) const;
        #endif

    private:

        Skeleton const*                             m_pSkeleton = nullptr;
        TInlineVector<PoseBuffer, 10>               m_poseBuffers;
        TInlineVector<CachedPoseBuffer, 10>         m_cachedBuffers;
        TInlineVector<UUID, 5>                      m_cachedPoseBuffersToDestroy;
        int8_t                                      m_firstFreeCachedBuffer = 0;
        int8_t                                      m_firstFreeBuffer = 0;

        #if EE_DEVELOPMENT_TOOLS
        TVector<Pose>                               m_debugBuffers;
        TVector<int8_t>                             m_debugBufferTaskIdxMapping; // The task index for each debug buffer
        int8_t                                      m_firstFreeDebugBuffer = 0;
        mutable bool                                m_isDebugRecordingEnabled = false;
        #endif
    };
}
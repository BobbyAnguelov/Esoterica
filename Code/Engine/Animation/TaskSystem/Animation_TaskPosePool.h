#pragma once

#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using SecondarySkeletonList = TInlineVector<Skeleton const*, 3>;

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API PoseBuffer
    {
        friend class PoseBufferPool;
        friend class TaskSystem;

    public:

        PoseBuffer( Skeleton const* pSkeleton, SecondarySkeletonList const& secondarySkeletons );

        // Data
        //-------------------------------------------------------------------------

        void CopyFrom( PoseBuffer const& RHS );
        inline void CopyFrom( PoseBuffer const* RHS ) { CopyFrom( *RHS ); }

        // Pose State
        //-------------------------------------------------------------------------

        inline bool IsPoseSet() const { return m_poses[0].IsPoseSet(); }
        inline bool IsAdditive() const { return m_poses[0].IsAdditivePose(); }
        inline Pose* GetPrimaryPose() { return &m_poses[0]; }
        void ResetPose( Pose::Type poseType = Pose::Type::None, bool calculateGlobalPose = false );
        void CalculateModelSpaceTransforms();

    protected:

        // Releases this buffer and resets the poses
        void Release( Pose::Type poseType = Pose::Type::None, bool calculateGlobalPose = false );

        // Changes the set of poses we store
        void UpdateSecondarySkeletonList( SecondarySkeletonList const& secondarySkeletons );
    
    public:

        TInlineVector<Pose, 10>             m_poses;

    private:

        bool                                m_isUsed = false;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API CachedPoseBuffer : public PoseBuffer
    {
        friend class PoseBufferPool;

    public:

        using PoseBuffer::PoseBuffer;

    private:

        inline void Release( Pose::Type poseType = Pose::Type::None )
        {
            PoseBuffer::Release( poseType );
            m_ID.Clear();
            m_shouldBeReset = false;
        }

    public:

        UUID                                m_ID;
        bool                                m_shouldBeReset = false;
    };

    //-------------------------------------------------------------------------
    // TODO: Store all poses in a contiguos block of memory and profile if this is in fact faster or not

    class EE_ENGINE_API PoseBufferPool
    {
        constexpr static int8_t const s_numInitialBuffers = 6;
        constexpr static int8_t const s_bufferGrowAmount = 3;

        #if EE_DEVELOPMENT_TOOLS
        static void ValidateSetOfSecondarySkeletons( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons );
        #endif

    public:

        PoseBufferPool( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons = SecondarySkeletonList() );
        PoseBufferPool( PoseBufferPool const& ) = delete;
        ~PoseBufferPool();

        PoseBufferPool& operator=( PoseBufferPool const& rhs ) = delete;

        void Reset();

        // Skeletons
        //-------------------------------------------------------------------------

        // Get the primary skeleton for this pool
        Skeleton const* GetPrimarySkeleton() const { return m_pPrimarySkeleton; }

        // Set the secondary skeletons that we can animate
        void SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons );

        // Get the number of secondary skeletons set
        inline int32_t GetNumSecondarySkeletons() const { return (int32_t) m_secondarySkeletons.size(); }

        // Get the index of the pose for a specific skeleton
        int32_t GetPoseIndexForSkeleton( Skeleton const* pSkeleton ) const;

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
        PoseBuffer* GetRecordedPoseBufferForTask( int8_t taskIdx ) const;
        #endif

    private:

        TInlineVector<PoseBuffer, 10>               m_poseBuffers;
        TInlineVector<CachedPoseBuffer, 10>         m_cachedBuffers;
        TInlineVector<UUID, 5>                      m_cachedPoseBuffersToDestroy;
        int8_t                                      m_firstFreeCachedBuffer = 0;
        int8_t                                      m_firstFreeBuffer = 0;

        Skeleton const*                             m_pPrimarySkeleton = nullptr;
        SecondarySkeletonList                       m_secondarySkeletons;

        #if EE_DEVELOPMENT_TOOLS
        TVector<PoseBuffer>                         m_debugPoseBuffers;
        TVector<int8_t>                             m_debugBufferTaskIdxMapping; // The task index for each debug buffer
        int8_t                                      m_firstFreeDebugBuffer = 0;
        mutable bool                                m_isDebugRecordingEnabled = false;
        #endif
    };
}
#pragma once

#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using SecondarySkeletonList = TInlineVector<Skeleton const*, 3>;

    //-------------------------------------------------------------------------
    // Pose Buffer
    //-------------------------------------------------------------------------
    // Storage for all the poses for a given animation update setup

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
    // Cached Pose Buffer
    //-------------------------------------------------------------------------
    // Cached storage for all the poses for a given animation update setup - used for forced transitions

    struct CachedPoseID
    {
        EE_SERIALIZE( m_ID );

        static constexpr uint8_t const s_maxAllowableValue = 64;
        static constexpr uint8_t const s_requiredBitsToSerialize = 6;

    public:

        CachedPoseID() = default;
        CachedPoseID( uint8_t ID ) : m_ID( Math::Min( ID, s_maxAllowableValue ) ) {}

        inline bool IsValid() const { return m_ID < s_maxAllowableValue; }
        inline void Clear() { m_ID = s_maxAllowableValue; }

        inline bool operator==( CachedPoseID const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( CachedPoseID const& rhs ) const { return m_ID != rhs.m_ID; }

    public:

        uint8_t m_ID = s_maxAllowableValue;
    };

    struct EE_ENGINE_API CachedPoseBuffer : public PoseBuffer
    {
        friend class PoseBufferPool;

    public:

        using PoseBuffer::PoseBuffer;

    private:

        void Release( Pose::Type poseType = Pose::Type::None );

    public:

        CachedPoseID                        m_ID;
        bool                                m_isLifetimeInternallyManaged = false; // Is the lifetime of this buffer controlled by the pose pool
        bool                                m_wasAccessed = false; // Did we either read or write to this buffer in a given update
        bool                                m_shouldBeReset = false;
    };

    //-------------------------------------------------------------------------
    // Pose Buffer Pool
    //-------------------------------------------------------------------------
    // TODO: Store all poses in a contiguous block of memory and profile if this is in fact faster or not

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

        bool IsValidCachedPose( CachedPoseID cachedPoseID ) const;

        CachedPoseID CreateCachedPoseBuffer();
        void DestroyCachedPoseBuffer( CachedPoseID cachedPoseID );
        void ResetCachedPoseBuffer( CachedPoseID cachedPoseID );

        // Try to get a pose-buffer with the specified ID, returns null if it cant find a buffer with the specified ID
        PoseBuffer* GetCachedPoseBuffer( CachedPoseID cachedPoseID ) { return GetCachedPoseBufferInternal( cachedPoseID ); }

        // Get a pose-buffer with the specified ID.
        // NOTE! This will create a buffer if one does not exist!!!
        PoseBuffer* GetOrCreateCachedPoseBuffer( CachedPoseID cachedPoseID, bool isLifetimeManagedByPosePool = false );

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

        CachedPoseBuffer* GetCachedPoseBufferInternal( CachedPoseID cachedPoseID );
        CachedPoseBuffer* CreateCachedPoseBufferInternal( CachedPoseID bufferID = CachedPoseID() );

    private:

        TInlineVector<PoseBuffer, 10>               m_poseBuffers;
        TInlineVector<CachedPoseBuffer, 10>         m_cachedBuffers;
        TInlineVector<UUID, 5>                      m_cachedPoseBuffersToDestroy;
        int8_t                                      m_firstFreeCachedBuffer = 0;
        int8_t                                      m_firstFreeBuffer = 0;
        uint8_t                                     m_nextCachedPoseID = 0;

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
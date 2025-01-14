#pragma once

#include "Animation_Task.h"
#include "Animation_TaskSerializer.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationDebug.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    enum class TaskSystemDebugMode
    {
        Off,
        FinalPose,
        PoseTree,
        DetailedPoseTree
    };
    #endif

    //-------------------------------------------------------------------------

    class TaskSerializer;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TaskSystem
    {
        friend class AnimationDebugView;

    public:

        static void InitializeTaskTypesList( TypeSystem::TypeRegistry const& typeRegistry );
        static void ShutdownTaskTypesList();
        static TVector<TypeSystem::TypeInfo const*> const* GetTaskTypesList();

    public:

        TaskSystem( Skeleton const* pSkeleton );
        ~TaskSystem();

        void Reset();

        // Get the actual final character transform for this frame
        Transform const& GetCharacterWorldTransform() const { return m_taskContext.m_worldTransform; }

        // Pose(s)
        //-------------------------------------------------------------------------

        // Get final pose buffer
        PoseBuffer const* GetPoseBuffer() const { return &m_finalPoseBuffer; }

        // Get the final primary pose generated by the task system
        Pose const* GetPrimaryPose() const{ return &m_finalPoseBuffer.m_poses[0]; }

        // Get the final primary pose generated by the task system
        TInlineVector<Pose const*, 1> GetSecondaryPoses() const;

        // Skeleton(s)
        //-------------------------------------------------------------------------

        // Get the primary skeleton from this task system
        Skeleton const* GetSkeleton() const { return m_posePool.GetPrimarySkeleton(); }

        // Set the skeleton LOD for the result pose
        EE_FORCE_INLINE void SetSkeletonLOD( Skeleton::LOD lod ) { m_taskContext.m_skeletonLOD = lod; }

        // Get the current skeleton LOD we are using
        EE_FORCE_INLINE Skeleton::LOD GetSkeletonLOD() const { return m_taskContext.m_skeletonLOD; }

        // Set the secondary skeletons that we can animate
        void SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons );

        // Get the number of secondary skeletons set
        EE_FORCE_INLINE int32_t GetNumSecondarySkeletons() const { return m_posePool.GetNumSecondarySkeletons(); }

        // Execution
        //-------------------------------------------------------------------------

        // Do we have any tasks that need execution
        inline bool RequiresUpdate() const { return m_needsUpdate; }

        // Do we have a physics dependency in our registered tasks
        inline bool HasPhysicsDependency() const { return m_hasPhysicsDependency; }

        // Run all pre-physics tasks
        void UpdatePrePhysics( float deltaTime, Transform const& worldTransform, Transform const& worldTransformInverse );

        // Run all post-physics tasks and fill out the final pose buffer
        void UpdatePostPhysics();

        // Cached Pose storage
        //-------------------------------------------------------------------------

        // Is there a valid cached pose buffer for this ID?
        bool IsValidCachedPose( CachedPoseID cachedPoseID ) const;

        // Create storage for a cached pose - returns the ID for the cached pose storage
        CachedPoseID CreateCachedPose();

        // Reset the pose in the allocated cached pose buffer
        void ResetCachedPose( CachedPoseID cachedPoseID );

        // Destroy an allocated cached pose buffer - frees the ID to be reused
        void DestroyCachedPose( CachedPoseID cachedPoseID );

        #if EE_DEVELOPMENT_TOOLS
        // Ensure that we have a valid cached pose buffer with the given ID. Needed for restoring from a recorded state
        void EnsureCachedPoseExists( CachedPoseID cachedPoseID );
        #endif

        // Task Registration
        //-------------------------------------------------------------------------

        inline bool HasTasks() const { return m_tasks.size() > 0; }
        inline TVector<Task*> const& GetRegisteredTasks() const { return m_tasks; }

        template< typename T, typename ... ConstructorParams >
        inline int8_t RegisterTask( int64_t sourceID, ConstructorParams&&... params )
        {
            EE_ASSERT( m_tasks.size() < 0xFF );

            #if EE_DEVELOPMENT_TOOLS
            m_debugPathTracker.AddTrackedPath( sourceID );
            #endif

            auto pNewTask = m_tasks.emplace_back( EE::New<T>( eastl::forward<ConstructorParams>( params )... ) );
            m_hasPhysicsDependency |= pNewTask->HasPhysicsDependency();
            m_needsUpdate = true;
            return (int8_t) ( m_tasks.size() - 1 );
        }

        int8_t GetCurrentTaskIndexMarker() const { return (int8_t) m_tasks.size(); }
        void RollbackToTaskIndexMarker( int8_t const marker );

        // Task Serialization
        //-------------------------------------------------------------------------

        // Serialized the current executed tasks - NOTE: this can fail since some tasks (i.e. physics) cannot be serialized!
        // Only do this if there are no currently pending tasks!
        bool SerializeTasks( ResourceMappings const& resourceMappings, Blob& outSerializedData ) const;

        // Create a new set of tasks from a serialized set of data
        // Only do this if there are no registered tasks!
        void DeserializeTasks( ResourceMappings const& resourceMappings, Blob const& inSerializedData );

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void SetDebugMode( TaskSystemDebugMode mode );
        TaskSystemDebugMode GetDebugMode() const { return m_debugMode; }
        void DrawDebug( Drawing::DrawContext& drawingContext );

        // Add a new path element to add extra information about the source of events (this should only be called from child/external graph nodes)
        void PushBaseDebugPath( int16_t nodeIdx ) { m_debugPathTracker.PushBasePath( nodeIdx ); }

        // Pop a path element from the current path
        void PopBaseDebugPath() { m_debugPathTracker.PopBasePath(); }

        // Do we currently have a debug path set?
        inline bool HasDebugBasePathSet() const { return m_debugPathTracker.HasBasePath(); }

        TInlineVector<int64_t, 5> const& GetTaskDebugPath( int8_t taskIdx ) const
        {
            EE_ASSERT( taskIdx >= 0 && taskIdx < m_tasks.size() );
            return m_debugPathTracker.m_itemPaths[taskIdx];
        }
        #endif

    private:

        bool AddTaskChainToPrePhysicsList( int8_t taskIdx );
        void ExecuteTasks();

    private:

        TVector<Task*>                          m_tasks;
        PoseBufferPool                          m_posePool;
        BoneMaskPool                            m_boneMaskPool;
        TaskContext                             m_taskContext;
        TInlineVector<int8_t, 16>               m_prePhysicsTaskIndices;
        PoseBuffer                              m_finalPoseBuffer;
        bool                                    m_hasPhysicsDependency = false;
        bool                                    m_hasCodependentPhysicsTasks = false;
        bool                                    m_needsUpdate = false;
        uint32_t                                m_maxBitsForTaskTypeID = 0;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        TaskSystemDebugMode                     m_debugMode = TaskSystemDebugMode::Off;
        DebugPathTracker                        m_debugPathTracker;
        #endif
    };
}
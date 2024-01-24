#pragma once

#include "Animation_TaskPosePool.h"
#include "Base/Types/Color.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Task;
    class BoneMaskPool;
    class TaskSerializer;

    //-------------------------------------------------------------------------

    using TaskSourceID = int16_t;
    using TaskIndex = int8_t;
    using TaskDependencies = TInlineVector<TaskIndex, 2>;

    //-------------------------------------------------------------------------

    enum class TaskUpdateStage : uint8_t
    {
        Any = 0,
        PrePhysics,
        PostPhysics,
    };

    //-------------------------------------------------------------------------

    struct TaskContext
    {
        TaskContext( PoseBufferPool& posePool, BoneMaskPool& boneMaskPool )
            : m_posePool( posePool )
            , m_boneMaskPool( boneMaskPool )
        {}

        Transform                       m_worldTransform = Transform::Identity;
        Transform                       m_worldTransformInverse = Transform::Identity;
        TInlineVector<Task*, 2>         m_dependencies = { nullptr, nullptr };
        PoseBufferPool&                 m_posePool;
        BoneMaskPool&                   m_boneMaskPool;
        float                           m_deltaTime = 0;
        TaskUpdateStage                 m_updateStage = TaskUpdateStage::Any;
        int8_t                          m_currentTaskIdx = InvalidIndex;
        Skeleton::LOD                   m_skeletonLOD = Skeleton::LOD::High;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Task : public IReflectedType
    {
        EE_REFLECT_TYPE( Task );

    public:

        #if EE_DEVELOPMENT_TOOLS
        static void DrawSecondaryPoses( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled );
        #endif

    public:

        Task( TaskSourceID sourceID, TaskUpdateStage updateStage = TaskUpdateStage::Any, TaskDependencies const& dependencies = TaskDependencies() );
        virtual ~Task() {}
        virtual void Execute( TaskContext const& context ) = 0;
        virtual uint32_t GetTaskTypeID() const { return 0; }

        inline int8_t GetResultBufferIndex() const { return m_bufferIdx; }
        inline bool IsComplete() const { return m_isComplete; }
        inline TaskDependencies const& GetDependencyIndices() const { return m_dependencies; }
        inline int32_t GetNumDependencies() const { return (int32_t) m_dependencies.size(); }

        // Get the stage that this task is required to run in
        inline TaskUpdateStage GetRequiredUpdateStage() const { return m_updateStage; }

        // Get the stage that this task actually run in (this only differs from the required stage for 'Any' stage tasks)
        inline TaskUpdateStage GetActualUpdateStage() const { return m_actualUpdateStage; }

        // Do we have a dependency on the physics simulation?
        inline bool	HasPhysicsDependency() const { return m_updateStage != TaskUpdateStage::Any; }

        // Serialization
        //-------------------------------------------------------------------------

        // Can this task be serialized?
        virtual bool AllowsSerialization() const { return false; }

        // Serialize the task to a compressed format
        // The base function should never be called!
        virtual void Serialize( TaskSerializer& serializer ) const { EE_UNREACHABLE_CODE(); }

        // Deserialize the task from a compressed format
        // The base function should never be called!
        virtual void Deserialize( TaskSerializer& serializer ) { EE_UNREACHABLE_CODE(); }

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const { return String(); }
        virtual Color GetDebugColor() const { return Colors::White; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const;
        #endif

    protected:

        inline PoseBuffer* GetNewPoseBuffer( TaskContext const& context )
        {
            EE_ASSERT( m_bufferIdx == InvalidIndex );
            m_bufferIdx = context.m_posePool.RequestPoseBuffer();
            return context.m_posePool.GetBuffer( m_bufferIdx );
        }

        inline void ReleasePoseBuffer( TaskContext const& context )
        {
            EE_ASSERT( m_bufferIdx != InvalidIndex );
            context.m_posePool.ReleasePoseBuffer( m_bufferIdx );
            m_bufferIdx = InvalidIndex;
        }

        // Transfer a dependency result to this task
        inline PoseBuffer* TransferDependencyPoseBuffer( TaskContext const& context, TaskIndex dependencyIdx )
        {
            EE_ASSERT( m_bufferIdx == InvalidIndex );

            auto pDependencyTask = context.m_dependencies[dependencyIdx];
            EE_ASSERT( pDependencyTask != nullptr && pDependencyTask->m_isComplete && pDependencyTask->m_bufferIdx != InvalidIndex );

            m_bufferIdx = pDependencyTask->m_bufferIdx;
            pDependencyTask->m_bufferIdx = InvalidIndex;
            return context.m_posePool.GetBuffer( m_bufferIdx );
        }

        // Get a dependency task result
        inline PoseBuffer* AccessDependencyPoseBuffer( TaskContext const& context, TaskIndex dependencyIdx )
        {
            auto pDependencyTask = context.m_dependencies[dependencyIdx];
            EE_ASSERT( pDependencyTask != nullptr && pDependencyTask->m_isComplete && pDependencyTask->m_bufferIdx != InvalidIndex );
            return context.m_posePool.GetBuffer( pDependencyTask->m_bufferIdx );
        }

        // Acquire and release a dependency's result buffer
        inline void ReleaseDependencyPoseBuffer( TaskContext const& context, TaskIndex dependencyIdx )
        {
            auto pDependencyTask = context.m_dependencies[dependencyIdx];
            EE_ASSERT( pDependencyTask != nullptr && pDependencyTask->m_isComplete && pDependencyTask->m_bufferIdx != InvalidIndex );
            pDependencyTask->ReleasePoseBuffer( context );
        }

        // Get a temporary working buffer that we intend to immediately release
        inline int8_t GetTemporaryPoseBuffer( TaskContext const& context, PoseBuffer*& outBuffer )
        {
            int8_t const tempBufferIdx = context.m_posePool.RequestPoseBuffer();
            outBuffer = context.m_posePool.GetBuffer( tempBufferIdx );
            return tempBufferIdx;
        }

        // Release the temporary buffer we requested
        inline void ReleaseTemporaryPoseBuffer( TaskContext const& context, int8_t bufferIdx )
        {
            EE_ASSERT( bufferIdx != InvalidIndex && bufferIdx != m_bufferIdx );
            context.m_posePool.ReleasePoseBuffer( bufferIdx );
        }

        //-------------------------------------------------------------------------

        inline void MarkTaskComplete( TaskContext const& context )
        {
            EE_ASSERT( !m_isComplete );
            m_isComplete = true;
            m_actualUpdateStage = context.m_updateStage;

            #if EE_DEVELOPMENT_TOOLS
            EE_ASSERT( context.m_currentTaskIdx != InvalidIndex && m_bufferIdx != InvalidIndex );
            context.m_posePool.RecordPose( context.m_currentTaskIdx, m_bufferIdx );
            #endif
        }

    protected:

        TaskSourceID                    m_sourceID = InvalidIndex;
        TaskUpdateStage                 m_updateStage = TaskUpdateStage::Any;
        int8_t                          m_bufferIdx = InvalidIndex;
        TaskUpdateStage                 m_actualUpdateStage = TaskUpdateStage::Any;
        bool                            m_isComplete = false;
        TaskDependencies                m_dependencies;
    };
}
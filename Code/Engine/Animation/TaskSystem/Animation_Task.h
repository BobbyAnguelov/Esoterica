#pragma once

#include "Animation_TaskPosePool.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Task;

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
        TaskContext( PoseBufferPool& posePool )
            : m_posePool( posePool )
        {}

        Transform                       m_worldTransform = Transform::Identity;
        Transform                       m_worldTransformInverse = Transform::Identity;
        TInlineVector<Task*, 2>         m_dependencies = { nullptr, nullptr };
        float                           m_deltaTime = 0;
        TaskUpdateStage                 m_updateStage = TaskUpdateStage::Any;
        PoseBufferPool&                 m_posePool;
    };

    //-------------------------------------------------------------------------

    class Task
    {

    public:

        Task( TaskSourceID sourceID, TaskUpdateStage updateStage = TaskUpdateStage::Any, TaskDependencies const& dependencies = TaskDependencies() );
        virtual ~Task() {}
        virtual void Execute( TaskContext const& context ) = 0;
        virtual uint32_t GetTypeID() const { return 0; }

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

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const { return String(); }
        virtual Color GetDebugColor() const { return Colors::White; }
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
            EE_ASSERT( m_bufferIdx != InvalidIndex );
            context.m_posePool.RecordPose( m_bufferIdx );
            #endif
        }

    protected:

        TaskSourceID                    m_sourceID = InvalidIndex;
        TaskUpdateStage                 m_updateStage = TaskUpdateStage::Any;

    private:

        int8_t                            m_bufferIdx = InvalidIndex;
        TaskDependencies                m_dependencies;
        TaskUpdateStage                 m_actualUpdateStage = TaskUpdateStage::Any;
        bool                            m_isComplete = false;
    };
}

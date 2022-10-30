#include "Animation_TaskSystem.h"
#include "Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/AnimationBlender.h"
#include "System/Log.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TaskSystem::TaskSystem( Skeleton const* pSkeleton )
        : m_posePool( pSkeleton )
        , m_taskContext( m_posePool )
        , m_finalPose( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );
        m_finalPose.CalculateGlobalTransforms();
    }

    TaskSystem::~TaskSystem()
    {
        Reset();
    }

    void TaskSystem::Reset()
    {
        for ( auto pTask : m_tasks )
        {
            EE::Delete( pTask );
        }

        m_tasks.clear();
        m_posePool.Reset();
        m_hasPhysicsDependency = false;
    }

    //-------------------------------------------------------------------------

    void TaskSystem::RollbackToTaskIndexMarker( TaskIndex const marker )
    {
        EE_ASSERT( marker >= 0 && marker <= m_tasks.size() );

        for ( int16_t t = (int16_t) m_tasks.size() - 1; t >= marker; t-- )
        {
            EE::Delete( m_tasks[t] );
            m_tasks.erase( m_tasks.begin() + t );
        }
    }

    //-------------------------------------------------------------------------

    bool TaskSystem::AddTaskChainToPrePhysicsList( TaskIndex taskIdx )
    {
        EE_ASSERT( taskIdx >= 0 && taskIdx < m_tasks.size() );
        EE_ASSERT( m_hasCodependentPhysicsTasks == false );

        auto pTask = m_tasks[taskIdx];
        for ( auto DepTaskIdx : pTask->GetDependencyIndices() )
        {
            if ( !AddTaskChainToPrePhysicsList( DepTaskIdx ) )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        // Cant have a dependency that relies on physics
        if ( pTask->GetRequiredUpdateStage() == TaskUpdateStage::PostPhysics )
        {
            return false;
        }

        // Cant add the same task twice, i.e. codependency
        if ( VectorContains( m_prePhysicsTaskIndices, taskIdx ) )
        {
            return false;
        }

        m_prePhysicsTaskIndices.emplace_back( taskIdx );
        return true;
    }

    void TaskSystem::UpdatePrePhysics( float deltaTime, Transform const& worldTransform, Transform const& worldTransformInverse )
    {
        EE_PROFILE_SCOPE_ANIMATION( "Anim Pre-Physics Tasks" );

        m_taskContext.m_currentTaskIdx = InvalidIndex;
        m_taskContext.m_deltaTime = deltaTime;
        m_taskContext.m_worldTransform = worldTransform;
        m_taskContext.m_worldTransformInverse = worldTransformInverse;
        m_taskContext.m_updateStage = TaskUpdateStage::PrePhysics;

        m_prePhysicsTaskIndices.clear();
        m_hasCodependentPhysicsTasks = false;

        // Conditionally execute all pre-physics tasks
        //-------------------------------------------------------------------------

        if ( m_hasPhysicsDependency )
        {
            // Go backwards through the registered task and execute all task chains with a pre-physics requirement
            auto const numTasks = (int8_t) m_tasks.size();
            for ( TaskIndex i = 0; i < numTasks; i++ )
            {
                if ( m_tasks[i]->GetRequiredUpdateStage() == TaskUpdateStage::PrePhysics )
                {
                    if ( !AddTaskChainToPrePhysicsList( i ) )
                    {
                        m_hasCodependentPhysicsTasks = true;
                        break;
                    }
                }
            }

            // If we've detected a co-dependent physics task, ignore all registered tasks by just pushing a new task and immediately executing it
            if ( m_hasCodependentPhysicsTasks )
            {
                EE_LOG_WARNING( "Animation", "TODO", "Co-dependent physics tasks detected!" );
                RegisterTask<Tasks::DefaultPoseTask>( (int16_t) InvalidIndex, Pose::Type::ReferencePose );
                m_tasks.back()->Execute( m_taskContext );
            }
            else // Execute pre-physics tasks
            {
                for ( TaskIndex prePhysicsTaskIdx : m_prePhysicsTaskIndices )
                {
                    m_taskContext.m_currentTaskIdx = prePhysicsTaskIdx;

                    // Set dependencies
                    m_taskContext.m_dependencies.clear();
                    for ( auto depTaskIdx : m_tasks[prePhysicsTaskIdx]->GetDependencyIndices() )
                    {
                        EE_ASSERT( m_tasks[depTaskIdx]->IsComplete() );
                        m_taskContext.m_dependencies.emplace_back( m_tasks[depTaskIdx] );
                    }

                    m_tasks[prePhysicsTaskIdx]->Execute( m_taskContext );
                }
            }
        }
        else // If we have no physics dependent tasks, execute all tasks now
        {
            ExecuteTasks();
        }
    }

    void TaskSystem::UpdatePostPhysics()
    {
        EE_PROFILE_SCOPE_ANIMATION( "Anim Post-Physics Tasks" );

        m_taskContext.m_updateStage = TaskUpdateStage::PostPhysics;

        // If we detected co-dependent tasks in the pre-physics update, there's nothing to do here
        if ( m_hasCodependentPhysicsTasks )
        {
            return;
        }

        // Execute tasks
        //-------------------------------------------------------------------------
        // Only run tasks if we have a physics dependency, else all tasks were already executed in the first update stage

        if ( m_hasPhysicsDependency )
        {
            ExecuteTasks();
        }

        // Reflect animation pose out
        //-------------------------------------------------------------------------

        if ( !m_tasks.empty() )
        {
            auto pFinalTask = m_tasks.back();
            EE_ASSERT( pFinalTask->IsComplete() );
            PoseBuffer const* pResultPoseBuffer = m_posePool.GetBuffer( pFinalTask->GetResultBufferIndex() );
            Pose const* pResultPose = &pResultPoseBuffer->m_pose;

            // Always return a non-additive pose
            if ( pResultPose->IsAdditivePose() )
            {
                m_finalPose.Reset( Pose::Type::ReferencePose );
                Blender::BlendAdditive( &m_finalPose, pResultPose, 1.0f, nullptr, &m_finalPose );
            }
            else // Just copy the pose
            {
                m_finalPose.CopyFrom( pResultPoseBuffer->m_pose );
            }

            // Calculate the global transforms and release the task pose buffer
            m_finalPose.CalculateGlobalTransforms();
            m_posePool.ReleasePoseBuffer( pFinalTask->GetResultBufferIndex() );
        }
        else
        {
            m_finalPose.Reset( Pose::Type::ReferencePose, true );
        }
    }

    void TaskSystem::ExecuteTasks()
    {
        int16_t const numTasks = (int8_t) m_tasks.size();
        for ( TaskIndex i = 0; i < numTasks; i++ )
        {
            if ( !m_tasks[i]->IsComplete() )
            {
                m_taskContext.m_currentTaskIdx = i;

                // Set dependencies
                m_taskContext.m_dependencies.clear();
                for ( auto DepTaskIdx : m_tasks[i]->GetDependencyIndices() )
                {
                    EE_ASSERT( m_tasks[DepTaskIdx]->IsComplete() );
                    m_taskContext.m_dependencies.emplace_back( m_tasks[DepTaskIdx] );
                }

                // Execute task
                m_tasks[i]->Execute( m_taskContext );
            }
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void TaskSystem::SetDebugMode( TaskSystemDebugMode mode )
    {
        m_debugMode = mode;
        m_posePool.EnableRecording( m_debugMode != TaskSystemDebugMode::Off );
    }

    void TaskSystem::CalculateTaskOffset( TaskIndex taskIdx, Float2 const& currentOffset, TInlineVector<Float2, 16>& offsets )
    {
        EE_ASSERT( taskIdx >= 0 && taskIdx < m_tasks.size() );
        auto pTask = m_tasks[taskIdx];
        offsets[taskIdx] = currentOffset;
        
        int32_t const numDependencies = pTask->GetNumDependencies();
        if ( numDependencies == 0 )
        {
            // Do nothing
        }
        else if ( numDependencies == 1 )
        {
            Float2 childOffset = currentOffset;
            childOffset += Float2( 0, -1 );
            CalculateTaskOffset( pTask->GetDependencyIndices()[0], childOffset, offsets );
        }
        else // multiple dependencies
        {
            Float2 childOffset = currentOffset;
            childOffset += Float2( 0, -1 );

            float const childTaskStartOffset = -( numDependencies - 1.0f ) / 2.0f;
            for ( int32_t i = 0; i < numDependencies; i++ )
            {
                childOffset.m_x = currentOffset.m_x + childTaskStartOffset + i;
                CalculateTaskOffset( pTask->GetDependencyIndices()[i], childOffset, offsets );
            }
        }
    }

    void TaskSystem::DrawDebug( Drawing::DrawContext& drawingContext )
    {
        if ( m_debugMode == TaskSystemDebugMode::Off )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_posePool.IsRecordingEnabled() );
        if ( !HasTasks() || !m_posePool.HasRecordedData() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_debugMode == TaskSystemDebugMode::FinalPose )
        {
            auto const& pFinalTask = m_tasks.back();
            EE_ASSERT( pFinalTask->IsComplete() );
            auto pPose = m_posePool.GetRecordedPoseForTask( (int8_t) m_tasks.size() - 1 );
            pPose->DrawDebug( drawingContext, m_taskContext.m_worldTransform );
            return;
        }

        // Calculate task tree offsets
        //-------------------------------------------------------------------------

        TInlineVector<Float2, 16> taskTreeOffsets;
        taskTreeOffsets.resize( m_tasks.size(), Float2::Zero );
        CalculateTaskOffset( (TaskIndex) m_tasks.size() - 1, Float2::Zero, taskTreeOffsets );

        Vector const offsetVectorX = m_taskContext.m_worldTransform.GetAxisX().GetNegated() * 2.0f;
        Vector const offsetVectorY = m_taskContext.m_worldTransform.GetAxisY().GetNegated() * 1.5f;

        TInlineVector<Transform, 16> taskTransforms;
        taskTransforms.resize( m_tasks.size(), m_taskContext.m_worldTransform );

        for ( int8_t i = (int8_t) m_tasks.size() - 1; i >= 0; i-- )
        {
            Vector const offset = ( offsetVectorX * taskTreeOffsets[i].m_x ) + ( offsetVectorY * taskTreeOffsets[i].m_y );
            taskTransforms[i].SetTranslation( m_taskContext.m_worldTransform.GetTranslation() + offset );
        }

        // Draw tree
        //-------------------------------------------------------------------------

        for ( int8_t i = (int8_t) m_tasks.size() - 1; i >= 0; i-- )
        {
            auto pPose = m_posePool.GetRecordedPoseForTask( i );

            // No point displaying a pile of bones, so display an additive on top of the reference pose
            if ( pPose->IsAdditivePose() )
            {
                Pose tempPose( pPose->GetSkeleton(), Pose::Type::ReferencePose );
                Blender::BlendAdditive( &tempPose, pPose, 1.0f, nullptr, &tempPose );
                tempPose.CalculateGlobalTransforms();
                tempPose.DrawDebug( drawingContext, taskTransforms[i], m_tasks[i]->GetDebugColor() );
            }
            else // Just draw the recorded pose
            {
                pPose->DrawDebug( drawingContext, taskTransforms[i], m_tasks[i]->GetDebugColor() );
            }

            drawingContext.DrawText3D( taskTransforms[i].GetTranslation(), m_tasks[i]->GetDebugText().c_str(), m_tasks[i]->GetDebugColor(), Drawing::FontSmall, Drawing::AlignMiddleCenter );

            for ( auto& dependencyIdx : m_tasks[i]->GetDependencyIndices() )
            {
                drawingContext.DrawLine( taskTransforms[i].GetTranslation(), taskTransforms[dependencyIdx].GetTranslation(), m_tasks[i]->GetDebugColor(), 2.0f );
            }
        }
    }
    #endif
}
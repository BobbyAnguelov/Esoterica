#include "Animation_TaskSystem.h"
#include "Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/AnimationBlender.h"

#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    namespace
    {
        struct TaskTreeOffsetData
        {
            void CalculateRelativeOffsets()
            {
                int32_t const numChildren = (int32_t) m_children.size();
                if ( numChildren == 0 )
                {
                    m_width = 1;
                }
                else if ( numChildren == 1 )
                {
                    m_children[0]->CalculateRelativeOffsets();
                    m_children[0]->m_relativeOffset = Float2( 0.0f, -1.0f );
                    m_width = m_children[0]->m_width;
                }
                else // multiple children
                {
                    m_width = 0.0f;

                    for ( int32_t i = 0; i < numChildren; i++ )
                    {
                        m_children[i]->CalculateRelativeOffsets();
                        m_width += m_children[i]->m_width;
                    }

                    float startOffset = -( m_width / 2 );
                    for ( int32_t i = 0; i < numChildren; i++ )
                    {
                        float halfWidth = m_children[i]->m_width / 2;
                        m_children[i]->m_relativeOffset = Float2( startOffset + halfWidth, -1.0f );
                        startOffset += m_children[i]->m_width;
                    }
                }
            }

            void CalculateAbsoluteOffsets( Float2 const& parentOffset )
            {
                m_offset = parentOffset + m_relativeOffset;

                int32_t const numChildren = (int32_t) m_children.size();
                for ( int32_t i = 0; i < numChildren; i++ )
                {
                    m_children[i]->CalculateAbsoluteOffsets( m_offset );
                }
            }

        public:

            Task*                           m_pTask = nullptr;
            Float2                          m_relativeOffset;
            Float2                          m_offset;
            float                           m_width = 0;
            TVector<TaskTreeOffsetData*>        m_children;
        };

        void CalculateTaskTreeOffsets( TVector<Task*> const& tasks, Transform const& worldTransform, TInlineVector<Transform, 16>& taskTransforms )
        {
            int32_t const numTasks = (int32_t) tasks.size();
            TInlineVector<TaskTreeOffsetData, 16> taskOffsets;
            taskOffsets.resize( numTasks );

            for ( int32_t i = 0; i < numTasks; i++ )
            {
                auto pTask = tasks[i];
                taskOffsets[i].m_pTask = pTask;

                int32_t const numDependencies = pTask->GetNumDependencies();
                for ( int32_t j = 0; j < numDependencies; j++ )
                {
                    taskOffsets[i].m_children.emplace_back( &taskOffsets[pTask->GetDependencyIndices()[j]] );
                }
            }

            //-------------------------------------------------------------------------

            taskOffsets.back().CalculateRelativeOffsets();
            taskOffsets.back().CalculateAbsoluteOffsets( Float2::Zero );

            //-------------------------------------------------------------------------

            Vector const offsetVectorX = worldTransform.GetAxisX() * 2.0f;
            Vector const offsetVectorY = worldTransform.GetAxisY().GetNegated() * 1.5f;

            taskTransforms.resize( numTasks, worldTransform );

            for ( int32_t i = numTasks - 1; i >= 0; i-- )
            {
                Vector const offset = ( offsetVectorX * taskOffsets[i].m_offset.m_x ) + ( offsetVectorY * taskOffsets[i].m_offset.m_y );
                taskTransforms[i].AddTranslation( offset );
            }
        }
    }
}
#endif

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TaskSystem::TaskSystem( Skeleton const* pSkeleton )
        : m_posePool( pSkeleton )
        , m_boneMaskPool( pSkeleton )
        , m_taskContext( m_posePool, m_boneMaskPool )
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

        #if EE_DEVELOPMENT_TOOLS
        m_boneMaskPool.PerformValidation();
        #endif

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
                Blender::AdditiveBlend( &m_finalPose, pResultPose, 1.0f, nullptr, &m_finalPose );
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

        m_needsUpdate = false;
    }

    //-------------------------------------------------------------------------

    void TaskSystem::EnableSerialization( TypeSystem::TypeRegistry const& typeRegistry )
    {
        m_taskTypeRemapTable = typeRegistry.GetAllDerivedTypes( Task::GetStaticTypeID(), false, false, true );
        m_maxBitsForTaskTypeID = Math::GetMostSignificantBit( (uint32_t) m_taskTypeRemapTable.size() ) + 1;
        EE_ASSERT( m_maxBitsForTaskTypeID <= 8 );

        m_serializationEnabled = true;
    }

    void TaskSystem::DisableSerialization()
    {
        m_taskTypeRemapTable.clear();
        m_maxBitsForTaskTypeID = 0;
        m_serializationEnabled = false;
    }

    bool TaskSystem::SerializeTasks( TInlineVector<ResourceLUT const*, 10> const& LUTs, Blob& outSerializedData ) const
    {
        auto FindTaskTypeID = [this] ( TypeSystem::TypeID typeID )
        {
            uint8_t const numTaskTypes = (uint8_t) m_taskTypeRemapTable.size();
            for ( uint8_t i = 0; i < numTaskTypes; i++ )
            {
                if ( m_taskTypeRemapTable[i]->m_ID == typeID )
                {
                    return i;
                }
            }

            return uint8_t( 0xFF );
        };

        //-------------------------------------------------------------------------

        EE_ASSERT( m_serializationEnabled );
        EE_ASSERT( !m_needsUpdate );

        uint8_t const numTasks = (uint8_t) m_tasks.size();
        TaskSerializer serializer( GetSkeleton(), LUTs, numTasks );

        // Serialize task types
        for ( auto pTask : m_tasks )
        {
            EE_ASSERT( pTask->IsComplete() );
            uint8_t serializedTypeID = FindTaskTypeID( pTask->GetTypeID() );
            EE_ASSERT( serializedTypeID != 0xFF );
            serializer.WriteUInt( serializedTypeID, m_maxBitsForTaskTypeID );
        }

        // Serialize task data
        for ( auto pTask : m_tasks )
        {
            if ( !pTask->AllowsSerialization() )
            {
                return false;
            }

            pTask->Serialize( serializer );
        }

        serializer.GetWrittenData( outSerializedData );

        return true;
    }

    void TaskSystem::DeserializeTasks( TInlineVector<ResourceLUT const*, 10> const& LUTs, Blob const& inSerializedData )
    {
        EE_ASSERT( m_serializationEnabled );
        EE_ASSERT( m_tasks.empty() );
        EE_ASSERT( !m_needsUpdate );

        TaskSerializer serializer( GetSkeleton(), LUTs, inSerializedData );
        uint8_t const numTasks = serializer.GetNumSerializedTasks();

        // Create tasks
        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            uint8_t const taskTypeID = (uint8_t) serializer.ReadUInt( m_maxBitsForTaskTypeID );
            Task* pTask = Cast<Task>( m_taskTypeRemapTable[taskTypeID]->CreateType() );
            EE_ASSERT( pTask->AllowsSerialization() );
            m_tasks.emplace_back( pTask );
        }

        // Deserialize Tasks
        for ( auto pTask : m_tasks )
        {
            pTask->Deserialize( serializer );
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void TaskSystem::SetDebugMode( TaskSystemDebugMode mode )
    {
        m_debugMode = mode;
        m_posePool.EnableRecording( m_debugMode != TaskSystemDebugMode::Off );
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

        TInlineVector<Transform, 16> taskTransforms;
        CalculateTaskTreeOffsets( m_tasks, m_taskContext.m_worldTransform, taskTransforms );

        // Draw tree
        //-------------------------------------------------------------------------

        for ( int8_t i = (int8_t) m_tasks.size() - 1; i >= 0; i-- )
        {
            auto pPose = m_posePool.GetRecordedPoseForTask( i );

            // No point displaying a pile of bones, so display an additive on top of the reference pose
            if ( pPose->IsAdditivePose() )
            {
                Pose tempPose( pPose->GetSkeleton(), Pose::Type::ReferencePose );
                Blender::AdditiveBlend( &tempPose, pPose, 1.0f, nullptr, &tempPose );
                tempPose.CalculateGlobalTransforms();
                m_tasks[i]->DrawDebug( drawingContext, taskTransforms[i], &tempPose, m_debugMode == TaskSystemDebugMode::DetailedPoseTree );
            }
            else // Just use the recorded pose
            {
                m_tasks[i]->DrawDebug( drawingContext, taskTransforms[i], pPose, m_debugMode == TaskSystemDebugMode::DetailedPoseTree );
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
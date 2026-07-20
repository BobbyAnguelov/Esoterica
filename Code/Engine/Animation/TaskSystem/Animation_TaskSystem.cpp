#include "Animation_TaskSystem.h"
#include "Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/AnimationBlender.h"

#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Utils/TreeLayout.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    static TVector<EE::TypeSystem::TypeInfo const*> const * g_pTaskTypeTable;

    void TaskSystem::InitializeTaskTypesList( TypeSystem::TypeRegistry const& typeRegistry )
    {
        EE_ASSERT( g_pTaskTypeTable == nullptr );
        g_pTaskTypeTable = EE::New<TVector<EE::TypeSystem::TypeInfo const*>>( typeRegistry.GetAllDerivedTypes( PoseTask::GetStaticTypeID(), false, false, true ) );
    }

    void TaskSystem::ShutdownTaskTypesList()
    {
        EE_ASSERT( g_pTaskTypeTable != nullptr );
        EE::Delete( g_pTaskTypeTable );
    }

    TVector<EE::TypeSystem::TypeInfo const*> const* TaskSystem::GetTaskTypesList()
    {
        EE_ASSERT( g_pTaskTypeTable != nullptr );
        return g_pTaskTypeTable;
    }

    //-------------------------------------------------------------------------

    TaskSystem::TaskSystem( Skeleton const* pSkeleton )
        : m_posePool( pSkeleton )
        , m_boneMaskPool( pSkeleton, SecondarySkeletonList() )
        , m_taskContext( m_posePool, m_boneMaskPool )
    {
        EE_ASSERT( pSkeleton != nullptr );

        m_pFinalPoseBuffer = EE::New<PoseBuffer>( pSkeleton, SecondarySkeletonList() );
        m_pFinalPoseBuffer->CalculateModelSpaceTransforms();

        m_pPreviousFinalPoseBuffer = EE::New<PoseBuffer>( pSkeleton, SecondarySkeletonList() );
        m_pPreviousFinalPoseBuffer->CalculateModelSpaceTransforms();

        auto pTaskTypesTable = GetTaskTypesList();
        m_maxBitsForTaskTypeID = Math::GetMaxNumberOfBitsForValue( (uint32_t) pTaskTypesTable->size() );
        EE_ASSERT( m_maxBitsForTaskTypeID <= 8 );

        #if EE_DEVELOPMENT_TOOLS
        m_taskContext.m_pLog = &m_log;
        #endif
    }

    TaskSystem::~TaskSystem()
    {
        Reset();

        EE::Delete( m_pFinalPoseBuffer );
        EE::Delete( m_pPreviousFinalPoseBuffer );
    }

    void TaskSystem::Reset()
    {
        for ( auto pTask : m_tasks )
        {
            EE::Delete( pTask );
        }

        m_tasks.clear();
        m_posePool.Reset();
        m_needsUpdate = false;
        m_hasPhysicsDependency = false;
        m_executionTime = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        m_log.clear();
        #endif
    }

    void TaskSystem::ResetForNewUpdate()
    {
        Reset();
        eastl::swap( m_pFinalPoseBuffer, m_pPreviousFinalPoseBuffer );
        m_executionTime = 0.0f;
    }

    //-------------------------------------------------------------------------

    void TaskSystem::SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons )
    {
        EE_ASSERT( Skeleton::ValidateSkeletonSetup( GetSkeleton(), secondarySkeletons ) );

        m_posePool.SetSecondarySkeletons( secondarySkeletons );
        m_boneMaskPool.SetSecondarySkeletons( secondarySkeletons );
        m_pFinalPoseBuffer->UpdateSecondarySkeletonList( secondarySkeletons );
        m_pPreviousFinalPoseBuffer->UpdateSecondarySkeletonList( secondarySkeletons );
    }

    TInlineVector<Pose const*, 1> TaskSystem::GetSecondaryPoses() const
    {
        TInlineVector<Pose const*, 1> secondaryPoses;

        int8_t const numPoses = (int8_t) m_pFinalPoseBuffer->m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            secondaryPoses.emplace_back( &m_pFinalPoseBuffer->m_poses[poseIdx] );
        }

        return secondaryPoses;
    }


    TInlineVector<Pose const*, 1> TaskSystem::GetSecondaryPosesForPreviousUpdate() const
    {
        TInlineVector<Pose const*, 1> secondaryPoses;

        int8_t const numPoses = (int8_t) m_pPreviousFinalPoseBuffer->m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            secondaryPoses.emplace_back( &m_pPreviousFinalPoseBuffer->m_poses[poseIdx] );
        }

        return secondaryPoses;
    }

    //-------------------------------------------------------------------------

    void TaskSystem::RollbackToTaskIndexMarker( int8_t const marker )
    {
        EE_ASSERT( marker >= 0 && marker <= m_tasks.size() );

        for ( int16_t t = (int16_t) m_tasks.size() - 1; t >= marker; t-- )
        {
            EE::Delete( m_tasks[t] );
            m_tasks.erase( m_tasks.begin() + t );
        }
    }

    //-------------------------------------------------------------------------

    bool TaskSystem::AddTaskChainToPrePhysicsList( int8_t taskIdx )
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
        ScopedTimer<PlatformClock, Microseconds> st( m_executionTime );
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
            for ( int8_t i = 0; i < numTasks; i++ )
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
                EE_LOG_WARNING( LogCategory::Animation, "TODO", "Co-dependent physics tasks detected!" );
                RegisterTask<ReferencePoseTask>( SourcePath() );
                m_tasks.back()->Execute( m_taskContext );
            }
            else // Execute pre-physics tasks
            {
                for ( int8_t prePhysicsTaskIdx : m_prePhysicsTaskIndices )
                {
                    m_taskContext.m_currentTaskIdx = prePhysicsTaskIdx;

                    // Set dependencies
                    m_taskContext.m_dependencies.clear();
                    for ( auto depTaskIdx : m_tasks[prePhysicsTaskIdx]->GetDependencyIndices() )
                    {
                        EE_ASSERT( m_tasks[depTaskIdx]->IsComplete() );
                        m_taskContext.m_dependencies.emplace_back( m_tasks[depTaskIdx] );
                    }

                    {
                        #if EE_DEVELOPMENT_TOOLS
                        ScopedTimer<PlatformClock, Microseconds> tt( m_tasks[prePhysicsTaskIdx]->m_executionTime );
                        #endif

                        //EE_DEVELOPMENT_TOOLS_ONLY( ScopedTimer<PlatformClock, Microseconds> tt( m_tasks[prePhysicsTaskIdx]->m_executionTime ) );
                        m_tasks[prePhysicsTaskIdx]->Execute( m_taskContext );
                    }
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

        #if EE_DEVELOPMENT_TOOLS
        Timer<PlatformClock> timer;
        timer.Start();
        #endif

        // Execute tasks
        //-------------------------------------------------------------------------
        // Only run tasks if we have a physics dependency, else all tasks were already executed in the first update stage

        if ( m_hasPhysicsDependency )
        {
            ExecuteTasks();
        }

        // Reflect animation pose
        //-------------------------------------------------------------------------

        if ( !m_tasks.empty() )
        {
            #if EE_DEVELOPMENT_TOOLS
            CalculateTaskTreeOffsets( m_taskContext.m_worldTransform );
            #endif

            auto pFinalTask = m_tasks.back();
            EE_ASSERT( pFinalTask->IsComplete() );
            PoseBuffer const* pResultPoseBuffer = m_posePool.GetBuffer( pFinalTask->GetResultBufferIndex() );

            m_pFinalPoseBuffer->CopyFrom( pResultPoseBuffer );

            int8_t const numPoses = (int8_t) m_pFinalPoseBuffer->m_poses.size();
            for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
            {
                // Never return additive poses as a final result
                if ( m_pFinalPoseBuffer->m_poses[poseIdx].IsAdditivePose() )
                {
                    Blender::ApplyAdditiveToReferencePose( m_taskContext.m_skeletonLOD, &m_pFinalPoseBuffer->m_poses[poseIdx], 1.0f, nullptr, &m_pFinalPoseBuffer->m_poses[poseIdx] );
                }
            }

            // Calculate the global transforms and release the task pose buffer
            m_pFinalPoseBuffer->CalculateModelSpaceTransforms();
            m_posePool.ReleasePoseBuffer( pFinalTask->GetResultBufferIndex() );
        }
        else
        {
            m_pFinalPoseBuffer->Release( Pose::Init::ReferencePose, true );
        }

        #if EE_DEVELOPMENT_TOOLS
        m_executionTime += timer.GetElapsedTimeMicroseconds();
        #endif
    }

    void TaskSystem::ExecuteTasks()
    {
        // Execute all incomplete tasks
        //-------------------------------------------------------------------------

        int16_t const numTasks = (int8_t) m_tasks.size();
        for ( int8_t i = 0; i < numTasks; i++ )
        {
            if ( !m_tasks[i]->IsComplete() )
            {
                m_taskContext.m_currentTaskIdx = i;

                // Set dependencies
                m_taskContext.m_dependencies.clear();
                for ( auto depTaskIdx : m_tasks[i]->GetDependencyIndices() )
                {
                    EE_ASSERT( m_tasks[depTaskIdx]->IsComplete() );
                    m_taskContext.m_dependencies.emplace_back( m_tasks[depTaskIdx] );
                }

                // Execute task
                {
                    #if EE_DEVELOPMENT_TOOLS
                    ScopedTimer<PlatformClock, Microseconds> tt( m_tasks[i]->m_executionTime );
                    #endif

                    m_tasks[i]->Execute( m_taskContext );
                }
            }
        }

        //-------------------------------------------------------------------------

        m_needsUpdate = false;
    }

    //-------------------------------------------------------------------------

    bool TaskSystem::IsValidCachedPose( CachedPoseID cachedPoseID ) const
    {
        return m_posePool.IsValidCachedPose( cachedPoseID );
    }

    PoseBuffer* TaskSystem::GetCachedPose( CachedPoseID cachedPoseID )
    {
        return m_posePool.GetCachedPoseBuffer( cachedPoseID );
    }

    CachedPoseID TaskSystem::CreateCachedPose()
    {
        return m_posePool.CreateCachedPoseBuffer();
    }

    #if EE_DEVELOPMENT_TOOLS
    void TaskSystem::EnsureCachedPoseExists( CachedPoseID cachedPoseID )
    {
        m_posePool.GetOrCreateCachedPoseBuffer( cachedPoseID );
    }
    #endif

    void TaskSystem::DestroyCachedPose( CachedPoseID cachedPoseID )
    {
        m_posePool.DestroyCachedPoseBuffer( cachedPoseID );
    }

    //-------------------------------------------------------------------------

    void TaskSystem::SerializeTasks( TaskSerializationContext const& taskSerializationContext, Blob& outSerializedTopologyData, Blob& outSerializedTaskData ) const
    {
        EE_ASSERT( !m_needsUpdate );

        uint8_t const numTasks = (uint8_t) m_tasks.size();
        if ( numTasks == 0 )
        {
            outSerializedTopologyData.clear();
            outSerializedTaskData.clear();
            return;
        }

        //-------------------------------------------------------------------------

        TaskSerializer serializer( taskSerializationContext );
        serializer.WriteNumTasks( numTasks );

        //-------------------------------------------------------------------------

        // Serialize task data
        for ( auto pTask : m_tasks )
        {
            EE_ASSERT( pTask->IsComplete() );

            // Topology
            //-------------------------------------------------------------------------

            serializer.WriteTaskType( pTask );

            for ( int32_t i = 0; i < pTask->m_dependencies.size(); i++ )
            {
                serializer.WriteDependencyIndex( pTask->m_dependencies[i] );
            }

            // Data
            //-------------------------------------------------------------------------

            pTask->Serialize( serializer );
        }

        serializer.m_topologyData.GetWrittenData( outSerializedTopologyData );
        serializer.GetWrittenData( outSerializedTaskData );
    }

    void TaskSystem::DeserializeTasks( TaskSerializationContext const& taskSerializationContext, Blob const& inSerializedTopologyData, Blob const& inSerializedTaskData )
    {
        EE_ASSERT( m_tasks.empty() );
        EE_ASSERT( !m_needsUpdate );
        EE_ASSERT( !inSerializedTopologyData.empty() );
        EE_ASSERT( !inSerializedTaskData.empty() );

        TaskSerializer serializer( taskSerializationContext, inSerializedTopologyData, inSerializedTaskData );
        uint8_t const numTasks = serializer.GetNumSerializedTasks();

        // Create tasks
        bool hasErrorOccurred = false;
        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            // Create task
            PoseTask* pTask = serializer.ReadTaskTypeAndCreateTask();
            if ( pTask == nullptr )
            {
                hasErrorOccurred = true;
                break;
            }

            m_tasks.emplace_back( pTask );

            // Get dependency data
            //-------------------------------------------------------------------------

            int32_t const numDependencies = pTask->GetNumDependencies();
            pTask->m_dependencies.reserve( numDependencies );
            for ( int32_t d = 0; d < numDependencies; d++ )
            {
                pTask->m_dependencies[d] = serializer.ReadDependencyIndex();
            }
        }

        // If something went wrong, wipe the whole recipe!
        if ( hasErrorOccurred )
        {
            for ( PoseTask* pTask : m_tasks )
            {
                EE::Delete( pTask );
            }

            m_tasks.clear();
            return;
        }

        // Task Data
        //-------------------------------------------------------------------------

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

    void TaskSystem::CalculateTaskTreeOffsets( Transform const &worldTransform ) const
    {
        EE_ASSERT( !RequiresUpdate() );
        EE_ASSERT( !m_tasks.empty() );

        // Create Tree Layout
        //-------------------------------------------------------------------------

        int32_t const numTasks = (int32_t) m_tasks.size();
        TreeLayout layout;
        layout.m_nodes.resize( numTasks );

        for ( int32_t i = 0; i < numTasks; i++ )
        {
            auto pTask = m_tasks[i];
            layout.m_nodes[i].m_pItem = pTask;
            layout.m_nodes[i].m_width = 2.0f;
            layout.m_nodes[i].m_height = 1.5f;

            int32_t const numDependencies = pTask->GetNumDependencies();
            if ( numDependencies > 0 )
            {
                for ( int32_t j = 0; j < numDependencies; j++ )
                {
                    layout.m_nodes[i].m_children.emplace_back( &layout.m_nodes[pTask->GetDependencyIndices()[j]] );
                }
            }
        }

        layout.PerformLayout( Float2( 0.1f, 0.1f ) );

        // Set up final world transforms
        //-------------------------------------------------------------------------

        Vector const offsetVectorX = m_taskContext.m_worldTransform.GetAxisX();
        Vector const offsetVectorY = m_taskContext.m_worldTransform.GetAxisY();

        TInlineVector<Transform, 16> taskTransforms;
        taskTransforms.resize( numTasks, m_taskContext.m_worldTransform );

        for ( int32_t i = numTasks - 1; i >= 0; i-- )
        {
            Vector const offset = ( offsetVectorX * layout.m_nodes[i].m_position.m_x ) + ( offsetVectorY * layout.m_nodes[i].m_position.m_y );
            taskTransforms[i].AddTranslation( offset );
        }
    }

    void TaskSystem::DrawDebug( DebugDrawContext& drawingContext )
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

        int8_t const numPoses = (int8_t) m_pFinalPoseBuffer->m_poses.size();

        // Only draw the final pose
        if ( m_debugMode == TaskSystemDebugMode::FinalPose )
        {
            auto const& pFinalTask = m_tasks.back();
            EE_ASSERT( pFinalTask->IsComplete() );

            auto pPoseBuffer = m_posePool.GetRecordedPoseBufferForTask( (int8_t) m_tasks.size() - 1 );

            // Primary Pose
            pPoseBuffer->m_poses[0].DrawDebug( drawingContext, m_taskContext.m_worldTransform );
            PoseTask::DrawSecondaryPoses( drawingContext, m_taskContext.m_worldTransform, m_taskContext.m_skeletonLOD, pPoseBuffer, m_debugMode == TaskSystemDebugMode::DetailedPoseTree );
        }
        else // Draw Tree
        {
            // Draw tree
            //-------------------------------------------------------------------------

            InlineString strBuffer;

            for ( int8_t i = (int8_t) m_tasks.size() - 1; i >= 0; i-- )
            {
                PoseBuffer* pPoseBuffer = m_posePool.GetRecordedPoseBufferForTask( i );
                EE_ASSERT( pPoseBuffer != nullptr );

                // No point displaying a pile of bones, so display additives on top of their skeleton's reference pose
                for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
                {
                    Pose& pose = pPoseBuffer->m_poses[poseIdx];
                    if ( pose.IsAdditivePose() )
                    {
                        Blender::ApplyAdditiveToReferencePose( m_taskContext.m_skeletonLOD, &pose, 1.0f, nullptr, &pose );
                        pPoseBuffer->m_poses[poseIdx].CalculateModelSpaceTransforms();
                    }
                }

                //-------------------------------------------------------------------------

                bool const isDetailedModeEnabled = ( m_debugMode == TaskSystemDebugMode::DetailedPoseTree );

                strBuffer.sprintf( "%s\n%s\n%.2f", m_tasks[i]->GetDebugName(), m_tasks[i]->GetDebugTextInfo( isDetailedModeEnabled ).c_str(), m_tasks[i]->GetDebugProgressOrWeight() );

                m_tasks[i]->DrawDebug( drawingContext, m_tasks[i]->m_debugWorldTransform, m_taskContext.m_skeletonLOD, pPoseBuffer, isDetailedModeEnabled );
                drawingContext.DrawText3D( m_tasks[i]->m_debugWorldTransform.GetTranslation(), strBuffer.c_str(), m_tasks[i]->GetDebugColor(), DebugFont::Small, DebugTextAlign::MiddleCenter );

                for ( auto& dependencyIdx : m_tasks[i]->GetDependencyIndices() )
                {
                    drawingContext.DrawLine( m_tasks[i]->m_debugWorldTransform.GetTranslation(), m_tasks[dependencyIdx]->m_debugWorldTransform.GetTranslation(), m_tasks[i]->GetDebugColor(), 2.0f );
                }
            }

            // Draw LOD
            //-------------------------------------------------------------------------

            if ( m_taskContext.m_skeletonLOD == Skeleton::LOD::Low )
            {
                drawingContext.DrawTextBox3D( m_taskContext.m_worldTransform.GetTranslation() + Vector( 0, 0, 0.5f ), "Low LOD", Colors::Red, DebugFont::Small, DebugTextAlign::MiddleCenter );
            }
        }
    }
    #endif
}
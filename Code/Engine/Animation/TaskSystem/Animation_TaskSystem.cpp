#include "Animation_TaskSystem.h"
#include "Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/AnimationBlender.h"

#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Utils/TreeLayout.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    static TVector<EE::TypeSystem::TypeInfo const*> const * g_pTaskTypeTable;

    void TaskSystem::InitializeTaskTypesList( TypeSystem::TypeRegistry const& typeRegistry )
    {
        EE_ASSERT( g_pTaskTypeTable == nullptr );
        g_pTaskTypeTable = EE::New<TVector<EE::TypeSystem::TypeInfo const*>>( typeRegistry.GetAllDerivedTypes( Task::GetStaticTypeID(), false, false, true ) );
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
        , m_boneMaskPool( pSkeleton )
        , m_taskContext( m_posePool, m_boneMaskPool )
        , m_finalPoseBuffer( pSkeleton, SecondarySkeletonList() )
    {
        EE_ASSERT( pSkeleton != nullptr );
        m_finalPoseBuffer.CalculateModelSpaceTransforms();

        auto pTaskTypesTable = GetTaskTypesList();
        m_maxBitsForTaskTypeID = Math::GetMostSignificantBit( (uint32_t) pTaskTypesTable->size() ) + 1;
        EE_ASSERT( m_maxBitsForTaskTypeID <= 8 );
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

        #if EE_DEVELOPMENT_TOOLS
        m_debugPathTracker.Clear();
        #endif
    }

    //-------------------------------------------------------------------------

    void TaskSystem::SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons )
    {
        m_posePool.SetSecondarySkeletons( secondarySkeletons );
        m_finalPoseBuffer.UpdateSecondarySkeletonList( secondarySkeletons );
    }

    TInlineVector<Pose const*, 1> TaskSystem::GetSecondaryPoses() const
    {
        TInlineVector<Pose const*, 1> secondaryPoses;

        int8_t const numPoses = (int8_t) m_finalPoseBuffer.m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            secondaryPoses.emplace_back( &m_finalPoseBuffer.m_poses[poseIdx] );
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

            #if EE_DEVELOPMENT_TOOLS
            m_debugPathTracker.RemoveTrackedPath( t );
            #endif
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
                EE_LOG_WARNING( "Animation", "TODO", "Co-dependent physics tasks detected!" );
                RegisterTask<Tasks::ReferencePoseTask>( InvalidIndex );
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

            // Always return a non-additive pose
            if ( pResultPoseBuffer->IsAdditive() )
            {
                int8_t const numPoses = (int8_t) m_finalPoseBuffer.m_poses.size();
                for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
                {
                    Blender::ApplyAdditiveToReferencePose( m_taskContext.m_skeletonLOD, &pResultPoseBuffer->m_poses[poseIdx], 1.0f, nullptr, &m_finalPoseBuffer.m_poses[poseIdx] );
                }
            }
            else // Just copy the pose
            {
                m_finalPoseBuffer.CopyFrom( pResultPoseBuffer );
            }

            // Calculate the global transforms and release the task pose buffer
            m_finalPoseBuffer.CalculateModelSpaceTransforms();
            m_posePool.ReleasePoseBuffer( pFinalTask->GetResultBufferIndex() );
        }
        else
        {
            m_finalPoseBuffer.Release( Pose::Type::ReferencePose, true );
        }
    }

    void TaskSystem::ExecuteTasks()
    {
        int16_t const numTasks = (int8_t) m_tasks.size();
        for ( int8_t i = 0; i < numTasks; i++ )
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

    bool TaskSystem::IsValidCachedPose( CachedPoseID cachedPoseID ) const
    {
        return m_posePool.IsValidCachedPose( cachedPoseID );
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

    void TaskSystem::ResetCachedPose( CachedPoseID cachedPoseID )
    {
        m_posePool.ResetCachedPoseBuffer( cachedPoseID );
    }

    void TaskSystem::DestroyCachedPose( CachedPoseID cachedPoseID )
    {
        m_posePool.DestroyCachedPoseBuffer( cachedPoseID );
    }

    //-------------------------------------------------------------------------

    bool TaskSystem::SerializeTasks( ResourceMappings const& resourceMappings, Blob& outSerializedData ) const
    {
        auto FindTaskTypeID = [] ( TypeSystem::TypeID typeID )
        {
            auto const& taskTypeTable = *GetTaskTypesList();
            uint8_t const numTaskTypes = (uint8_t) taskTypeTable.size();
            for ( uint8_t i = 0; i < numTaskTypes; i++ )
            {
                if ( taskTypeTable[i]->m_ID == typeID )
                {
                    return i;
                }
            }

            return uint8_t( 0xFF );
        };

        //-------------------------------------------------------------------------

        EE_ASSERT( !m_needsUpdate );

        uint8_t const numTasks = (uint8_t) m_tasks.size();
        TaskSerializer serializer( GetSkeleton(), resourceMappings, numTasks );

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

    void TaskSystem::DeserializeTasks( ResourceMappings const& resourceMappings, Blob const& inSerializedData )
    {
        EE_ASSERT( m_tasks.empty() );
        EE_ASSERT( !m_needsUpdate );

        TaskSerializer serializer( GetSkeleton(), resourceMappings, inSerializedData );
        uint8_t const numTasks = serializer.GetNumSerializedTasks();

        // Create tasks
        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            auto const& taskTypeTable = *GetTaskTypesList();
            uint8_t const taskTypeID = (uint8_t) serializer.ReadUInt( m_maxBitsForTaskTypeID );
            Task* pTask = Cast<Task>( taskTypeTable[taskTypeID]->CreateType() );
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

        int8_t const numPoses = (int8_t) m_finalPoseBuffer.m_poses.size();

        // Only draw the final pose
        if ( m_debugMode == TaskSystemDebugMode::FinalPose )
        {
            auto const& pFinalTask = m_tasks.back();
            EE_ASSERT( pFinalTask->IsComplete() );

            auto pPoseBuffer = m_posePool.GetRecordedPoseBufferForTask( (int8_t) m_tasks.size() - 1 );

            // Primary Pose
            pPoseBuffer->m_poses[0].DrawDebug( drawingContext, m_taskContext.m_worldTransform );
            Task::DrawSecondaryPoses( drawingContext, m_taskContext.m_worldTransform, m_taskContext.m_skeletonLOD, pPoseBuffer, m_debugMode == TaskSystemDebugMode::DetailedPoseTree );
        }
        else // Draw Tree
        {
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

            // Draw tree
            //-------------------------------------------------------------------------

            InlineString strBuffer;

            for ( int8_t i = (int8_t) m_tasks.size() - 1; i >= 0; i-- )
            {
                auto pPoseBuffer = m_posePool.GetRecordedPoseBufferForTask( i );
                EE_ASSERT( pPoseBuffer != nullptr );

                // No point displaying a pile of bones, so display additives on top of their skeleton's reference pose
                if ( pPoseBuffer->IsAdditive() )
                {
                    for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
                    {
                        Pose& pose = pPoseBuffer->m_poses[poseIdx];
                        if ( pose.IsAdditivePose() )
                        {
                            Blender::ApplyAdditiveToReferencePose( m_taskContext.m_skeletonLOD, &pose, 1.0f, nullptr, &pose );
                            pPoseBuffer->m_poses[poseIdx].CalculateModelSpaceTransforms();
                        }
                    }
                }

                //-------------------------------------------------------------------------

                strBuffer.sprintf( "%s\n%s\n%.2f", m_tasks[i]->GetDebugName(), m_tasks[i]->GetDebugTextInfo().c_str(), m_tasks[i]->GetDebugProgressOrWeight() );

                m_tasks[i]->DrawDebug( drawingContext, taskTransforms[i], m_taskContext.m_skeletonLOD, pPoseBuffer, m_debugMode == TaskSystemDebugMode::DetailedPoseTree );
                drawingContext.DrawText3D( taskTransforms[i].GetTranslation(), strBuffer.c_str(), m_tasks[i]->GetDebugColor(), Drawing::FontSmall, Drawing::AlignMiddleCenter );

                for ( auto& dependencyIdx : m_tasks[i]->GetDependencyIndices() )
                {
                    drawingContext.DrawLine( taskTransforms[i].GetTranslation(), taskTransforms[dependencyIdx].GetTranslation(), m_tasks[i]->GetDebugColor(), 2.0f );
                }
            }

            // Draw LOD
            //-------------------------------------------------------------------------

            if ( m_taskContext.m_skeletonLOD == Skeleton::LOD::Low )
            {
                drawingContext.DrawTextBox3D( m_taskContext.m_worldTransform.GetTranslation() + Vector( 0, 0, 0.5f ), "Low LOD", Colors::Red, Drawing::FontSmall, Drawing::AlignMiddleCenter );
            }
        }
    }
    #endif
}
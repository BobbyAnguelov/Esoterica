#include "Animation_Task_Blend.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    void BlendTaskBase::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteDependencyIndex( m_dependencies[1] );
        serializer.WriteNormalizedFloat8Bit( m_blendWeight );

        // Bone Mask Task List
        bool const hasBoneMaskTasks = m_boneMaskTaskList.HasTasks();
        serializer.WriteBool( hasBoneMaskTasks );
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Serialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    void BlendTaskBase::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 2 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_dependencies[1] = serializer.ReadDependencyIndex();
        m_blendWeight = serializer.ReadNormalizedFloat8Bit();

        // Bone Mask Task List
        bool const hasBoneMaskTasks = serializer.ReadBool();
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Deserialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void BlendTaskBase::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        TBitFlags<Pose::DrawFlags> drawFlags;
        if ( isDetailedViewEnabled )
        {
            if ( m_debugBoneMask.IsValid() )
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneWeights );
            }
            else
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneNames );
            }
        }

        pRecordedPoseBuffer->m_poses[0].DrawDebug( drawingContext, worldTransform, GetDebugColor(), 3.0f, m_debugBoneMask.IsValid() ? &m_debugBoneMask : nullptr, drawFlags );
        DrawSecondaryPoses( drawingContext, worldTransform, pRecordedPoseBuffer );
    }
    #endif

    //-------------------------------------------------------------------------

    BlendTask::BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : BlendTaskBase( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
    {
        m_blendWeight = blendWeight;
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }

        if ( pBoneMaskTaskList != nullptr )
        {
            m_boneMaskTaskList.CopyFrom( *pBoneMaskTaskList );
        }
    }

    void BlendTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        // Get working buffers
        //-------------------------------------------------------------------------

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        // Primary Pose
        //-------------------------------------------------------------------------

        // If we have a bone mask task list, execute it
        // Note: Bone masks only apply to the primary skeleton
        if ( m_boneMaskTaskList.HasTasks() )
        {
            auto const boneMaskResult = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );
            Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[0], &pTargetBuffer->m_poses[0], m_blendWeight, boneMaskResult.m_pBoneMask, &pFinalBuffer->m_poses[0] );

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            if ( context.m_posePool.IsRecordingEnabled() )
            {
                m_debugBoneMask = *boneMaskResult.m_pBoneMask;
            }
            #endif

            if ( boneMaskResult.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( boneMaskResult.m_maskPoolIdx );
            }
        }
        else // Perform a simple blend
        {
            Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[0], &pTargetBuffer->m_poses[0], m_blendWeight, nullptr, &pFinalBuffer->m_poses[0] );
        }

        // Secondary Poses
        //-------------------------------------------------------------------------

        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            bool const hasTargetPose = pTargetBuffer->m_poses[poseIdx].IsPoseSet();

            if ( !hasSourcePose && !hasTargetPose )
            {
                // Do Nothing
            }
            else if ( !hasSourcePose && hasTargetPose )
            {
                Blender::LocalBlendFromReferencePose( context.m_skeletonLOD, &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
            else if ( hasSourcePose && !hasTargetPose )
            {
                Blender::LocalBlendToReferencePose( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
            else // has both poses
            {
                Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
        }

        //-------------------------------------------------------------------------

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    String BlendTask::GetDebugText() const
    {
        if ( m_boneMaskTaskList.HasTasks() )
        {
            return String( String::CtorSprintf(), "Blend (Masked): %.2f%%", m_blendWeight * 100 );
        }
        else
        {
            return String( String::CtorSprintf(), "Blend: %.2f%%", m_blendWeight * 100 );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    OverlayBlendTask::OverlayBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : BlendTaskBase( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
    {
        m_blendWeight = blendWeight;
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }

        if ( pBoneMaskTaskList != nullptr )
        {
            m_boneMaskTaskList.CopyFrom( *pBoneMaskTaskList );
        }
    }

    void OverlayBlendTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        // Get working buffers
        //-------------------------------------------------------------------------

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        // Primary Pose
        //-------------------------------------------------------------------------

        // If we have a bone mask task list, execute it
        // Note: Bone masks only apply to the primary skeleton
        if ( m_boneMaskTaskList.HasTasks() )
        {
            auto const boneMaskResult = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );
            Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[0], &pTargetBuffer->m_poses[0], m_blendWeight, boneMaskResult.m_pBoneMask, &pFinalBuffer->m_poses[0] );

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            if ( context.m_posePool.IsRecordingEnabled() )
            {
                m_debugBoneMask = *boneMaskResult.m_pBoneMask;
            }
            #endif

            if ( boneMaskResult.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( boneMaskResult.m_maskPoolIdx );
            }
        }
        else // Perform a simple blend
        {
            Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[0], &pTargetBuffer->m_poses[0], m_blendWeight, nullptr, &pFinalBuffer->m_poses[0] );
        }

        // Secondary Pose
        //-------------------------------------------------------------------------

        // Since this is an overlay blend - if a secondary pose is not set in the overlay, then we ignore the blend
        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            bool const hasTargetPose = pTargetBuffer->m_poses[poseIdx].IsPoseSet();
            if ( !hasTargetPose )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            if ( hasSourcePose )
            {
                Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
            else // Apply overlay to the reference pose
            {
                Blender::LocalBlendFromReferencePose( context.m_skeletonLOD, &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
        }

        //-------------------------------------------------------------------------

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    String OverlayBlendTask::GetDebugText() const
    {
        if ( m_boneMaskTaskList.HasTasks() )
        {
            return String( String::CtorSprintf(), "Overlay Blend (Masked): %.2f%%", m_blendWeight * 100 );
        }
        else
        {
            return String( String::CtorSprintf(), "Overlay Blend: %.2f%%", m_blendWeight * 100 );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    AdditiveBlendTask::AdditiveBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : BlendTaskBase( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
    {
        m_blendWeight = blendWeight;
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }

        if ( pBoneMaskTaskList != nullptr )
        {
            m_boneMaskTaskList.CopyFrom( *pBoneMaskTaskList );
        }
    }

    void AdditiveBlendTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        // Get working buffers
        //-------------------------------------------------------------------------

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        // Primary Pose
        //-------------------------------------------------------------------------

        if ( !pTargetBuffer->m_poses[0].IsZeroPose() )
        {
            // If we have a bone mask task list, execute it 
            // Note: Bone masks only apply to the primary skeleton
            if ( m_boneMaskTaskList.HasTasks() )
            {
                auto const boneMaskResult = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );

                //-------------------------------------------------------------------------

                Blender::AdditiveBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[0], &pTargetBuffer->m_poses[0], m_blendWeight, boneMaskResult.m_pBoneMask, &pFinalBuffer->m_poses[0] );

                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                if ( context.m_posePool.IsRecordingEnabled() )
                {
                    m_debugBoneMask = *boneMaskResult.m_pBoneMask;
                }
                #endif

                if ( boneMaskResult.m_maskPoolIdx != InvalidIndex )
                {
                    context.m_boneMaskPool.ReleaseMask( boneMaskResult.m_maskPoolIdx );
                }
            }
            else // Perform a simple blend
            {
                Blender::AdditiveBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[0], &pTargetBuffer->m_poses[0], m_blendWeight, nullptr, &pFinalBuffer->m_poses[0] );
            }
        }

        // Secondary Pose
        //-------------------------------------------------------------------------

        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            // Skip any unset or zero additives
            if ( pTargetBuffer->m_poses[poseIdx].IsZeroPose() || !pTargetBuffer->m_poses[poseIdx].IsPoseSet() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            if ( hasSourcePose )
            {
                Blender::AdditiveBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
            else // Apply the additive to the reference pose
            {
                Blender::ApplyAdditiveToReferencePose( context.m_skeletonLOD, &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
            }
        }

        //-------------------------------------------------------------------------

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    String AdditiveBlendTask::GetDebugText() const
    {
        if ( m_boneMaskTaskList.HasTasks() )
        {
            return String( String::CtorSprintf(), "Additive Blend (Masked): %.2f%%", m_blendWeight * 100 );
        }
        else
        {
            return String( String::CtorSprintf(), "Additive Blend: %.2f%%", m_blendWeight * 100 );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    GlobalBlendTask::GlobalBlendTask( TaskSourceID sourceID, TaskIndex baseTaskIdx, TaskIndex layerTaskIdx, float const blendWeight, BoneMaskTaskList const& boneMaskTaskList )
        : BlendTaskBase( sourceID, TaskUpdateStage::Any, { baseTaskIdx, layerTaskIdx } )
    {
        m_blendWeight = blendWeight;
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );
        EE_ASSERT( boneMaskTaskList.HasTasks() );
        m_boneMaskTaskList.CopyFrom( boneMaskTaskList );
    }

    void GlobalBlendTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        // Get working buffers
        //-------------------------------------------------------------------------

        auto pSourceBuffer = AccessDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = TransferDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pTargetBuffer;

        // Primary Pose
        //-------------------------------------------------------------------------

        bool shouldRunBlend = m_boneMaskTaskList.HasTasks();

        // If we have a bone mask task list, execute it
        if ( shouldRunBlend )
        {
            auto const boneMaskResult = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );
            Blender::GlobalBlend( context.m_skeletonLOD, pSourceBuffer->GetPrimaryPose(), pTargetBuffer->GetPrimaryPose(), m_blendWeight, boneMaskResult.m_pBoneMask, pFinalBuffer->GetPrimaryPose() );

            #if EE_DEVELOPMENT_TOOLS
            if ( context.m_posePool.IsRecordingEnabled() )
            {
                m_debugBoneMask = *boneMaskResult.m_pBoneMask;
            }
            #endif

            if ( boneMaskResult.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( boneMaskResult.m_maskPoolIdx );
            }
        }

        // Secondary Pose
        //-------------------------------------------------------------------------

        if ( shouldRunBlend )
        {
            // Since a global blend is an overlay blend - if a secondary pose is not set in the overlay, then we ignore the blend
            int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
            for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
            {
                bool const hasTargetPose = pTargetBuffer->m_poses[poseIdx].IsPoseSet();
                if ( !hasTargetPose )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
                if ( hasSourcePose )
                {
                    Blender::LocalBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
                }
                else // Apply overlay to the reference pose
                {
                    Blender::LocalBlendFromReferencePose( context.m_skeletonLOD, &pTargetBuffer->m_poses[poseIdx], m_blendWeight, nullptr, &pFinalBuffer->m_poses[poseIdx] );
                }
            }
        }

        //-------------------------------------------------------------------------

        ReleaseDependencyPoseBuffer( context, 0 );
        MarkTaskComplete( context );
    }
}
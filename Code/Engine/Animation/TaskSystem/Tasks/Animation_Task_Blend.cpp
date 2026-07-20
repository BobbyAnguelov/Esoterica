#include "Animation_Task_Blend.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Engine/Animation/TaskSystem/Animation_BoneMaskPool.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void BlendTaskBase::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteNormalizedFloat8Bit( m_blendWeight );

        // Bone Mask Task List
        bool const hasBoneMaskTasks = m_boneMaskTaskList.HasTasks();
        serializer.WriteBool( hasBoneMaskTasks );
        if ( hasBoneMaskTasks )
        {
            serializer.WriteBoneMaskTaskList( m_boneMaskTaskList );
        }
    }

    void BlendTaskBase::Deserialize( TaskSerializer& serializer )
    {
        m_blendWeight = serializer.ReadNormalizedFloat8Bit();

        // Bone Mask Task List
        bool const hasBoneMaskTasks = serializer.ReadBool();
        if ( hasBoneMaskTasks )
        {
            serializer.ReadBoneMaskTaskList( m_boneMaskTaskList );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void BlendTaskBase::DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        DrawOptions options;
        options.m_boneColor = GetDebugColor();
        options.m_drawBoneNames = isDetailedViewEnabled;
        options.m_pBoneWeights = m_debugBoneMask.IsValid() ? &m_debugBoneMask.GetWeights() : nullptr;

        pRecordedPoseBuffer->m_poses[0].DrawDebug( drawingContext, worldTransform, lod, options );
        DrawSecondaryPoses( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );
    }
    #endif

    //-------------------------------------------------------------------------

    BlendTask::BlendTask( int8_t sourceTaskIdx, int8_t targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : BlendTaskBase( TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
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

        // If neither pose is set, this is a no-op
        if ( !pSourceBuffer->IsPoseSet() && !pTargetBuffer->IsPoseSet() )
        {
            ReleaseDependencyPoseBuffer( context, 1 );
            MarkTaskComplete( context );
            return;
        }

        // If the pose types are different then this is an invalid operation
        if ( pSourceBuffer->IsAdditivePose() != pTargetBuffer->IsAdditivePose() )
        {
            EE_DEVELOPMENT_TOOLS_ONLY( context.LogError( m_sourcePath, "BlendTask - Trying to perform an blend with mis-matched pose types (additive/parent-space)" ) );

            pFinalBuffer->ResetPose();
            ReleaseDependencyPoseBuffer( context, 1 );
            MarkTaskComplete( context );
            return;
        }

        // Bone Mask
        //-------------------------------------------------------------------------

        int8_t boneMaskBufferIdx = InvalidIndex;
        BoneMaskBuffer const* pBoneMaskBuffer = nullptr;
        if ( m_boneMaskTaskList.HasTasks() )
        {
            boneMaskBufferIdx = m_boneMaskTaskList.GenerateBoneMask( pSourceBuffer->GetPrimarySkeleton(), context.m_boneMaskPool );
            EE_ASSERT( boneMaskBufferIdx != InvalidIndex );
            pBoneMaskBuffer = context.m_boneMaskPool.GetBuffer( boneMaskBufferIdx );
        }

        // Blend
        //-------------------------------------------------------------------------

        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
        {
            BoneMask const* pBoneMask = ( pBoneMaskBuffer != nullptr ) ? pBoneMaskBuffer->TryGetBoneMask( pSourceBuffer->m_poses[poseIdx].GetSkeleton() ) : nullptr;

            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            bool const hasTargetPose = pTargetBuffer->m_poses[poseIdx].IsPoseSet();

            if ( !hasSourcePose && !hasTargetPose )
            {
                // Do Nothing
            }
            else
            {
                if ( !hasSourcePose )
                {
                    pSourceBuffer->m_poses[poseIdx].Reset( pSourceBuffer->IsAdditivePose() ? Pose::Init::ZeroPose : Pose::Init::ReferencePose );
                }

                if ( !hasTargetPose )
                {
                    pTargetBuffer->m_poses[poseIdx].Reset( pTargetBuffer->IsAdditivePose() ? Pose::Init::ZeroPose : Pose::Init::ReferencePose );
                }

                Blender::ParentSpaceBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, pBoneMask, &pFinalBuffer->m_poses[poseIdx] );
            }

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            if ( poseIdx == 0 && context.m_posePool.IsRecordingEnabled() && pBoneMask != nullptr )
            {
                m_debugBoneMask = *pBoneMask;
            }
            #endif
        }

        //-------------------------------------------------------------------------

        bool const isAdditive = pFinalBuffer->GetPrimaryPose()->IsAdditivePose();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            EE_ASSERT( !pFinalBuffer->m_poses[poseIdx].IsPoseSet() || pFinalBuffer->m_poses[poseIdx].IsAdditivePose() == isAdditive );
        }

        //-------------------------------------------------------------------------

        if ( boneMaskBufferIdx != InvalidIndex )
        {
            context.m_boneMaskPool.ReleaseMaskBuffer( boneMaskBufferIdx );
        }

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString BlendTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        InlineString str;
        if ( m_boneMaskTaskList.HasTasks() )
        {
            str = "Masked";
        }
        return str;
    }
    #endif

    //-------------------------------------------------------------------------

    OverlayBlendTask::OverlayBlendTask( int8_t sourceTaskIdx, int8_t targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : BlendTaskBase( TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
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

        // If neither pose is set, this is a no-op
        if ( !pSourceBuffer->IsPoseSet() && !pTargetBuffer->IsPoseSet() )
        {
            ReleaseDependencyPoseBuffer( context, 1 );
            MarkTaskComplete( context );
            return;
        }

        // If the pose types are different then this is an invalid operation
        if ( pTargetBuffer->IsPoseSet() && ( pSourceBuffer->IsAdditivePose() != pTargetBuffer->IsAdditivePose() ) )
        {
            EE_DEVELOPMENT_TOOLS_ONLY( context.LogError( m_sourcePath, "OverlayBlendTask - Trying to perform an overlay blend with mis-matched pose types (additive/parent-space)" ) );

            pFinalBuffer->ResetPose();
            ReleaseDependencyPoseBuffer( context, 1 );
            MarkTaskComplete( context );
            return;
        }

        // Bone Mask
        //-------------------------------------------------------------------------

        int8_t boneMaskBufferIdx = InvalidIndex;
        BoneMaskBuffer const* pBoneMaskBuffer = nullptr;
        if ( m_boneMaskTaskList.HasTasks() )
        {
            boneMaskBufferIdx = m_boneMaskTaskList.GenerateBoneMask( pSourceBuffer->GetPrimarySkeleton(), context.m_boneMaskPool );
            EE_ASSERT( boneMaskBufferIdx != InvalidIndex );
            pBoneMaskBuffer = context.m_boneMaskPool.GetBuffer( boneMaskBufferIdx );
        }

        // Overlay Blend
        //-------------------------------------------------------------------------

        // Since this is an overlay blend - if a secondary pose is not set in the overlay, then we ignore the blend
        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
        {
            bool const hasTargetPose = pTargetBuffer->m_poses[poseIdx].IsPoseSet();
            if ( !hasTargetPose )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            BoneMask const* pBoneMask = ( pBoneMaskBuffer != nullptr ) ? pBoneMaskBuffer->TryGetBoneMask( pSourceBuffer->m_poses[poseIdx].GetSkeleton() ) : nullptr;

            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            if ( !hasSourcePose )
            {
                if ( pSourceBuffer->GetPrimaryPose()->IsAdditivePose() )
                {
                    pSourceBuffer->m_poses[poseIdx].Reset( Pose::Init::ZeroPose );
                }
                else
                {
                    pSourceBuffer->m_poses[poseIdx].Reset( Pose::Init::ReferencePose );
                }
            }

            Blender::ParentSpaceOverlayBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, pBoneMask, &pFinalBuffer->m_poses[poseIdx] );

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            if ( poseIdx == 0 && context.m_posePool.IsRecordingEnabled() && pBoneMask != nullptr )
            {
                m_debugBoneMask = *pBoneMask;
            }
            #endif
        }

        //-------------------------------------------------------------------------

        if ( boneMaskBufferIdx != InvalidIndex )
        {
            context.m_boneMaskPool.ReleaseMaskBuffer( boneMaskBufferIdx );
        }

        //-------------------------------------------------------------------------

        bool const isAdditive = pFinalBuffer->GetPrimaryPose()->IsAdditivePose();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            EE_ASSERT( !pFinalBuffer->m_poses[poseIdx].IsPoseSet() || pFinalBuffer->m_poses[poseIdx].IsAdditivePose() == isAdditive );
        }

        //-------------------------------------------------------------------------

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString OverlayBlendTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        InlineString str;
        if ( m_boneMaskTaskList.HasTasks() )
        {
            str = "Masked";
        }
        return str;
    }
    #endif

    //-------------------------------------------------------------------------

    AdditiveBlendTask::AdditiveBlendTask( int8_t sourceTaskIdx, int8_t targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : BlendTaskBase( TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
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

        // If there's no target pose set, then this is a no-op
        if ( !pTargetBuffer->IsPoseSet() )
        {
            ReleaseDependencyPoseBuffer( context, 1 );
            MarkTaskComplete( context );
            return;
        }

        // Check that the target pose is an additive pose
        if ( !pTargetBuffer->IsAdditivePose() )
        {
            EE_DEVELOPMENT_TOOLS_ONLY( context.LogError( m_sourcePath, "AdditiveBlendTask - Trying to perform an additive blend using a parent space pose" ) );

            pFinalBuffer->ResetPose();
            ReleaseDependencyPoseBuffer( context, 1 );
            MarkTaskComplete( context );
            return;
        }

        // Bone Mask
        //-------------------------------------------------------------------------

        int8_t boneMaskBufferIdx = InvalidIndex;
        BoneMaskBuffer const* pBoneMaskBuffer = nullptr;
        if ( m_boneMaskTaskList.HasTasks() )
        {
            boneMaskBufferIdx = m_boneMaskTaskList.GenerateBoneMask( pSourceBuffer->GetPrimarySkeleton(), context.m_boneMaskPool );
            EE_ASSERT( boneMaskBufferIdx != InvalidIndex );
            pBoneMaskBuffer = context.m_boneMaskPool.GetBuffer( boneMaskBufferIdx );
        }

        // Additive Blend
        //-------------------------------------------------------------------------

        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 0; poseIdx < numPoses; poseIdx++ )
        {
            // Skip any unset or zero additives
            if ( pTargetBuffer->m_poses[poseIdx].IsZeroPose() || !pTargetBuffer->m_poses[poseIdx].IsPoseSet() )
            {
                continue;
            }

            EE_ASSERT( pTargetBuffer->m_poses[poseIdx].IsAdditivePose() );

            //-------------------------------------------------------------------------

            BoneMask const* pBoneMask = ( pBoneMaskBuffer != nullptr ) ? pBoneMaskBuffer->TryGetBoneMask( pSourceBuffer->m_poses[poseIdx].GetSkeleton() ) : nullptr;

            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            if ( hasSourcePose )
            {
                Blender::AdditiveBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, pBoneMask, &pFinalBuffer->m_poses[poseIdx] );
            }
            else // Apply the additive to the reference pose
            {
                Blender::ApplyAdditiveToReferencePose( context.m_skeletonLOD, &pTargetBuffer->m_poses[poseIdx], m_blendWeight, pBoneMask, &pFinalBuffer->m_poses[poseIdx] );
            }

            #if EE_DEVELOPMENT_TOOLS
            if ( poseIdx == 0 && context.m_posePool.IsRecordingEnabled() && pBoneMask != nullptr )
            {
                m_debugBoneMask = *pBoneMask;
            }
            #endif
        }

        //-------------------------------------------------------------------------

        if ( boneMaskBufferIdx != InvalidIndex )
        {
            context.m_boneMaskPool.ReleaseMaskBuffer( boneMaskBufferIdx );
        }

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString AdditiveBlendTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        InlineString str;
        if ( m_boneMaskTaskList.HasTasks() )
        {
            str = "Masked";
        }
        return str;
    }
    #endif

    //-------------------------------------------------------------------------

    ModelSpaceBlendTask::ModelSpaceBlendTask( int8_t baseTaskIdx, int8_t layerTaskIdx, float const blendWeight, BoneMaskTaskList const& boneMaskTaskList )
        : BlendTaskBase( TaskUpdateStage::Any, { baseTaskIdx, layerTaskIdx } )
    {
        m_blendWeight = blendWeight;
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );
        EE_ASSERT( boneMaskTaskList.HasTasks() );
        m_boneMaskTaskList.CopyFrom( boneMaskTaskList );
    }

    void ModelSpaceBlendTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();

        // Get working buffers
        //-------------------------------------------------------------------------

        auto pSourceBuffer = AccessDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = TransferDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pTargetBuffer;

        // If neither pose is set, this is a no-op
        if ( !pSourceBuffer->IsPoseSet() && !pTargetBuffer->IsPoseSet() )
        {
            ReleaseDependencyPoseBuffer( context, 0 );
            MarkTaskComplete( context );
            return;
        }

        // Check that both poses are parent-space
        if ( pSourceBuffer->IsAdditivePose() || pTargetBuffer->IsAdditivePose() )
        {
            EE_DEVELOPMENT_TOOLS_ONLY( context.LogError( m_sourcePath, "ModelSpaceBlendTask - Trying to perform a model-space blend using an additive pose" ) );

            pFinalBuffer->ResetPose( Pose::Init::ReferencePose );
            ReleaseDependencyPoseBuffer( context, 0 );
            MarkTaskComplete( context );
            return;
        }

        // Primary Pose
        //-------------------------------------------------------------------------

        auto pPrimaryPose = pSourceBuffer->GetPrimaryPose();
        int8_t const boneMaskBufferIdx = m_boneMaskTaskList.GenerateBoneMask( pPrimaryPose->GetSkeleton(), context.m_boneMaskPool );
        EE_ASSERT( boneMaskBufferIdx != InvalidIndex );

        BoneMaskBuffer const* pBoneMaskBuffer = context.m_boneMaskPool.GetBuffer( boneMaskBufferIdx );
        Blender::ModelSpaceBlend( context.m_skeletonLOD, pSourceBuffer->GetPrimaryPose(), pTargetBuffer->GetPrimaryPose(), m_blendWeight, &pBoneMaskBuffer->m_masks[0], pFinalBuffer->GetPrimaryPose() );

        #if EE_DEVELOPMENT_TOOLS
        if ( context.m_posePool.IsRecordingEnabled() )
        {
            m_debugBoneMask.CopyFrom( pBoneMaskBuffer->m_masks[0] );
        }
        #endif

        // Secondary Poses
        //-------------------------------------------------------------------------
        // Since a model-space blend is an overlay blend - if a secondary pose is not set in the overlay, then we ignore the blend

        int32_t const numPoses = (int32_t) pSourceBuffer->m_poses.size();
        for ( int32_t poseIdx = 1; poseIdx < numPoses; poseIdx++ )
        {
            bool const hasTargetPose = pTargetBuffer->m_poses[poseIdx].IsPoseSet();
            if ( !hasTargetPose )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            BoneMask const* pMask = pBoneMaskBuffer->TryGetBoneMask( pSourceBuffer->m_poses[poseIdx].GetSkeleton() );

            bool const hasSourcePose = pSourceBuffer->m_poses[poseIdx].IsPoseSet();
            if ( !hasSourcePose )
            {
                pSourceBuffer->m_poses[poseIdx].Reset( Pose::Init::ReferencePose );
            }

            Blender::ParentSpaceBlend( context.m_skeletonLOD, &pSourceBuffer->m_poses[poseIdx], &pTargetBuffer->m_poses[poseIdx], m_blendWeight, pMask, &pFinalBuffer->m_poses[poseIdx] );
        }

        // Release buffers and complete task
        //-------------------------------------------------------------------------

        context.m_boneMaskPool.ReleaseMaskBuffer( boneMaskBufferIdx );
        ReleaseDependencyPoseBuffer( context, 0 );
        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString ModelSpaceBlendTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        InlineString str( InlineString::CtorSprintf(), "Weight: %.2f%%", m_blendWeight );
        return str;
    }
    #endif
}
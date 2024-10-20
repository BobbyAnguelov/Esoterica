#include "Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    Task::Task( TaskUpdateStage updateRequirements, TaskDependencies const& dependencies )
        : m_updateStage( updateRequirements )
        , m_dependencies( dependencies )
    {}

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Task::DrawSecondaryPoses( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled )
    {
        Pose const* pPrimaryPose = &pRecordedPoseBuffer->m_poses[0];
        Skeleton const* pPrimarySkeleton = pPrimaryPose->GetSkeleton();

        int32_t numPoses = (int32_t) pRecordedPoseBuffer->m_poses.size();
        for ( int32_t i = 1; i < numPoses; i++ )
        {
            Pose const* pSecondaryPose = &pRecordedPoseBuffer->m_poses[i];
            Skeleton const* pSecondarySkeleton = pSecondaryPose->GetSkeleton();

            // Offset pose based on default preview attachment socketID
            Transform poseWorldTransform = worldTransform;
            if ( pSecondarySkeleton->GetPreviewAttachmentSocketID().IsValid() )
            {
                int32_t const boneIdx = pPrimarySkeleton->GetBoneIndex( pSecondarySkeleton->GetPreviewAttachmentSocketID() );
                if ( boneIdx != InvalidIndex )
                {
                    poseWorldTransform = pPrimaryPose->GetModelSpaceTransform( boneIdx ) * worldTransform;
                }
            }

            // Draw pose
            pSecondaryPose->DrawDebug( drawingContext, poseWorldTransform, lod, pSecondaryPose->IsPoseSet() ? Colors::Cyan : Colors::LightGray.GetAlphaVersion( 0.25f ), 3.0f, isDetailedViewEnabled );
        }
    }

    //-------------------------------------------------------------------------

    void Task::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        pRecordedPoseBuffer->m_poses[0].DrawDebug( drawingContext, worldTransform, lod, GetDebugColor(), 3.0f, false );
        DrawSecondaryPoses( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );
    }
    #endif
}
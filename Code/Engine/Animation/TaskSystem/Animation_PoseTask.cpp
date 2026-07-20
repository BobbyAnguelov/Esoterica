#include "Animation_PoseTask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    void TaskContext::LogWarning( SourcePath const &sourcePath, char const *pFormat, ... ) const
    {
        TaskLogEntry &entry = m_pLog->emplace_back();
        entry.m_sourcePath = sourcePath;
        entry.m_severity = Severity::Warning;

        InlineString str;

        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }

    void TaskContext::LogError( SourcePath const& sourcePath, char const *pFormat, ... ) const
    {
        TaskLogEntry &entry = m_pLog->emplace_back();
        entry.m_sourcePath = sourcePath;
        entry.m_severity = Severity::Error;

        InlineString str;

        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }
    #endif

    //-------------------------------------------------------------------------

    PoseTask::PoseTask( TaskUpdateStage updateRequirements, TaskDependencies const& dependencies )
        : m_updateStage( updateRequirements )
        , m_dependencies( dependencies )
    {}

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PoseTask::DrawSecondaryPoses( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled )
    {
        Pose const* pPrimaryPose = &pRecordedPoseBuffer->m_poses[0];
        Skeleton const* pPrimarySkeleton = pPrimaryPose->GetSkeleton();

        int32_t numPoses = (int32_t) pRecordedPoseBuffer->m_poses.size();
        for ( int32_t i = 1; i < numPoses; i++ )
        {
            if ( !pRecordedPoseBuffer->m_poses[i].IsPoseSet() )
            {
                continue;
            }

            Pose const* pSecondaryPose = &pRecordedPoseBuffer->m_poses[i];
            Skeleton const* pSecondarySkeleton = pSecondaryPose->GetSkeleton();

            // Offset pose based on default preview attachment socketID
            Transform poseWorldTransform = worldTransform;
            StringID parentBoneID = pPrimarySkeleton->GetPreviewAttachmentBoneIDForSecondarySkeleton( pSecondarySkeleton );
            if ( parentBoneID.IsValid() )
            {
                int32_t const boneIdx = pPrimarySkeleton->GetBoneIndex( parentBoneID );
                if ( boneIdx != InvalidIndex )
                {
                    poseWorldTransform = pPrimaryPose->GetModelSpaceTransform( boneIdx ) * worldTransform;
                }
            }

            // Draw pose
            DrawOptions options;
            options.m_boneColor = pSecondaryPose->IsPoseSet() ? Colors::Cyan : Colors::LightGray.GetAlphaVersion( 0.25f );
            options.m_drawBoneNames = isDetailedViewEnabled;
            pSecondaryPose->DrawDebug( drawingContext, poseWorldTransform, lod, options );
        }
    }

    //-------------------------------------------------------------------------

    void PoseTask::DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        DrawOptions options;
        options.m_boneColor = GetDebugColor();
        options.m_drawBoneNames = isDetailedViewEnabled;

        pRecordedPoseBuffer->m_poses[0].DrawDebug( drawingContext, worldTransform, lod, options );
        DrawSecondaryPoses( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );
    }
    #endif
}
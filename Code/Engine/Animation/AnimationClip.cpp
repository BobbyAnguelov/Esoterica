#include "AnimationClip.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    FrameTime AnimationClip::GetFrameTime( Percentage percentageThrough ) const
    {
        FrameTime frameTime;

        Percentage const clampedTime = percentageThrough.GetClamped( false );
        if ( clampedTime == 1.0f )
        {
            frameTime = FrameTime( (int32_t) m_numFrames - 1 );
        }
        // Find time range in key list
        else if ( clampedTime != 0.0f )
        {
            frameTime = FrameTime( percentageThrough, m_numFrames - 1 );
        }

        return frameTime;
    }

    //-------------------------------------------------------------------------

    void AnimationClip::GetPose( FrameTime const& frameTime, Pose* pOutPose ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( pOutPose != nullptr && pOutPose->GetSkeleton() == m_pSkeleton.GetPtr() );
        EE_ASSERT( frameTime.GetFrameIndex() < m_numFrames );

        pOutPose->ClearGlobalTransforms();

        //-------------------------------------------------------------------------

        Transform boneTransform;
        uint16_t const* pTrackData = m_compressedPoseData.data();

        // Read exact key frame
        if ( frameTime.IsExactlyAtKeyFrame() )
        {
            auto const numBones = m_pSkeleton->GetNumBones();
            for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                pTrackData = ReadCompressedTrackKeyFrame( pTrackData, m_trackCompressionSettings[boneIdx], frameTime.GetFrameIndex(), boneTransform );
                pOutPose->m_localTransforms[boneIdx] = boneTransform;
            }
        }
        else // Read interpolated anim pose
        {
            auto const numBones = m_pSkeleton->GetNumBones();
            for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                pTrackData = ReadCompressedTrackTransform( pTrackData, m_trackCompressionSettings[boneIdx], frameTime, boneTransform );
                pOutPose->m_localTransforms[boneIdx] = boneTransform;
            }
        }

        // Flag the pose as being set
        pOutPose->m_state = m_isAdditive ? Pose::State::AdditivePose : Pose::State::Pose;
    }

    Transform AnimationClip::GetLocalSpaceTransform( int32_t boneIdx, FrameTime const& frameTime ) const
    {
        EE_ASSERT( IsValid() && m_pSkeleton->IsValidBoneIndex( boneIdx ) );

        uint32_t frameIdx = frameTime.GetFrameIndex();
        EE_ASSERT( frameIdx < m_numFrames );

        //-------------------------------------------------------------------------

        auto const& trackSettings = m_trackCompressionSettings[boneIdx];
        uint16_t const* pTrackData = m_compressedPoseData.data() + trackSettings.m_trackStartIndex;

        //-------------------------------------------------------------------------

        Transform boneLocalTransform;

        if ( frameTime.IsExactlyAtKeyFrame() )
        {
            ReadCompressedTrackKeyFrame( pTrackData, trackSettings, frameIdx, boneLocalTransform );
        }
        else
        {
            ReadCompressedTrackTransform( pTrackData, trackSettings, frameTime, boneLocalTransform );
        }
        return boneLocalTransform;
    }

    Transform AnimationClip::GetGlobalSpaceTransform( int32_t boneIdx, FrameTime const& frameTime ) const
    {
        EE_ASSERT( IsValid() && m_pSkeleton->IsValidBoneIndex( boneIdx ) );

        uint32_t frameIdx = frameTime.GetFrameIndex();
        EE_ASSERT( frameIdx < m_numFrames );

        // Find all parent bones
        //-------------------------------------------------------------------------

        TInlineVector<int32_t, 20> boneHierarchy;
        boneHierarchy.emplace_back( boneIdx );

        int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
        while ( parentBoneIdx != InvalidIndex )
        {
            boneHierarchy.emplace_back( parentBoneIdx );
            parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
        }

        // Calculate the global transform
        //-------------------------------------------------------------------------

        Transform globalTransform;

        if ( frameTime.IsExactlyAtKeyFrame() )
        {
            // Read root transform
            {
                auto const& trackSettings = m_trackCompressionSettings[boneHierarchy.back()];
                uint16_t const* pTrackData = m_compressedPoseData.data() + trackSettings.m_trackStartIndex;
                ReadCompressedTrackKeyFrame( pTrackData, trackSettings, frameIdx, globalTransform );
            }

            // Read and multiply out all the transforms moving down the hierarchy
            Transform localTransform;
            for ( int32_t i = (int32_t) boneHierarchy.size() - 2; i >= 0; i-- )
            {
                int32_t const trackIdx = boneHierarchy[i];
                auto const& trackSettings = m_trackCompressionSettings[trackIdx];
                uint16_t const* pTrackData = m_compressedPoseData.data() + trackSettings.m_trackStartIndex;
                ReadCompressedTrackKeyFrame( pTrackData, trackSettings, frameIdx, localTransform );

                globalTransform = localTransform * globalTransform;
            }
        }
        else // Interpolate key-frames
        {
            // Read root transform
            {
                auto const& trackSettings = m_trackCompressionSettings[boneHierarchy.back()];
                uint16_t const* pTrackData = m_compressedPoseData.data() + trackSettings.m_trackStartIndex;
                ReadCompressedTrackTransform( pTrackData, trackSettings, frameTime, globalTransform );
            }

            // Read and multiply out all the transforms moving down the hierarchy
            Transform localTransform;
            for ( int32_t i = (int32_t) boneHierarchy.size() - 2; i >= 0; i-- )
            {
                int32_t const trackIdx = boneHierarchy[i];
                auto const& trackSettings = m_trackCompressionSettings[trackIdx];
                uint16_t const* pTrackData = m_compressedPoseData.data() + trackSettings.m_trackStartIndex;
                ReadCompressedTrackTransform( pTrackData, trackSettings, frameTime, localTransform );

                globalTransform = localTransform * globalTransform;
            }
        }

        return globalTransform;
    }
}
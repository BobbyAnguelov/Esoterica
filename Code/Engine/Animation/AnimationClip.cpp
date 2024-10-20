#include "AnimationClip.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AnimationClip::GetPose( FrameTime const& frameTime, Pose* pOutPose, Skeleton::LOD lod ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( pOutPose != nullptr && pOutPose->GetSkeleton() == m_skeleton.GetPtr() );
        EE_ASSERT( frameTime.GetFrameIndex() < m_numFrames );

        pOutPose->ClearModelSpaceTransforms();

        //-------------------------------------------------------------------------

        int32_t const numBones = m_skeleton->GetNumBones( lod );

        auto ReadCompressedPose = [&] ( int32_t poseIdx, Transform outTransforms[] )
        {
            uint16_t const* pReadPtr = m_compressedPoseData.data() + m_compressedPoseOffsets[poseIdx];

            // Read rotations
            for ( auto i = 0; i < numBones; i++ )
            {
                TrackCompressionSettings const& trackSettings = m_trackCompressionSettings[i];

                //-------------------------------------------------------------------------

                Quaternion rotation;

                if ( trackSettings.IsRotationTrackStatic() )
                {
                    rotation = trackSettings.GetStaticRotationValue();
                }
                else
                {
                    rotation = DecodeRotation( pReadPtr );
                    pReadPtr += 3; // Rotations are 48bits (3 x uint16_t)
                }

                //-------------------------------------------------------------------------

                Float4 translationScale;

                if ( trackSettings.IsTranslationTrackStatic() )
                {
                    translationScale = Float4( trackSettings.GetStaticTranslationValue() );
                }
                else
                {
                    translationScale = DecodeTranslation( pReadPtr, trackSettings );
                    pReadPtr += 3; // Translations are 48bits (3 x uint16_t)
                }

                //-------------------------------------------------------------------------

                if ( trackSettings.IsScaleTrackStatic() )
                {
                    translationScale.m_w = trackSettings.GetStaticScaleValue();
                }
                else
                {
                    DecodeScale( pReadPtr, trackSettings );
                    pReadPtr += 1; // Scales are 16bits (1 x uint16_t)
                }

                //-------------------------------------------------------------------------

                Transform::DirectlySetRotation( outTransforms[i], rotation );
                Transform::DirectlySetTranslationScale( outTransforms[i], translationScale );
            }
        };

        //-------------------------------------------------------------------------

        // Read the lower frame pose into the output pose
        ReadCompressedPose( frameTime.GetLowerBoundFrameIndex(), pOutPose->m_parentSpaceTransforms.data() );

        // If we're not exactly at a key frame we need to read the upper frame pose and blend
        if ( !frameTime.IsExactlyAtKeyFrame() )
        {
            TInlineVector<Transform, 200> tmpPose;
            tmpPose.resize( numBones );
            ReadCompressedPose( frameTime.GetUpperBoundFrameIndex(), tmpPose.data() );

            float const percentageThrough = frameTime.GetPercentageThrough().ToFloat();
            for ( auto i = 0; i < numBones; i++ )
            {
                pOutPose->m_parentSpaceTransforms[i] = Transform::FastSLerp( pOutPose->m_parentSpaceTransforms[i], tmpPose[i], percentageThrough );
            }
        }

        // Flag the pose as being set
        pOutPose->m_state = m_isAdditive ? Pose::State::AdditivePose : Pose::State::Pose;
    }

    TInlineVector<Skeleton const*, 1> AnimationClip::GetSecondarySkeletons() const
    {
        TInlineVector<Skeleton const*, 1> skeletons;

        for ( auto pSecondaryAnim : m_secondaryAnimations )
        {
            skeletons.emplace_back( pSecondaryAnim->GetSkeleton() );
        }

        return skeletons;
    }
}
#include "AnimationClip.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AnimationClip::GetPose( FrameTime const& frameTime, Pose* pOutPose, Skeleton::LOD lod, bool sampleFloatChannels ) const
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

            for ( auto i = 0; i < numBones; i++ )
            {
                TrackDefinition const& trackSettings = m_trackDefs[i];

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
                    translationScale.m_w = DecodeScale( pReadPtr, trackSettings );
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

            if ( m_modelSpaceBoneSamplingIndices.empty() )
            {
                float const percentageThrough = frameTime.GetPercentageThrough();
                for ( auto i = 0; i < numBones; i++ )
                {
                    pOutPose->m_parentSpaceTransforms[i] = Transform::SLerp( pOutPose->m_parentSpaceTransforms[i], tmpPose[i], percentageThrough );
                }
            }
            else
            {
                auto CalculateModelSpaceTransformsForPose = [this] ( Transform* pLocalSpaceTransforms, TInlineVector<Transform, 30> &outModelSpaceTransforms )
                {
                    int32_t const numBonesInChain = (int32_t) m_modelSpaceSamplingChain.size();
                    outModelSpaceTransforms.resize( numBonesInChain );

                    for ( int32_t i = 0; i < numBonesInChain; i++ )
                    {
                        int32_t const boneIdx = m_modelSpaceSamplingChain[i].m_boneIdx;
                        if ( boneIdx == 0 )
                        {
                            outModelSpaceTransforms[i] = Transform::Identity;
                        }
                        else
                        {
                            int32_t parentChainIdx = m_modelSpaceSamplingChain[i].m_parentChainLinkIdx;
                            outModelSpaceTransforms[i] = pLocalSpaceTransforms[boneIdx] * outModelSpaceTransforms[parentChainIdx];
                        }
                    }
                };

                // Calculate all the required model space chains
                //-------------------------------------------------------------------------

                TInlineVector<Transform, 30> modelSpaceSourceTransforms;
                CalculateModelSpaceTransformsForPose( pOutPose->m_parentSpaceTransforms.data(), modelSpaceSourceTransforms );

                TInlineVector<Transform, 30> modelSpaceTargetTransforms;
                CalculateModelSpaceTransformsForPose( tmpPose.data(), modelSpaceTargetTransforms );

                float const percentageThrough = frameTime.GetPercentageThrough();
                for ( auto i = 0; i < numBones; i++ )
                {
                    pOutPose->m_parentSpaceTransforms[i] = Transform::SLerp( pOutPose->m_parentSpaceTransforms[i], tmpPose[i], percentageThrough );
                }

                TInlineVector<Transform, 30> modelSpaceResultTransforms;
                CalculateModelSpaceTransformsForPose( pOutPose->m_parentSpaceTransforms.data(), modelSpaceResultTransforms );

                // Blend specified bones in model space and convert back to local
                //-------------------------------------------------------------------------

                for ( int32_t linkIdx : m_modelSpaceBoneSamplingIndices )
                {
                    ModelSpaceSamplingChainLink const &info = m_modelSpaceSamplingChain[linkIdx];
                    Transform const blendedModelSpaceTransform = Transform::SLerp( modelSpaceSourceTransforms[linkIdx], modelSpaceTargetTransforms[linkIdx], percentageThrough );
                    if ( info.m_parentChainLinkIdx != InvalidIndex )
                    {
                        pOutPose->m_parentSpaceTransforms[info.m_boneIdx] = blendedModelSpaceTransform * modelSpaceResultTransforms[info.m_parentChainLinkIdx].GetInverse();
                    }
                    else
                    {
                        pOutPose->m_parentSpaceTransforms[info.m_boneIdx] = blendedModelSpaceTransform;
                    }
                }
            }
        }

        // Flag the pose as being set
        pOutPose->m_state = m_isAdditive ? Pose::State::AdditivePose : Pose::State::ParentSpacePose;

        // Float Channels
        //-------------------------------------------------------------------------

        if ( sampleFloatChannels )
        {
            size_t const numFloatChannelData = m_floatChannelSetData.size();
            for ( size_t i = 0; i < numFloatChannelData; i++ )
            {
                for ( size_t p = 0; p < pOutPose->m_floatChannelSetValues.size(); p++ )
                {
                    if ( pOutPose->m_floatChannelSetValues[p].GetSetID() == m_floatChannelSetData[i].GetSetID() )
                    {
                        m_floatChannelSetData[i].GetValues( frameTime, pOutPose->m_floatChannelSetValues[i] );
                    }
                }
            }
        }
    }

    Transform AnimationClip::GetParentSpaceTransform( FrameTime const& frameTime, int32_t boneIdx ) const
    {
        Transform transform;
        if ( !VectorContains( m_modelSpaceBoneSamplingIndices, boneIdx ) || frameTime.IsExactlyAtKeyFrame() )
        {
            GetParentSpaceTransform( frameTime, boneIdx, transform );
        }
        else // Model space interpolation
        {
            TInlineVector<BoneChainElement, 20> boneChain = GetModelSpaceTransform( frameTime, boneIdx );
            transform = boneChain.back().m_parentSpaceTransform;
        }

        return transform;
    }

    void AnimationClip::GetParentSpaceTransform( FrameTime const& frameTime, int32_t boneIdx, Transform& outTransform ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( frameTime.GetFrameIndex() < m_numFrames );
        EE_ASSERT( m_skeleton->IsValidBoneIndex( boneIdx ) );

        auto ReadCompressedTransform = [&] ( int32_t poseIdx, Transform& outDecompressedTransform )
        {
            TrackDefinition const& trackSettings = m_trackDefs[boneIdx];
            uint16_t const* pReadPtr = m_compressedPoseData.data() + m_compressedPoseOffsets[poseIdx] + trackSettings.m_trackReadOffset;

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
                translationScale.m_w = DecodeScale( pReadPtr, trackSettings );
                pReadPtr += 1; // Scales are 16bits (1 x uint16_t)
            }

            //-------------------------------------------------------------------------

            Transform::DirectlySetRotation( outDecompressedTransform, rotation );
            Transform::DirectlySetTranslationScale( outDecompressedTransform, translationScale );
        };

        // Read the lower frame pose into the output pose
        ReadCompressedTransform( frameTime.GetLowerBoundFrameIndex(), outTransform );

        // If we're not exactly at a key frame we need to read the upper frame transform and blend
        if ( !frameTime.IsExactlyAtKeyFrame() )
        {
            Transform nextTransform;
            ReadCompressedTransform( frameTime.GetUpperBoundFrameIndex(), nextTransform );

            float const percentageThrough = frameTime.GetPercentageThrough();
            outTransform = Transform::SLerp( outTransform, nextTransform, percentageThrough );
        }
    }

    TInlineVector<BoneChainElement, 20> AnimationClip::GetModelSpaceTransform( FrameTime const& frameTime, int32_t chainEndBoneIdx ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( frameTime.GetFrameIndex() < m_numFrames );
        EE_ASSERT( m_skeleton->IsValidBoneIndex( chainEndBoneIdx ) );

        // Get the entire chain
        TInlineVector<int32_t, 100> boneChainIndices;
        {
            int32_t chainBoneIdx = chainEndBoneIdx;
            while ( chainBoneIdx != InvalidIndex )
            {
                boneChainIndices.emplace_back( chainBoneIdx );
                chainBoneIdx = m_skeleton->GetParentBoneIndex( chainBoneIdx );
            }
        }

        EE_ASSERT( boneChainIndices.back() == 0 );
        int32_t const numBonesInChain = int32_t( boneChainIndices.size() );

        //-------------------------------------------------------------------------

        auto GetBoneChainParentTransforms = [this, &boneChainIndices, numBonesInChain] ( FrameTime const& frameTime, TInlineVector<BoneChainElement, 20>& chain )
        {
            for ( int32_t i = numBonesInChain - 1; i >= 0; i-- )
            {
                int32_t boneIdx = boneChainIndices[i];
                Transform const parentSpaceTransform = GetParentSpaceTransform( frameTime, boneIdx );
                chain.emplace_back( boneIdx, m_skeleton->GetBoneID( boneIdx ), parentSpaceTransform, parentSpaceTransform );
            }
        };

        TInlineVector<BoneChainElement, 20> chain;

        if ( !VectorContains( m_modelSpaceBoneSamplingIndices, chainEndBoneIdx ) || frameTime.IsExactlyAtKeyFrame() )
        {
            GetBoneChainParentTransforms( frameTime, chain );

            // Calculate model space transforms
            for ( int32_t i = 1; i < numBonesInChain; i++ )
            {
                chain[i].m_modelSpaceTransform = chain[i].m_parentSpaceTransform * chain[i - 1].m_modelSpaceTransform;
            }
        }
        else
        {
            TInlineVector<BoneChainElement, 20> lowerChain;
            TInlineVector<BoneChainElement, 20> upperChain;
            GetBoneChainParentTransforms( frameTime.GetLowerBoundFrameIndex(), lowerChain );
            GetBoneChainParentTransforms( frameTime.GetUpperBoundFrameIndex(), upperChain );

            // Calculate model space transforms for lower/upper frames and the final modelspace transform
            for ( int32_t i = 1; i < numBonesInChain; i++ )
            {
                lowerChain[i].m_modelSpaceTransform = lowerChain[i].m_parentSpaceTransform * lowerChain[i - 1].m_modelSpaceTransform;
                upperChain[i].m_modelSpaceTransform = upperChain[i].m_parentSpaceTransform * upperChain[i - 1].m_modelSpaceTransform;
            }

            // Get the blended model space chain
            //-------------------------------------------------------------------------

            float const percentageThrough = frameTime.GetPercentageThrough();

            GetBoneChainParentTransforms( frameTime, chain );

            for ( int32_t i = 1; i < numBonesInChain; i++ )
            {
                int32_t const boneIdx = boneChainIndices[i];

                if ( VectorContains( m_modelSpaceBoneSamplingIndices, boneIdx )  )
                {
                    Transform const blendedModelSpaceTransform = Transform::SLerp( lowerChain[i].m_modelSpaceTransform, upperChain[i].m_modelSpaceTransform, percentageThrough );
                    chain[i].m_modelSpaceTransform = blendedModelSpaceTransform;
                    chain[i].m_parentSpaceTransform = blendedModelSpaceTransform * chain[i - 1].m_modelSpaceTransform.GetInverse();
                }
                else
                {
                    chain[i].m_modelSpaceTransform = chain[i].m_parentSpaceTransform * chain[i - 1].m_modelSpaceTransform;
                }
            }
        }

        return chain;
    }

    //-------------------------------------------------------------------------

    TInlineVector<Skeleton const*, 1> AnimationClip::GetSecondarySkeletons() const
    {
        TInlineVector<Skeleton const*, 1> skeletons;

        for ( auto pSecondaryAnim : m_secondaryAnimations )
        {
            skeletons.emplace_back( pSecondaryAnim->GetSkeleton() );
        }

        return skeletons;
    }

    //-------------------------------------------------------------------------

    Percentage AnimationClip::GetPercentageThrough( FrameTime const &frameTime ) const
    {
        if ( IsSingleFrameAnimation() )
        {
            return Percentage( 1.0f );
        }

        int32_t lastFrameIdx = m_numFrames - 1;
        if ( frameTime.GetFrameIndex() == lastFrameIdx && frameTime.IsExactlyAtKeyFrame() )
        {
            return Percentage( 1.0f );
        }

        Percentage percentageThrough( frameTime.ToFloat() / ( m_numFrames - 1 ) );
        percentageThrough.Clamp( false );
        return percentageThrough;
    }

    void AnimationClip::GetEventsForRange( Percentage fromTime, Percentage toTime, TInlineVector<Event const *, 10> &outEvents ) const
    {
        EE_ASSERT( toTime >= fromTime );

        bool const bIncludeTrailingEdge = ( toTime == 1.0f );

        for ( auto const &pEvent : m_events )
        {
            // Events are stored sorted by time so as soon as we reach an event after the end of the time range, we're done
            if ( pEvent->GetStartTime() > toTime )
            {
                break;
            }

            // Perform a [from,to) check for any events
            if ( FloatRange( fromTime, toTime ).Overlaps( pEvent->GetTimeRange() ) )
            {
                if ( bIncludeTrailingEdge )
                {
                    outEvents.emplace_back( pEvent );
                }
                else // Exclude trailing edge
                {
                    if ( pEvent->GetStartTime() != toTime )
                    {
                        outEvents.emplace_back( pEvent );
                    }
                }
            }
        }
    }
}
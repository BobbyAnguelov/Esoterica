#include "ImportedAnimation.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    void AnimationClip::TruncateLength( int32_t numRequiredFrames )
    {
        EE_ASSERT( m_hasData );

        int32_t const numFrames = (int32_t) m_tracks[0].m_parentSpaceTransforms.size();
        EE_ASSERT( numRequiredFrames < numFrames );

        for ( auto& track : m_tracks )
        {
            track.m_parentSpaceTransforms.resize( numRequiredFrames );
            track.m_modelSpaceTransforms.resize( numRequiredFrames );
        }
    }

    void AnimationClip::GenerateModelSpaceTransforms()
    {
        EE_ASSERT( !m_tracks.empty() );

        int32_t const numFrames = (int32_t) m_tracks[0].m_parentSpaceTransforms.size();
        int32_t const numBones = GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& trackData = m_tracks[i];

            int32_t const parentBoneIdx = m_skeleton.GetParentBoneIndex( i );
            if ( parentBoneIdx == InvalidIndex )
            {
                trackData.m_modelSpaceTransforms = trackData.m_parentSpaceTransforms;
            }
            else // Calculate global transforms
            {
                auto const& parentTrackData = m_tracks[parentBoneIdx];
                trackData.m_modelSpaceTransforms.resize( numFrames );

                for ( auto f = 0; f < numFrames; f++ )
                {
                    trackData.m_modelSpaceTransforms[f] = trackData.m_parentSpaceTransforms[f] * parentTrackData.m_modelSpaceTransforms[f];
                }
            }
        }
    }

    void AnimationClip::RecreateParentSpaceTransformsFromModelSpaceTransform()
    {
        EE_ASSERT( !m_tracks.empty() );

        int32_t const numFrames = (int32_t) m_tracks[0].m_modelSpaceTransforms.size();
        int32_t const numBones = GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& trackData = m_tracks[i];

            int32_t const parentBoneIdx = m_skeleton.GetParentBoneIndex( i );
            if ( parentBoneIdx == InvalidIndex )
            {
                trackData.m_parentSpaceTransforms = trackData.m_modelSpaceTransforms;
            }
            else // Calculate local transforms
            {
                auto const& parentTrackData = m_tracks[parentBoneIdx];
                trackData.m_parentSpaceTransforms.resize( numFrames );

                for ( auto f = 0; f < numFrames; f++ )
                {
                    trackData.m_parentSpaceTransforms[f] = Transform::Delta( parentTrackData.m_modelSpaceTransforms[f], trackData.m_modelSpaceTransforms[f] );
                }
            }
        }
    }

    void AnimationClip::MakeAdditiveRelativeToSkeleton()
    {
        EE_ASSERT( !m_tracks.empty() );
        EE_ASSERT( m_hasData );

        uint32_t const numBones = m_skeleton.GetNumBones();
        int32_t const numFrames = (int32_t) m_tracks[0].m_modelSpaceTransforms.size();
        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            Transform baseTransform = m_skeleton.GetParentSpaceTransform( boneIdx );

            for ( int32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
            {
                Transform const& poseTransform = m_tracks[boneIdx].m_parentSpaceTransforms[frameIdx];

                Transform additiveTransform;
                additiveTransform.SetRotation( Quaternion::Delta( baseTransform.GetRotation(), poseTransform.GetRotation() ) );
                additiveTransform.SetTranslation( poseTransform.GetTranslation() - baseTransform.GetTranslation() );
                additiveTransform.SetScale( poseTransform.GetScale() - baseTransform.GetScale() );

                m_tracks[boneIdx].m_parentSpaceTransforms[frameIdx] = additiveTransform;
            }
        }
    }

    void AnimationClip::MakeAdditiveRelativeToFrame( int32_t baseFrameIdx )
    {
        EE_ASSERT( m_hasData );
        EE_ASSERT( !m_tracks.empty() );

        // Bones
        //-------------------------------------------------------------------------

        int32_t const numFrames = (int32_t) m_tracks[0].m_modelSpaceTransforms.size();
        uint32_t const numBones = m_skeleton.GetNumBones();

        // Copy reference pose
        TVector<Transform> basePose;
        basePose.reserve( numBones );

        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            basePose.emplace_back( m_tracks[boneIdx].m_parentSpaceTransforms[baseFrameIdx] );
        }

        // Generate Additive Data
        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            Transform baseTransform = basePose[boneIdx];

            for ( int32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
            {
                Transform const& poseTransform = m_tracks[boneIdx].m_parentSpaceTransforms[frameIdx];

                Transform additiveTransform;
                additiveTransform.SetRotation( Quaternion::Delta( baseTransform.GetRotation(), poseTransform.GetRotation() ) );
                additiveTransform.SetTranslation( poseTransform.GetTranslation() - baseTransform.GetTranslation() );
                additiveTransform.SetScale( poseTransform.GetScale() - baseTransform.GetScale() );

                m_tracks[boneIdx].m_parentSpaceTransforms[frameIdx] = additiveTransform;
            }
        }

        // Float Channels
        //-------------------------------------------------------------------------

        int32_t const numChannels = (int32_t) m_floatChannels.size();
        for ( int32_t channelIdx = 0; channelIdx < numChannels; channelIdx++ )
        {
            FloatChannelData &channel = m_floatChannels[channelIdx];

            // If we have no data, subtraction is meaningless
            if ( !channel.HasValue() )
            {
                continue;
            }

            float const baseValue = channel.IsStatic() ? channel.m_values[0] : channel.m_values[baseFrameIdx];

            // If the base value is zero, subtraction is meaningless
            if ( Math::IsNearZero( baseValue ) )
            {
                continue;
            }

            if ( channel.IsStatic() )
            {
                channel.m_values[0] = channel.m_values[0] - baseValue;
            }
            else // Process all values
            {
                for ( int32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
                {
                    channel.m_values[frameIdx] = channel.m_values[frameIdx] - baseValue;
                }
            }
        }
    }

    void AnimationClip::MakeAdditiveRelativeToAnimation( AnimationClip const& baseClip, int32_t baseFrameIdx )
    {
        EE_ASSERT( m_hasData && baseClip.m_hasData );
        EE_ASSERT( baseClip.m_skeleton.GetName() == m_skeleton.GetName() );

        int32_t const numBaseFrames = (int32_t) baseClip.m_tracks[0].m_parentSpaceTransforms.size();
        EE_ASSERT( baseFrameIdx < numBaseFrames );
        int32_t const numFrames = (int32_t) m_tracks[0].m_parentSpaceTransforms.size();
        EE_ASSERT( numBaseFrames >= numFrames );

        //-------------------------------------------------------------------------

        uint32_t const numBaseBones = baseClip.GetNumBones();
        uint32_t const numBones = m_skeleton.GetNumBones();
        EE_ASSERT( numBaseBones == numBones );

        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            // Resize tracks to new animation length
            m_tracks[boneIdx].m_parentSpaceTransforms.resize( numFrames );

            TVector<Transform> const& baseLocalTransforms = baseClip.m_tracks[boneIdx].m_parentSpaceTransforms;

            for ( int32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
            {
                int32_t actualBaseframeIdx = ( baseFrameIdx < 0 ) ? frameIdx : baseFrameIdx;
                Transform const& baseTransform = baseLocalTransforms[actualBaseframeIdx];
                Transform const& poseTransform = m_tracks[boneIdx].m_parentSpaceTransforms[frameIdx];

                Transform additiveTransform;
                additiveTransform.SetRotation( Quaternion::Delta( baseTransform.GetRotation(), poseTransform.GetRotation() ) );
                additiveTransform.SetTranslation( poseTransform.GetTranslation() - baseTransform.GetTranslation() );
                additiveTransform.SetScale( poseTransform.GetScale() - baseTransform.GetScale() );

                m_tracks[boneIdx].m_parentSpaceTransforms[frameIdx] = additiveTransform;
            }

            // Float Channels
            //-------------------------------------------------------------------------

            int32_t const numChannels = (int32_t) m_floatChannels.size();
            for ( int32_t channelIdx = 0; channelIdx < numChannels; channelIdx++ )
            {
                // If we dont have values in the base channel then subtraction is meaningless
                FloatChannelData const &baseChannel = baseClip.m_floatChannels[channelIdx];
                if ( !baseChannel.HasValue() || ( baseChannel.IsStatic() && Math::IsNearZero( baseChannel.m_values[0] ) ) )
                {
                    continue;
                }

                FloatChannelData &channel = m_floatChannels[channelIdx];
                if ( !channel.HasValue() || channel.IsStatic() )
                {
                    float const channelValue = channel.IsStatic() ? channel.m_values[0] : 0.0f;

                    for ( int32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
                    {
                        int32_t actualBaseframeIdx = ( baseFrameIdx < 0 ) ? frameIdx : baseFrameIdx;
                        float const baseValue = baseChannel.IsStatic() ? baseChannel.m_values[0] : baseChannel.m_values[actualBaseframeIdx];
                        channel.m_values.emplace_back( channelValue - baseValue );
                    }
                }
                else
                {
                    for ( int32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
                    {
                        int32_t actualBaseframeIdx = ( baseFrameIdx < 0 ) ? frameIdx : baseFrameIdx;
                        float const baseValue = baseChannel.IsStatic() ? baseChannel.m_values[0] : baseChannel.m_values[actualBaseframeIdx];
                        channel.m_values[frameIdx] = channel.m_values[frameIdx] - baseValue;
                    }
                }
            }
        }
    }

    void AnimationClip::SanitizeAllTransforms()
    {
        for ( auto& track : m_tracks )
        {
            for ( auto& transform : track.m_parentSpaceTransforms )
            {
                transform.Sanitize();
            }

            for ( auto& transform : track.m_modelSpaceTransforms )
            {
                transform.Sanitize();
            }
        }
    }

    //-------------------------------------------------------------------------

    void Animation::Finalize()
    {
        EE_ASSERT( m_numFrames > 0 );

        // Extract Root Motion
        //-------------------------------------------------------------------------

        m_rootTransforms.resize( m_numFrames );

        AnimationClip::TrackData& rootTrackData = m_primaryClip.m_tracks[0];
        Vector rootMotionOriginOffset = rootTrackData.m_parentSpaceTransforms[0].GetTranslation(); // Ensure that the root motion always starts at the origin

        for ( int32_t i = 0; i < m_numFrames; i++ )
        {
            // If we detect scaling on the root, log an error and exit
            if ( rootTrackData.m_parentSpaceTransforms[i].HasScale() )
            {
                LogError( "Root scaling detected! This is not allowed, please remove all scaling from the root bone!" );
                return;
            }

            // Extract root position and remove the origin offset from it
            m_rootTransforms[i] = rootTrackData.m_parentSpaceTransforms[i];
            m_rootTransforms[i].SetTranslation( m_rootTransforms[i].GetTranslation() - rootMotionOriginOffset );

            // Set the root tracks transform to Identity
            rootTrackData.m_parentSpaceTransforms[i] = Transform::Identity;
        }

        // Model Space Transforms
        //-------------------------------------------------------------------------

        m_primaryClip.GenerateModelSpaceTransforms();

        for ( auto &secondaryClip : m_secondaryClips )
        {
            secondaryClip.GenerateModelSpaceTransforms();
        }

        // Sanitize all transforms
        //-------------------------------------------------------------------------

        m_primaryClip.SanitizeAllTransforms();

        for ( auto &secondaryClip : m_secondaryClips )
        {
            secondaryClip.SanitizeAllTransforms();
        }
    }

    void Animation::RecreateParentSpaceTransformsFromModelSpaceTransform()
    {
        m_primaryClip.RecreateParentSpaceTransformsFromModelSpaceTransform();

        for ( auto &secondaryClip : m_secondaryClips )
        {
            secondaryClip.RecreateParentSpaceTransformsFromModelSpaceTransform();
        }
    }

    void Animation::MakeAdditiveRelativeToSkeleton()
    {
        m_primaryClip.MakeAdditiveRelativeToSkeleton();

        for ( auto &secondaryClip : m_secondaryClips )
        {
            if ( secondaryClip.m_hasData )
            {
                secondaryClip.MakeAdditiveRelativeToSkeleton();
            }
        }

        m_isAdditive = true;
    }

    void Animation::MakeAdditiveRelativeToFrame( int32_t baseFrameIdx )
    {
        EE_ASSERT( baseFrameIdx >= 0 && baseFrameIdx < ( m_numFrames - 1 ) );

        m_primaryClip.MakeAdditiveRelativeToFrame( baseFrameIdx );

        for ( auto &secondaryClip : m_secondaryClips )
        {
            if ( secondaryClip.m_hasData )
            {
                secondaryClip.MakeAdditiveRelativeToFrame( baseFrameIdx );
            }
        }

        m_isAdditive = true;
    }

    void Animation::MakeAdditiveRelativeToAnimation( Animation const& baseAnimation, int32_t baseFrameIdx )
    {
        EE_ASSERT( baseAnimation.m_secondaryClips.size() == m_secondaryClips.size() );
        EE_ASSERT( baseFrameIdx < m_numFrames );

        if ( baseAnimation.GetNumFrames() < GetNumFrames() )
        {
            LogWarning( "Base Additive Animation has less frames than required so we are truncating this animation to the same length as the base animation" );

            m_numFrames = baseAnimation.GetNumFrames();
            m_duration = baseAnimation.GetDuration();

            m_primaryClip.TruncateLength( m_numFrames );

            for ( auto &secondaryClip : m_secondaryClips )
            {
                if ( secondaryClip.m_hasData )
                {
                    secondaryClip.TruncateLength( m_numFrames );
                }
            }
        }

        //-------------------------------------------------------------------------

        m_primaryClip.MakeAdditiveRelativeToAnimation( baseAnimation.m_primaryClip, baseFrameIdx );

        int32_t const numClips = (int32_t) baseAnimation.m_secondaryClips.size();
        for ( int32_t i = 0; i < numClips; i++ )
        {
            if ( m_secondaryClips[i].m_hasData )
            {
                if ( baseAnimation.m_secondaryClips[i].m_hasData )
                {
                    m_secondaryClips[i].MakeAdditiveRelativeToAnimation( baseAnimation.m_secondaryClips[i], baseFrameIdx );
                }
                else
                {
                    m_secondaryClips[i].MakeAdditiveRelativeToSkeleton();
                }
            }
        }

        m_isAdditive = true;
    }
}
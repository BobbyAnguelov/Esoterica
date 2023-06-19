#include "RawAnimation.h"
#include "System/FileSystem/FileSystemPath.h"
#include "Formats/FBX.h"
#include "Formats/GLTF.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    void RawAnimation::Finalize()
    {
        EE_ASSERT( m_numFrames > 0 );

        // Global Transforms
        //-------------------------------------------------------------------------

        int32_t const numBones = GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& trackData = m_tracks[i];

            int32_t const parentBoneIdx = m_skeleton.GetParentBoneIndex( i );
            if ( parentBoneIdx == InvalidIndex )
            {
                trackData.m_globalTransforms = trackData.m_localTransforms;
            }
            else // Calculate global transforms
            {
                auto const& parentTrackData = m_tracks[parentBoneIdx];
                trackData.m_globalTransforms.resize( m_numFrames );

                for ( auto f = 0; f < m_numFrames; f++ )
                {
                    trackData.m_globalTransforms[f] = trackData.m_localTransforms[f] * parentTrackData.m_globalTransforms[f];
                }
            }
        }

        // Extract Root Motion
        //-------------------------------------------------------------------------

        m_rootTransforms.resize( m_numFrames );

        TrackData& rootTrackData = m_tracks[0];
        Vector rootMotionOriginOffset = rootTrackData.m_localTransforms[0].GetTranslation(); // Ensure that the root motion always starts at the origin

        for ( int32_t i = 0; i < m_numFrames; i++ )
        {
            // If we detect scaling on the root, log an error and exit
            if ( rootTrackData.m_localTransforms[i].HasScale() )
            {
                LogError( "Root scaling detected! This is not allowed, please remove all scaling from the root bone!" );
                return;
            }

            // Extract root position and remove the origin offset from it
            m_rootTransforms[i] = rootTrackData.m_localTransforms[i];
            m_rootTransforms[i].SetTranslation( m_rootTransforms[i].GetTranslation() - rootMotionOriginOffset );

            // Set the root tracks transform to Identity
            rootTrackData.m_localTransforms[i] = Transform::Identity;
            rootTrackData.m_globalTransforms[i] = Transform::Identity;
        }

        // Calculate component ranges
        //-------------------------------------------------------------------------

        CalculateComponentRanges();
    }

    void RawAnimation::RegenerateLocalTransforms()
    {
        int32_t const numBones = GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& trackData = m_tracks[i];

            int32_t const parentBoneIdx = m_skeleton.GetParentBoneIndex( i );
            if ( parentBoneIdx == InvalidIndex )
            {
                trackData.m_localTransforms = trackData.m_globalTransforms;
            }
            else // Calculate local transforms
            {
                auto const& parentTrackData = m_tracks[parentBoneIdx];
                trackData.m_localTransforms.resize( m_numFrames );

                for ( auto f = 0; f < m_numFrames; f++ )
                {
                    trackData.m_localTransforms[f] = Transform::Delta( parentTrackData.m_globalTransforms[f], trackData.m_globalTransforms[f] );
                }
            }
        }
    }

    void RawAnimation::MakeAdditiveRelativeToSkeleton()
    {
        uint32_t const numBones = m_skeleton.GetNumBones();
        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            Transform baseTransform = m_skeleton.GetLocalTransform( boneIdx );

            for ( int32_t frameIdx = 0; frameIdx < m_numFrames; frameIdx++ )
            {
                Transform const& poseTransform = m_tracks[boneIdx].m_localTransforms[frameIdx];

                Transform additiveTransform;
                additiveTransform.SetRotation( Quaternion::Delta( baseTransform.GetRotation(), poseTransform.GetRotation() ) );
                additiveTransform.SetTranslation( poseTransform.GetTranslation() - baseTransform.GetTranslation() );
                additiveTransform.SetScale( poseTransform.GetScale() - baseTransform.GetScale() );

                m_tracks[boneIdx].m_localTransforms[frameIdx] = additiveTransform;
            }
        }

        CalculateComponentRanges();
        m_isAdditive = true;
    }

    void RawAnimation::MakeAdditiveRelativeToFrame( int32_t baseFrameIdx )
    {
        EE_ASSERT( baseFrameIdx >= 0 && baseFrameIdx < ( m_numFrames - 1 ) );

        uint32_t const numBones = m_skeleton.GetNumBones();

        // Copy reference pose
        TVector<Transform> basePose;
        basePose.reserve( numBones );

        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            basePose.emplace_back( m_tracks[boneIdx].m_localTransforms[baseFrameIdx] );
        }

        // Generate Additive Data
        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            Transform baseTransform = basePose[boneIdx];

            for ( int32_t frameIdx = 0; frameIdx < m_numFrames; frameIdx++ )
            {
                Transform const& poseTransform = m_tracks[boneIdx].m_localTransforms[frameIdx];

                Transform additiveTransform;
                additiveTransform.SetRotation( Quaternion::Delta( baseTransform.GetRotation(), poseTransform.GetRotation() ) );
                additiveTransform.SetTranslation( poseTransform.GetTranslation() - baseTransform.GetTranslation() );
                additiveTransform.SetScale( poseTransform.GetScale() - baseTransform.GetScale() );

                m_tracks[boneIdx].m_localTransforms[frameIdx] = additiveTransform;
            }
        }

        CalculateComponentRanges();
        m_isAdditive = true;
    }

    void RawAnimation::MakeAdditiveRelativeToAnimation( RawAnimation const& baseAnimation )
    {
        EE_ASSERT( !IsAdditive() );
        EE_ASSERT( baseAnimation.IsValid() && !baseAnimation.IsAdditive() );
        EE_ASSERT( baseAnimation.m_skeleton.GetName() == m_skeleton.GetName() );

        if ( baseAnimation.GetNumFrames() < GetNumFrames() )
        {
            LogWarning( "Base Additive Animation has less frames than required so we are truncating this animation to the same length as the base animation" );
            m_numFrames = baseAnimation.GetNumFrames();
            m_duration = baseAnimation.GetDuration();
        }

        //-------------------------------------------------------------------------

        uint32_t const numBones = m_skeleton.GetNumBones();
        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            // Resize tracks to new animation length
            m_tracks[boneIdx].m_localTransforms.resize( m_numFrames );

            TVector<Transform> const& baseLocalTransforms = baseAnimation.GetTrackData()[boneIdx].m_localTransforms;

            for ( int32_t frameIdx = 0; frameIdx < m_numFrames; frameIdx++ )
            {
                Transform const& baseTransform = baseLocalTransforms[frameIdx];
                Transform const& poseTransform = m_tracks[boneIdx].m_localTransforms[frameIdx];

                Transform additiveTransform;
                additiveTransform.SetRotation( Quaternion::Delta( baseTransform.GetRotation(), poseTransform.GetRotation()));
                additiveTransform.SetTranslation( poseTransform.GetTranslation() - baseTransform.GetTranslation() );
                additiveTransform.SetScale( poseTransform.GetScale() - baseTransform.GetScale() );

                m_tracks[boneIdx].m_localTransforms[frameIdx] = additiveTransform;
            }
        }

        CalculateComponentRanges();
        m_isAdditive = true;
    }

    void RawAnimation::CalculateComponentRanges()
    {
        for ( auto& track : m_tracks )
        {
            track.m_translationValueRangeX = FloatRange();
            track.m_translationValueRangeY = FloatRange();
            track.m_translationValueRangeZ = FloatRange();
            track.m_scaleValueRange = FloatRange();

            for ( auto const& transform : track.m_localTransforms )
            {
                Float3 const& translation = transform.GetTranslation().ToFloat3();
                track.m_translationValueRangeX.GrowRange( translation.m_x );
                track.m_translationValueRangeY.GrowRange( translation.m_y );
                track.m_translationValueRangeZ.GrowRange( translation.m_z );

                track.m_scaleValueRange.GrowRange( transform.GetScale() );
            }
        }
    }
}
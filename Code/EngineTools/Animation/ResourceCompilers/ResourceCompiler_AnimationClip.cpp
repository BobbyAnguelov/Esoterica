#include "ResourceCompiler_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/Events/AnimationEventTrack.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/RawAssets/RawAnimation.h"
#include "EngineTools/Core/TimelineEditor/TimelineTrackContainer.h"
#include "Engine/Animation/AnimationSyncTrack.h"
#include "Engine/Animation/AnimationClip.h"
#include "System/Resource/ResourcePtr.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Math/MathHelpers.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct AnimationClipEventData
    {
        TypeSystem::TypeDescriptorCollection            m_collection;
        TInlineVector<SyncTrack::EventMarker, 10>       m_syncEventMarkers;
    };

    //-------------------------------------------------------------------------

    AnimationClipCompiler::AnimationClipCompiler()
        : Resource::Compiler( "AnimationCompiler", s_version )
    {
        m_outputTypes.push_back( AnimationClip::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult AnimationClipCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        AnimationClipResourceDescriptor resourceDescriptor;

        Serialization::TypeArchiveReader typeReader( *m_pTypeRegistry );
        if ( !typeReader.ReadFromFile( ctx.m_inputFilePath ) )
        {
            return Error( "Failed to read resource descriptor file: %s", ctx.m_inputFilePath.c_str() );
        }

        if ( !typeReader.ReadType( &resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        FileSystem::Path animationFilePath;
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_animationPath, animationFilePath ) )
        {
            return Error( "Invalid animation data path: %s", resourceDescriptor.m_animationPath.c_str() );
        }

        if ( !FileSystem::Exists( animationFilePath ) )
        {
            return Error( "Invalid animation file path: %s", animationFilePath.ToString().c_str() );
        }

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        // Convert the skeleton resource path to a physical file path
        if ( !resourceDescriptor.m_pSkeleton.GetResourceID().IsValid() )
        {
            return Error( "Invalid skeleton resource ID" );
        }

        ResourcePath const& skeletonPath = resourceDescriptor.m_pSkeleton.GetResourcePath();
        FileSystem::Path skeletonDescriptorFilePath;
        if ( !ConvertResourcePathToFilePath( skeletonPath, skeletonDescriptorFilePath ) )
        {
            return Error( "Invalid skeleton data path: %s", skeletonPath.c_str() );
        }

        if ( !FileSystem::Exists( skeletonDescriptorFilePath ) )
        {
            return Error( "Invalid skeleton descriptor file path: %s", skeletonDescriptorFilePath.ToString().c_str() );
        }

        SkeletonResourceDescriptor skeletonResourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, skeletonDescriptorFilePath, skeletonResourceDescriptor ) )
        {
            return Error( "Failed to read skeleton resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        FileSystem::Path skeletonFilePath;
        if ( !ConvertResourcePathToFilePath( skeletonResourceDescriptor.m_skeletonPath, skeletonFilePath ) )
        {
            return Error( "Invalid skeleton FBX data path: %s", skeletonResourceDescriptor.m_skeletonPath.GetString().c_str() );
        }

        RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        auto pRawSkeleton = RawAssets::ReadSkeleton( readerCtx, skeletonFilePath, skeletonResourceDescriptor.m_skeletonRootBoneName );
        if ( pRawSkeleton == nullptr || !pRawSkeleton->IsValid() )
        {
            return Error( "Failed to read skeleton file: %s", skeletonFilePath.ToString().c_str() );
        }

        // Read animation data
        //-------------------------------------------------------------------------

        TUniquePtr<RawAssets::RawAnimation> pRawAnimation = RawAssets::ReadAnimation( readerCtx, animationFilePath, *pRawSkeleton, resourceDescriptor.m_animationName );
        if ( pRawAnimation == nullptr )
        {
            return Error( "Failed to read animation from source file" );
        }

        if ( resourceDescriptor.m_regenerateRootMotion )
        {
            if ( !RegenerateRootMotion( resourceDescriptor, pRawAnimation.get() ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }

        // Reflect raw animation data into runtime format
        //-------------------------------------------------------------------------

        AnimationClip animData;
        animData.m_pSkeleton = resourceDescriptor.m_pSkeleton;

        TransferAndCompressAnimationData( *pRawAnimation, animData );

        // Handle events
        //-------------------------------------------------------------------------

        AnimationClipEventData eventData;
        if ( !ReadEventsData( ctx, typeReader.GetDocument(), *pRawAnimation, eventData ) )
        {
            return CompilationFailed( ctx );
        }

        // Serialize animation data
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, AnimationClip::GetStaticResourceTypeID() );
        hdr.AddInstallDependency( resourceDescriptor.m_pSkeleton.GetResourceID() );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << animData;
        archive << eventData.m_syncEventMarkers;
        archive << eventData.m_collection;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            if ( pRawAnimation->HasWarnings() )
            {
                return CompilationSucceededWithWarnings( ctx );
            }
            else
            {
                return CompilationSucceeded( ctx );
            }
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationClipCompiler::RegenerateRootMotion( AnimationClipResourceDescriptor const& resourceDescriptor, RawAssets::RawAnimation* pRawAnimation ) const
    {
        EE_ASSERT( pRawAnimation != nullptr && pRawAnimation->IsValid() );

        // Validate Root Motion Generation settings
        //-------------------------------------------------------------------------

        int32_t rootMotionGenerationBoneIdx = InvalidIndex;
        if ( resourceDescriptor.m_regenerateRootMotion )
        {
            if ( resourceDescriptor.m_rootMotionGenerationBoneID.IsValid() )
            {
                rootMotionGenerationBoneIdx = pRawAnimation->GetSkeleton().GetBoneIndex( resourceDescriptor.m_rootMotionGenerationBoneID );
                if ( rootMotionGenerationBoneIdx == InvalidIndex )
                {
                    Error( "Root Motion Generation: Skeleton doesnt contain specified bone: %s", resourceDescriptor.m_rootMotionGenerationBoneID.c_str() );
                    return false;
                }
            }
            else
            {
                Error( "Root Motion Generation: No bone specified for root motion generation!" );
                return false;
            }
        }

        // Calculate root position based on specified bone
        //-------------------------------------------------------------------------

        auto const& sourceBoneTrackData = pRawAnimation->GetTrackData()[rootMotionGenerationBoneIdx];
        auto& rootTrackData = pRawAnimation->GetTrackData()[0];
        Quaternion const preRotation( resourceDescriptor.m_rootMotionGenerationPreRotation );

        // Get the Z value for the root on the first frame, use this as the starting point for the root motion track
        auto& rootMotion = pRawAnimation->GetRootMotion();
        float const flatZ = rootMotion.front().GetTranslation().m_z;
        float const offsetZ = ( rootMotion.front().GetTranslation() - sourceBoneTrackData.m_globalTransforms[0].GetTranslation() ).m_z;

        int32_t const numFrames = pRawAnimation->GetNumFrames();
        for ( auto i = 0; i < numFrames; i++ )
        {
            Transform const& rootMotionTransform = sourceBoneTrackData.m_globalTransforms[i];

            // Calculate root orientation
            Quaternion orientation = rootMotionTransform.GetRotation() * preRotation;
            EulerAngles orientationAngles = orientation.ToEulerAngles();
            orientationAngles.m_y = 0; // remove roll
            orientationAngles.m_x = 0; // remove pitch
            orientation = Quaternion( orientationAngles );

            // Calculate root position
            Vector translation = rootMotionTransform.GetTranslation();
            if ( resourceDescriptor.m_rootMotionGenerationRestrictToHorizontalPlane )
            {
                translation.m_z = flatZ;
            }
            else
            {
                translation.m_z += offsetZ;
            }

            // Set root position
            rootTrackData.m_globalTransforms[i] = Transform( orientation, translation );
        }

        // Regenerate the local transforms taking into account the new root position
        pRawAnimation->RegenerateLocalTransforms();
        pRawAnimation->Finalize();

        //-------------------------------------------------------------------------

        return true;
    }

    void AnimationClipCompiler::TransferAndCompressAnimationData( RawAssets::RawAnimation const& rawAnimData, AnimationClip& animClip ) const
    {
        auto const& rawTrackData = rawAnimData.GetTrackData();
        uint32_t const numBones = rawAnimData.GetNumBones();
        uint32_t const numFrames = rawAnimData.GetNumFrames();

        // Transfer basic animation data
        //-------------------------------------------------------------------------

        animClip.m_numFrames = rawAnimData.GetNumFrames();
        animClip.m_duration = ( animClip.IsSingleFrameAnimation() ) ? 0.0f : rawAnimData.GetDuration();
        animClip.m_rootMotion.m_transforms = rawAnimData.GetRootMotion();

        // Calculate root motion extra data
        //-------------------------------------------------------------------------

        float totalDistance = 0.0f;
        float totalRotation = 0.0f;

        for ( auto i = 0u; i < animClip.GetNumFrames(); i++ )
        {
            // Track deltas
            if ( i > 0 )
            {
                Transform const deltaRoot = Transform::DeltaNoScale( animClip.m_rootMotion.m_transforms[i - 1], animClip.m_rootMotion.m_transforms[i] );
                totalDistance += deltaRoot.GetTranslation().GetLength3();

                // If we have a rotation delta, accumulate the yaw value
                if ( !deltaRoot.GetRotation().IsIdentity() )
                {
                    Vector const deltaForward2D = deltaRoot.GetForwardVector().GetNormalized2();
                    Radians const deltaAngle = Math::GetYawAngleBetweenVectors( deltaForward2D, Vector::WorldBackward ).GetClamped360();
                    totalRotation += Math::Abs( (float) deltaAngle );
                }
            }
        }

        animClip.m_rootMotion.m_totalDelta = Transform::DeltaNoScale( animClip.m_rootMotion.m_transforms.front(), animClip.m_rootMotion.m_transforms.back() );
        animClip.m_rootMotion.m_averageLinearVelocity = totalDistance / animClip.GetDuration();
        animClip.m_rootMotion.m_averageAngularVelocity = totalRotation / animClip.GetDuration();

        // Compress raw data
        //-------------------------------------------------------------------------

        static constexpr float const defaultQuantizationRangeLength = 0.1f;

        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            TrackCompressionSettings trackSettings;

            // Record offset into data for this track
            trackSettings.m_trackStartIndex = (uint32_t) animClip.m_compressedPoseData.size();

            //-------------------------------------------------------------------------
            // Rotation
            //-------------------------------------------------------------------------

            for ( uint32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[frameIdx];
                Quaternion const rotation = rawBoneTransform.GetRotation();

                Quantization::EncodedQuaternion const encodedQuat( rotation );
                animClip.m_compressedPoseData.push_back( encodedQuat.GetData0() );
                animClip.m_compressedPoseData.push_back( encodedQuat.GetData1() );
                animClip.m_compressedPoseData.push_back( encodedQuat.GetData2() );
            }

            //-------------------------------------------------------------------------
            // Translation
            //-------------------------------------------------------------------------

            auto const& rawTranslationValueRangeX = rawTrackData[boneIdx].m_translationValueRangeX;
            auto const& rawTranslationValueRangeY = rawTrackData[boneIdx].m_translationValueRangeY;
            auto const& rawTranslationValueRangeZ = rawTrackData[boneIdx].m_translationValueRangeZ;

            float const& rawTranslationValueRangeLengthX = rawTranslationValueRangeX.GetLength();
            float const& rawTranslationValueRangeLengthY = rawTranslationValueRangeY.GetLength();
            float const& rawTranslationValueRangeLengthZ = rawTranslationValueRangeZ.GetLength();

            // We could arguably compress more by saving each component individually at the cost of sampling performance. If we absolutely need more compression, we can do it here
            bool const isTranslationConstant = Math::IsNearZero( rawTranslationValueRangeLengthX ) && Math::IsNearZero( rawTranslationValueRangeLengthY ) && Math::IsNearZero( rawTranslationValueRangeLengthZ );
            if ( isTranslationConstant )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[0];
                Vector const& translation = rawBoneTransform.GetTranslation();

                trackSettings.m_translationRangeX = { translation.m_x, defaultQuantizationRangeLength };
                trackSettings.m_translationRangeY = { translation.m_y, defaultQuantizationRangeLength };
                trackSettings.m_translationRangeZ = { translation.m_z, defaultQuantizationRangeLength };
                trackSettings.m_isTranslationStatic = true;
            }
            else
            {
                trackSettings.m_translationRangeX = { rawTranslationValueRangeX.m_begin, Math::IsNearZero( rawTranslationValueRangeLengthX ) ? defaultQuantizationRangeLength : rawTranslationValueRangeLengthX };
                trackSettings.m_translationRangeY = { rawTranslationValueRangeY.m_begin, Math::IsNearZero( rawTranslationValueRangeLengthY ) ? defaultQuantizationRangeLength : rawTranslationValueRangeLengthY };
                trackSettings.m_translationRangeZ = { rawTranslationValueRangeZ.m_begin, Math::IsNearZero( rawTranslationValueRangeLengthZ ) ? defaultQuantizationRangeLength : rawTranslationValueRangeLengthZ };
            }

            //-------------------------------------------------------------------------

            if ( trackSettings.IsTranslationTrackStatic() )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[0];
                Vector const& translation = rawBoneTransform.GetTranslation();

                uint16_t const m_x = Quantization::EncodeFloat( translation.m_x, trackSettings.m_translationRangeX.m_rangeStart, trackSettings.m_translationRangeX.m_rangeLength );
                uint16_t const m_y = Quantization::EncodeFloat( translation.m_y, trackSettings.m_translationRangeY.m_rangeStart, trackSettings.m_translationRangeY.m_rangeLength );
                uint16_t const m_z = Quantization::EncodeFloat( translation.m_z, trackSettings.m_translationRangeZ.m_rangeStart, trackSettings.m_translationRangeZ.m_rangeLength );

                animClip.m_compressedPoseData.push_back( m_x );
                animClip.m_compressedPoseData.push_back( m_y );
                animClip.m_compressedPoseData.push_back( m_z );
            }
            else // Store all frames
            {
                for ( uint32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[frameIdx];
                    Vector const& translation = rawBoneTransform.GetTranslation();

                    uint16_t const m_x = Quantization::EncodeFloat( translation.m_x, trackSettings.m_translationRangeX.m_rangeStart, trackSettings.m_translationRangeX.m_rangeLength );
                    uint16_t const m_y = Quantization::EncodeFloat( translation.m_y, trackSettings.m_translationRangeY.m_rangeStart, trackSettings.m_translationRangeY.m_rangeLength );
                    uint16_t const m_z = Quantization::EncodeFloat( translation.m_z, trackSettings.m_translationRangeZ.m_rangeStart, trackSettings.m_translationRangeZ.m_rangeLength );

                    animClip.m_compressedPoseData.push_back( m_x );
                    animClip.m_compressedPoseData.push_back( m_y );
                    animClip.m_compressedPoseData.push_back( m_z );
                }
            }

            //-------------------------------------------------------------------------
            // Scale
            //-------------------------------------------------------------------------

            auto const& rawScaleValueRangeX = rawTrackData[boneIdx].m_scaleValueRangeX;
            auto const& rawScaleValueRangeY = rawTrackData[boneIdx].m_scaleValueRangeY;
            auto const& rawScaleValueRangeZ = rawTrackData[boneIdx].m_scaleValueRangeZ;

            float const& rawScaleValueRangeLengthX = rawScaleValueRangeX.GetLength();
            float const& rawScaleValueRangeLengthY = rawScaleValueRangeY.GetLength();
            float const& rawScaleValueRangeLengthZ = rawScaleValueRangeZ.GetLength();

            // We could arguably compress more by saving each component individually at the cost of sampling performance. If we absolutely need more compression, we can do it here
            bool const isScaleConstant = Math::IsNearZero( rawScaleValueRangeLengthX ) && Math::IsNearZero( rawScaleValueRangeLengthY ) && Math::IsNearZero( rawScaleValueRangeLengthZ );
            if ( isScaleConstant )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[0];
                Vector const& scale = rawBoneTransform.GetScale();

                trackSettings.m_scaleRangeX = { scale.m_x, defaultQuantizationRangeLength };
                trackSettings.m_scaleRangeY = { scale.m_y, defaultQuantizationRangeLength };
                trackSettings.m_scaleRangeZ = { scale.m_z, defaultQuantizationRangeLength };
                trackSettings.m_isScaleStatic = true;
            }
            else
            {
                trackSettings.m_scaleRangeX = { rawScaleValueRangeX.m_begin, Math::IsNearZero( rawScaleValueRangeLengthX ) ? defaultQuantizationRangeLength : rawScaleValueRangeLengthX };
                trackSettings.m_scaleRangeY = { rawScaleValueRangeY.m_begin, Math::IsNearZero( rawScaleValueRangeLengthY ) ? defaultQuantizationRangeLength : rawScaleValueRangeLengthY };
                trackSettings.m_scaleRangeZ = { rawScaleValueRangeZ.m_begin, Math::IsNearZero( rawScaleValueRangeLengthZ ) ? defaultQuantizationRangeLength : rawScaleValueRangeLengthZ };
            }

            //-------------------------------------------------------------------------

            if ( trackSettings.IsScaleTrackStatic() )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[0];
                Vector const& scale = rawBoneTransform.GetScale();

                uint16_t const m_x = Quantization::EncodeFloat( scale.m_x, trackSettings.m_scaleRangeX.m_rangeStart, trackSettings.m_scaleRangeX.m_rangeLength );
                uint16_t const m_y = Quantization::EncodeFloat( scale.m_y, trackSettings.m_scaleRangeY.m_rangeStart, trackSettings.m_scaleRangeY.m_rangeLength );
                uint16_t const m_z = Quantization::EncodeFloat( scale.m_z, trackSettings.m_scaleRangeZ.m_rangeStart, trackSettings.m_scaleRangeZ.m_rangeLength );

                animClip.m_compressedPoseData.push_back( m_x );
                animClip.m_compressedPoseData.push_back( m_y );
                animClip.m_compressedPoseData.push_back( m_z );
            }
            else // Store all frames
            {
                for ( uint32_t frameIdx = 0; frameIdx < numFrames; frameIdx++ )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[frameIdx];
                    Vector const& scale = rawBoneTransform.GetScale();

                    uint16_t const m_x = Quantization::EncodeFloat( scale.m_x, trackSettings.m_scaleRangeX.m_rangeStart, trackSettings.m_scaleRangeX.m_rangeLength );
                    uint16_t const m_y = Quantization::EncodeFloat( scale.m_y, trackSettings.m_scaleRangeY.m_rangeStart, trackSettings.m_scaleRangeY.m_rangeLength );
                    uint16_t const m_z = Quantization::EncodeFloat( scale.m_z, trackSettings.m_scaleRangeZ.m_rangeStart, trackSettings.m_scaleRangeZ.m_rangeLength );

                    animClip.m_compressedPoseData.push_back( m_x );
                    animClip.m_compressedPoseData.push_back( m_y );
                    animClip.m_compressedPoseData.push_back( m_z );
                }
            }

            //-------------------------------------------------------------------------

            animClip.m_trackCompressionSettings.emplace_back( trackSettings );
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationClipCompiler::ReadEventsData( Resource::CompileContext const& ctx, rapidjson::Document const& document, RawAssets::RawAnimation const& rawAnimData, AnimationClipEventData& outEventData ) const
    {
        EE_ASSERT( document.IsObject() );

        // Read event track data
        //-------------------------------------------------------------------------

        // Check if there is potential additional event data
        auto trackDataIter = document.FindMember( Timeline::TrackContainer::s_trackContainerKey );
        if ( trackDataIter == document.MemberEnd() )
        {
            return true;
        }

        auto const& eventDataValueObject = trackDataIter->value;
        if ( !eventDataValueObject.IsArray() )
        {
            Error( "Malformed event track data" );
            return false;
        }

        Timeline::TrackContainer trackContainer;
        if ( !trackContainer.Serialize( *m_pTypeRegistry, eventDataValueObject ) )
        {
            Error( "Malformed event track data" );
            return false;
        }

        // Reflect into runtime events
        //-------------------------------------------------------------------------

        int32_t numSyncTracks = 0;
        float const numIntervals = float( rawAnimData.GetNumFrames() - 1 );
        TVector<Event*> events;
        FloatRange const animationTimeRange( 0, numIntervals );
        for ( Timeline::Track* pTrack : trackContainer.m_tracks )
        {
            if ( pTrack->GetStatus() == Timeline::Track::Status::HasErrors )
            {
                if ( pTrack->IsRenameable() )
                {
                    Warning( "Invalid animation event track (%s - %s) encountered!", pTrack->GetName(), pTrack->GetTypeName() );
                }
                else
                {
                    Warning( "Invalid animation event track (%s) encountered!", pTrack->GetTypeName() );
                }

                // Skip invalid tracks
                continue;
            }

            //-------------------------------------------------------------------------

            if ( pTrack->GetStatus() != Timeline::Track::Status::HasWarnings )
            {
                if ( pTrack->IsRenameable() )
                {
                    Warning( "Animation event track (Track: %s, Type: %s) has warnings: %s", pTrack->GetName(), pTrack->GetTypeName(), pTrack->GetStatusMessage().c_str() );
                }
                else
                {
                    Warning( "Animation event track (%s) has warnings: %s", pTrack->GetTypeName(), pTrack->GetStatusMessage().c_str() );
                }
            }

            //-------------------------------------------------------------------------

            auto pEventTrack = Cast<EventTrack>( pTrack );

            if ( pEventTrack->IsSyncTrack() )
            {
                numSyncTracks++;
            }

            for ( auto const pItem : pTrack->GetItems() )
            {
                auto pEvent = Cast<Event>( pItem->GetData() );

                // Add event
                //-------------------------------------------------------------------------

                // Ignore any event entirely outside the animation time range
                if ( !animationTimeRange.Overlaps( pEvent->GetTimeRange() ) )
                {
                    Warning( "Event detected outside animation time range, event will be ignored" );
                    continue;
                }

                // Clamp events that extend out of the animation time range to the animation time range
                FloatRange eventTimeRange = pEvent->GetTimeRange();
                if ( !animationTimeRange.ContainsInclusive( pEvent->GetTimeRange() ) )
                {
                    Warning( "Event extend outside the valid animation time range, event will be clamped to animation range" );

                    eventTimeRange.m_begin = animationTimeRange.GetClampedValue( eventTimeRange.m_begin );
                    eventTimeRange.m_end = animationTimeRange.GetClampedValue( eventTimeRange.m_end );
                    EE_ASSERT( eventTimeRange.IsSetAndValid() );
                }

                // Set event time and add new event
                pEvent->m_startTime = eventTimeRange.m_begin / numIntervals;
                pEvent->m_duration = eventTimeRange.GetLength() / numIntervals;
                events.emplace_back( pEvent );

                // Create sync event
                //-------------------------------------------------------------------------

                if ( pEventTrack->IsSyncTrack() && numSyncTracks == 1 )
                {
                    outEventData.m_syncEventMarkers.emplace_back( SyncTrack::EventMarker( Percentage( pEvent->GetStartTime() / rawAnimData.GetDuration() ), pEvent->GetSyncEventID() ) );
                }
            }
        }

        if ( numSyncTracks > 1 )
        {
            Warning( "Multiple sync tracks detected, using the first one encountered!" );
        }

        // Transfer sorted events
        //-------------------------------------------------------------------------

        auto sortPredicate = [] ( Event* const& pEventA, Event* const& pEventB )
        {
            return pEventA->GetStartTime() < pEventB->GetStartTime();
        };

        eastl::sort( events.begin(), events.end(), sortPredicate );

        for ( auto const& pEvent : events )
        {
            outEventData.m_collection.m_descriptors.emplace_back( TypeSystem::TypeDescriptor( *m_pTypeRegistry, pEvent ) );
        }

        eastl::sort( outEventData.m_syncEventMarkers.begin(), outEventData.m_syncEventMarkers.end() );

        // Free allocated memory
        //-------------------------------------------------------------------------

        trackContainer.Reset();

        return true;
    }

    bool AnimationClipCompiler::GetReferencedResources( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        // TODO: check events for referenced resources
        return true;
    }
}
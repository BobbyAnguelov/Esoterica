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
#include "System/Math/MathUtils.h"
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
        // Read descriptor
        rapidjson::Document descriptorDocument;
        AnimationClipResourceDescriptor resourceDescriptor;
        if ( !TryLoadResourceDescriptor( ctx.m_inputFilePath, resourceDescriptor, &descriptorDocument ) )
        {
            return Resource::CompilationResult::Failure;
        }

        // Read animation data
        TUniquePtr<RawAssets::RawAnimation> pRawAnimation = nullptr;
        if ( !ReadRawAnimation( ctx, resourceDescriptor, pRawAnimation ) )
        {
            return Resource::CompilationResult::Failure;
        }

        // Validate frame limit
        if ( resourceDescriptor.m_limitFrameRange.IsSet() )
        {
            if ( pRawAnimation->GetNumFrames() == 1 )
            {
                return Error( "Having a frame limit range on a single frame animation doesnt make sense!" );
            }

            resourceDescriptor.m_limitFrameRange.m_begin = Math::Clamp( resourceDescriptor.m_limitFrameRange.m_begin, 0, (int32_t) pRawAnimation->GetNumFrames() );
            resourceDescriptor.m_limitFrameRange.m_end = Math::Clamp( resourceDescriptor.m_limitFrameRange.m_end + 1, 0, (int32_t) pRawAnimation->GetNumFrames() ); // Inclusive range so we need to increment the index

            if ( !resourceDescriptor.m_limitFrameRange.IsValid() )
            {
                return Error( "Invalid frame limit range set!" );
            }

            if ( resourceDescriptor.m_limitFrameRange.GetLength() == 0 )
            {
                return Error( "Zero frame limit range set!" );
            }
        }

        // Auto-generate root motion
        if ( resourceDescriptor.m_regenerateRootMotion )
        {
            if ( !RegenerateRootMotion( resourceDescriptor, pRawAnimation.get() ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }

        // Additive generation
        if ( resourceDescriptor.m_additiveType != AnimationClipResourceDescriptor::AdditiveType::None )
        {
            if ( !MakeAdditive( ctx, resourceDescriptor, *pRawAnimation ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }

        // Reflect raw animation data into runtime format
        //-------------------------------------------------------------------------

        AnimationClip animData;
        animData.m_skeleton = resourceDescriptor.m_skeleton;
        TransferAndCompressAnimationData( *pRawAnimation, animData, resourceDescriptor.m_limitFrameRange );

        // Handle events
        //-------------------------------------------------------------------------

        AnimationClipEventData eventData;
        if ( !ReadEventsData( ctx, descriptorDocument, *pRawAnimation, eventData ) )
        {
            return CompilationFailed( ctx );
        }

        // Serialize animation data
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, AnimationClip::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        hdr.AddInstallDependency( resourceDescriptor.m_skeleton.GetResourceID() );

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

    bool AnimationClipCompiler::ReadRawAnimation( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, TUniquePtr<RawAssets::RawAnimation>& pOutAnimation ) const
    {
        EE_ASSERT( pOutAnimation == nullptr );

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        SkeletonResourceDescriptor skeletonResourceDescriptor;
        if ( !TryLoadResourceDescriptor( resourceDescriptor.m_skeleton.GetResourcePath(), skeletonResourceDescriptor ) )
        {
            Error( "Failed to load skeleton descriptor!" );
            return false;
        }

        FileSystem::Path skeletonFilePath;
        if ( !ConvertResourcePathToFilePath( skeletonResourceDescriptor.m_skeletonPath, skeletonFilePath ) )
        {
            Error( "Invalid skeleton FBX data path: %s", skeletonResourceDescriptor.m_skeletonPath.GetString().c_str() );
            return false;
        }

        RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        auto pRawSkeleton = RawAssets::ReadSkeleton( readerCtx, skeletonFilePath, skeletonResourceDescriptor.m_skeletonRootBoneName );
        if ( pRawSkeleton == nullptr || !pRawSkeleton->IsValid() )
        {
            Error( "Failed to read skeleton file: %s", skeletonFilePath.ToString().c_str() );
            return false;
        }

        // Read animation data
        //-------------------------------------------------------------------------

        FileSystem::Path animationFilePath;
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_animationPath, animationFilePath ) )
        {
            Error( "Invalid animation data path: %s", resourceDescriptor.m_animationPath.c_str() );
            return false;
        }

        if ( !FileSystem::Exists( animationFilePath ) )
        {
            Error( "Invalid animation file path: %s", animationFilePath.ToString().c_str() );
            return false;
        }

        pOutAnimation = RawAssets::ReadAnimation( readerCtx, animationFilePath, *pRawSkeleton, resourceDescriptor.m_animationName );
        if ( pOutAnimation == nullptr )
        {
            Error( "Failed to read animation from source file" );
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool AnimationClipCompiler::MakeAdditive( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, RawAssets::RawAnimation& rawAnimData ) const
    {
        EE_ASSERT( resourceDescriptor.m_additiveType != AnimationClipResourceDescriptor::AdditiveType::None );

        if ( resourceDescriptor.m_additiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToSkeleton )
        {
            rawAnimData.MakeAdditiveRelativeToSkeleton();
        }
        else if ( resourceDescriptor.m_additiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame )
        {
            int32_t const baseFrameIdx = Math::Clamp( (int32_t) resourceDescriptor.m_additiveBaseFrameIndex, 0, rawAnimData.GetNumFrames() - 1 );
            rawAnimData.MakeAdditiveRelativeToFrame( baseFrameIdx );
        }
        else if ( resourceDescriptor.m_additiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip )
        {
            AnimationClipResourceDescriptor baseAnimResourceDescriptor;
            if ( !TryLoadResourceDescriptor( resourceDescriptor.m_additiveBaseAnimation.GetResourcePath(), baseAnimResourceDescriptor ) )
            {
                Error( "Failed to load base animation descriptor!" );
                return false;
            }

            if ( baseAnimResourceDescriptor.m_skeleton != resourceDescriptor.m_skeleton )
            {
                Error( "Base additive animation skeleton does not match animation! Expected: %s, instead got: %s", resourceDescriptor.m_skeleton.GetResourcePath().c_str(), baseAnimResourceDescriptor.m_skeleton.GetResourcePath().c_str() );
                return false;
            }

            TUniquePtr<RawAssets::RawAnimation> pBaseRawAnimation = nullptr;
            if ( !ReadRawAnimation( ctx, baseAnimResourceDescriptor, pBaseRawAnimation ) )
            {
                Error( "Failed to load base animation data!" );
                return false;
            }

            rawAnimData.MakeAdditiveRelativeToAnimation( *pBaseRawAnimation.get() );
        }

        //-------------------------------------------------------------------------

        return true;
    }

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
        float const flatZ = rootMotion.front().GetTranslation().GetZ();
        float const offsetZ = ( rootMotion.front().GetTranslation() - sourceBoneTrackData.m_globalTransforms[0].GetTranslation() ).GetZ();

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
                translation.SetZ( flatZ );
            }
            else
            {
                translation += Vector( 0, 0, offsetZ, 0 );
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

    void AnimationClipCompiler::TransferAndCompressAnimationData( RawAssets::RawAnimation const& rawAnimData, AnimationClip& animClip, IntRange const& limitRange ) const
    {
        auto const& rawTrackData = rawAnimData.GetTrackData();
        uint32_t const numBones = rawAnimData.GetNumBones();
        int32_t const numOriginalFrames = rawAnimData.GetNumFrames();

        // Calculate frame limits
        //-------------------------------------------------------------------------

        int32_t frameIdxStart = 0;
        int32_t frameIdxEnd = numOriginalFrames;

        if ( limitRange.IsSetAndValid() )
        {
            if( limitRange.m_end >= numOriginalFrames )
            {
                Warning( "Frame range limit exceed available frames, clamping to end of animation!" );
            }

            frameIdxStart = Math::Max( limitRange.m_begin, 0 );
            frameIdxEnd = Math::Min( limitRange.m_end, numOriginalFrames );
        }

        animClip.m_numFrames = frameIdxEnd - frameIdxStart;

        // Transfer basic animation data
        //-------------------------------------------------------------------------

        float const FPS = ( numOriginalFrames - 1 ) / rawAnimData.GetDuration().ToFloat();
        animClip.m_duration = ( animClip.IsSingleFrameAnimation() ) ? 0.0f : Seconds( ( animClip.m_numFrames - 1 ) / FPS );
        animClip.m_isAdditive = rawAnimData.IsAdditive();

        // Transfer root motion
        //-------------------------------------------------------------------------

        if ( limitRange.IsSetAndValid() )
        {
            animClip.m_rootMotion.m_transforms.clear();
            animClip.m_rootMotion.m_transforms.insert( animClip.m_rootMotion.m_transforms.end(), rawAnimData.GetRootMotion().begin() + frameIdxStart, rawAnimData.GetRootMotion().begin() + frameIdxEnd );
        }
        else
        {
            animClip.m_rootMotion.m_transforms = rawAnimData.GetRootMotion();
        }

        // Calculate root motion extra data
        //-------------------------------------------------------------------------

        float totalDistance = 0.0f;
        float totalRotation = 0.0f;

        for ( uint32_t i = 1u; i < animClip.m_numFrames; i++ )
        {
            // Track deltas
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

            for ( int32_t frameIdx = frameIdxStart; frameIdx < frameIdxEnd; frameIdx++ )
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

                trackSettings.m_translationRangeX = { translation.GetX(), defaultQuantizationRangeLength };
                trackSettings.m_translationRangeY = { translation.GetY(), defaultQuantizationRangeLength };
                trackSettings.m_translationRangeZ = { translation.GetZ(), defaultQuantizationRangeLength };
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

                uint16_t const m_x = Quantization::EncodeFloat( translation.GetX(), trackSettings.m_translationRangeX.m_rangeStart, trackSettings.m_translationRangeX.m_rangeLength );
                uint16_t const m_y = Quantization::EncodeFloat( translation.GetY(), trackSettings.m_translationRangeY.m_rangeStart, trackSettings.m_translationRangeY.m_rangeLength );
                uint16_t const m_z = Quantization::EncodeFloat( translation.GetZ(), trackSettings.m_translationRangeZ.m_rangeStart, trackSettings.m_translationRangeZ.m_rangeLength );

                animClip.m_compressedPoseData.push_back( m_x );
                animClip.m_compressedPoseData.push_back( m_y );
                animClip.m_compressedPoseData.push_back( m_z );
            }
            else // Store frames
            {
                for ( int32_t frameIdx = frameIdxStart; frameIdx < frameIdxEnd; frameIdx++ )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[frameIdx];
                    Vector const& translation = rawBoneTransform.GetTranslation();

                    uint16_t const m_x = Quantization::EncodeFloat( translation.GetX(), trackSettings.m_translationRangeX.m_rangeStart, trackSettings.m_translationRangeX.m_rangeLength );
                    uint16_t const m_y = Quantization::EncodeFloat( translation.GetY(), trackSettings.m_translationRangeY.m_rangeStart, trackSettings.m_translationRangeY.m_rangeLength );
                    uint16_t const m_z = Quantization::EncodeFloat( translation.GetZ(), trackSettings.m_translationRangeZ.m_rangeStart, trackSettings.m_translationRangeZ.m_rangeLength );

                    animClip.m_compressedPoseData.push_back( m_x );
                    animClip.m_compressedPoseData.push_back( m_y );
                    animClip.m_compressedPoseData.push_back( m_z );
                }
            }

            //-------------------------------------------------------------------------
            // Scale
            //-------------------------------------------------------------------------

            FloatRange const& rawScaleValueRange = rawTrackData[boneIdx].m_scaleValueRange;
            float const rawScaleValueRangeLengthX = rawScaleValueRange.GetLength();

            // We could arguably compress more by saving each component individually at the cost of sampling performance. If we absolutely need more compression, we can do it here
            bool const isScaleConstant = Math::IsNearZero( rawScaleValueRangeLengthX );
            if ( isScaleConstant )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[0];
                trackSettings.m_scaleRange = { rawBoneTransform.GetScale(), defaultQuantizationRangeLength };
                trackSettings.m_isScaleStatic = true;
            }
            else
            {
                trackSettings.m_scaleRange = { rawScaleValueRange.m_begin, Math::IsNearZero( rawScaleValueRangeLengthX ) ? defaultQuantizationRangeLength : rawScaleValueRangeLengthX };
            }

            //-------------------------------------------------------------------------

            if ( trackSettings.IsScaleTrackStatic() )
            {
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[0];
                uint16_t const m_x = Quantization::EncodeFloat( rawBoneTransform.GetScale(), trackSettings.m_scaleRange.m_rangeStart, trackSettings.m_scaleRange.m_rangeLength );
                animClip.m_compressedPoseData.push_back( m_x );
            }
            else // Store frames
            {
                for ( int32_t frameIdx = frameIdxStart; frameIdx < frameIdxEnd; frameIdx++ )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[frameIdx];
                    uint16_t const m_x = Quantization::EncodeFloat( rawBoneTransform.GetScale(), trackSettings.m_scaleRange.m_rangeStart, trackSettings.m_scaleRange.m_rangeLength );
                    animClip.m_compressedPoseData.push_back( m_x );
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
        FloatRange const animationTimeRange( 0, numIntervals ); // In Frames
        for ( Timeline::Track* pTrack : trackContainer.m_tracks )
        {
            auto trackStatus = pTrack->GetValidationStatus( numIntervals );

            //-------------------------------------------------------------------------

            if ( trackStatus == Timeline::Track::Status::HasErrors )
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

            if ( trackStatus == Timeline::Track::Status::HasWarnings )
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

                if ( !pEvent->IsValid() )
                {
                    Warning( "Animation event track (%s) has warnings: %s", pTrack->GetTypeName(), pTrack->GetStatusMessage().c_str() );
                    continue;
                }

                // Add event
                //-------------------------------------------------------------------------

                // Ignore any event entirely outside the animation time range
                FloatRange eventTimeRange = pItem->GetTimeRange();
                if ( !animationTimeRange.Overlaps( eventTimeRange ) )
                {
                    Warning( "Event detected outside animation time range, event will be ignored" );
                    continue;
                }

                // Clamp events that extend out of the animation time range to the animation time range
                if ( !animationTimeRange.ContainsInclusive( eventTimeRange ) )
                {
                    Warning( "Event extend outside the valid animation time range, event will be clamped to animation range" );

                    eventTimeRange.m_begin = animationTimeRange.GetClampedValue( eventTimeRange.m_begin );
                    eventTimeRange.m_end = animationTimeRange.GetClampedValue( eventTimeRange.m_end );
                    EE_ASSERT( eventTimeRange.IsSetAndValid() );
                }

                // Set event time (in Seconds) and add new event
                pEvent->m_startTime = ( eventTimeRange.m_begin / numIntervals ) * rawAnimData.GetDuration();
                pEvent->m_duration = ( eventTimeRange.GetLength() / numIntervals ) * rawAnimData.GetDuration();
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

    bool AnimationClipCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        // Try read event tracks
        //-------------------------------------------------------------------------

        FileSystem::Path const descriptorFilePath = resourceID.GetResourcePath().ToFileSystemPath( m_rawResourceDirectoryPath );

        Serialization::TypeArchiveReader typeReader( *m_pTypeRegistry );
        if ( !typeReader.ReadFromFile( descriptorFilePath ) )
        {
            Error( "Failed to read resource descriptor file: %s", descriptorFilePath.c_str() );
            return false;
        }

        auto& document = typeReader.GetDocument();
        auto trackDataIter = document.FindMember( Timeline::TrackContainer::s_trackContainerKey );

        // If we have no events, then nothing is referenced
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

        //-------------------------------------------------------------------------

        for ( Timeline::Track* pTrack : trackContainer.m_tracks )
        {
            for ( auto const pItem : pTrack->GetItems() )
            {
                IReflectedType* pEventInstance = pItem->GetData();
                pEventInstance->GetTypeInfo()->GetReferencedResources( pEventInstance, outReferencedResources );
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }
}
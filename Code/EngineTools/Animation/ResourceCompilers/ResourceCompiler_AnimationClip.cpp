#include "ResourceCompiler_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Import/Importer.h"
#include "EngineTools/Import/ImportedAnimation.h"
#include "EngineTools/Timeline/Timeline.h"
#include "Engine/Animation/AnimationSyncTrack.h"
#include "Engine/Animation/AnimationClip.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Math/MathUtils.h"
#include "Base/Time/Timers.h"
#include "Base/TypeSystem/TypeDescriptors.h"
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
        : Resource::Compiler( "AnimationCompiler" )
    {
        AddOutputType<AnimationClip>();
    }

    Resource::CompilationResult AnimationClipCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        Resource::CompilationResult result = Resource::CompilationResult::Success;

        //-------------------------------------------------------------------------

        Milliseconds timeTaken = 0;

        // Read descriptor
        //-------------------------------------------------------------------------

        AnimationClipResourceDescriptor resourceDescriptor;
        if ( !TryLoadResourceDescriptor( ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Resource::CompilationResult::Failure;
        }

        int32_t const numSecondaryAnims = (int32_t) resourceDescriptor.m_secondaryAnimations.size();

        // Read animation data
        //-------------------------------------------------------------------------

        TUniquePtr<Import::ImportedAnimation> ImportedAnimationPtr = nullptr;
        TVector<TUniquePtr<Import::ImportedAnimation>> secondaryAnimations;
        result = CombineResultCode( result, ReadImportedAnimation( resourceDescriptor.m_skeleton.GetDataPath(), resourceDescriptor.m_animationPath, ImportedAnimationPtr ) );
        if ( result == Resource::CompilationResult::Failure )
        {
            return Error( "Failed to read raw animation data!" );
        }

        for ( int32_t i = 0; i < numSecondaryAnims; i++ )
        {
            // Ensure that the secondary skeleton is different from the parent skeleton
            if ( resourceDescriptor.m_secondaryAnimations[i].m_skeleton == resourceDescriptor.m_skeleton )
            {
                return Error( "Secondary animations are not allowed to be on the same skeleton as the parent" );
            }

            // Ensure that all secondary skeletons are unique
            for ( int32_t j = i - 1; j >= 0; j-- )
            {
                if ( resourceDescriptor.m_secondaryAnimations[i].m_skeleton == resourceDescriptor.m_secondaryAnimations[j].m_skeleton )
                {
                    return Error( "More than one secondary animation with the same skeleton, this is not allowed!" );
                }
            }

            // Read raw animation data
            result = CombineResultCode( result, ReadImportedAnimation( resourceDescriptor.m_secondaryAnimations[i].m_skeleton.GetDataPath(), resourceDescriptor.m_secondaryAnimations[i].m_animationPath, secondaryAnimations.emplace_back() ) );
            if ( result == Resource::CompilationResult::Failure )
            {
                return Error( "Failed to read secondary animation data!" );
            }
        }

        // Validate frame limit
        //-------------------------------------------------------------------------

        if ( resourceDescriptor.m_limitFrameRange.IsSet() )
        {
            if ( ImportedAnimationPtr->GetNumFrames() == 1 )
            {
                return Error( "Having a frame limit range on a single frame animation doesnt make sense!" );
            }

            resourceDescriptor.m_limitFrameRange.m_begin = Math::Clamp( resourceDescriptor.m_limitFrameRange.m_begin, 0, (int32_t) ImportedAnimationPtr->GetNumFrames() - 1 );
            resourceDescriptor.m_limitFrameRange.m_end = Math::Clamp( resourceDescriptor.m_limitFrameRange.m_end, 0, (int32_t) ImportedAnimationPtr->GetNumFrames() - 1 ); // Inclusive range so we need to increment the index

            if ( !resourceDescriptor.m_limitFrameRange.IsValid() )
            {
                return Error( "Invalid frame limit range set!" );
            }
        }

        // Auto-generate root motion
        //-------------------------------------------------------------------------

        if ( resourceDescriptor.m_regenerateRootMotion )
        {
            {
                ScopedTimer<PlatformClock> timer( timeTaken );
                result = CombineResultCode( result, RegenerateRootMotion( resourceDescriptor, ImportedAnimationPtr.get() ) );
                if ( result == Resource::CompilationResult::Failure )
                {
                    return Error( "Failed to generate root motion data!" );
                }
            }
            Message( "Root Motion Generation: %.3fms", timeTaken.ToFloat() );
        }

        // Additive generation
        //-------------------------------------------------------------------------

        if ( resourceDescriptor.m_additiveType != AnimationClipResourceDescriptor::AdditiveType::None )
        {
            {
                ScopedTimer<PlatformClock> timer( timeTaken );
                result = CombineResultCode( result, MakeAdditive( ctx, resourceDescriptor, *ImportedAnimationPtr, false ) );
                if ( result == Resource::CompilationResult::Failure )
                {
                    return Error( "Failed to generate additive animation data!" );
                }

                for ( int32_t i = 0; i < numSecondaryAnims; i++ )
                {
                    result = CombineResultCode( result, MakeAdditive( ctx, resourceDescriptor, *secondaryAnimations[i], true ) );
                    if ( result == Resource::CompilationResult::Failure )
                    {
                        return Error( "Failed to generate additive animation data!" );
                    }
                }
            }

            Message( "Additive Generation: %.3fms", timeTaken.ToFloat() );
        }

        // Compress raw animation data into runtime format
        //-------------------------------------------------------------------------

        AnimationClip animClip;
        TVector<AnimationClip> secondaryAnimData;

        {
            ScopedTimer<PlatformClock> timer( timeTaken );
            animClip.m_skeleton = resourceDescriptor.m_skeleton;
            result = CombineResultCode( result, TransferAndCompressAnimationData( *ImportedAnimationPtr, animClip, resourceDescriptor.m_limitFrameRange, false ) );
            if ( result == Resource::CompilationResult::Failure )
            {
                return Error( "Failed to compress animation!" );
            }

            IntRange const parentFrameRange( 0, animClip.GetNumFrames() - 1 ); // Inclusive frame index range
            for ( int32_t i = 0; i < numSecondaryAnims; i++ )
            {
                secondaryAnimData.emplace_back();
                secondaryAnimData[i].m_skeleton = resourceDescriptor.m_secondaryAnimations[i].m_skeleton;
                result = CombineResultCode( result, TransferAndCompressAnimationData( *secondaryAnimations[i], secondaryAnimData[i], parentFrameRange, true ) );
                if ( result == Resource::CompilationResult::Failure )
                {
                    return Error( "Failed to compress secondary animation!" );
                }
            }
        }
        Message( "Compression: %.3fms", timeTaken.ToFloat() );

        // Handle events
        //-------------------------------------------------------------------------

        AnimationClipEventData eventData;
        {
            ScopedTimer<PlatformClock> timer( timeTaken );
            result = CombineResultCode( result, ProcessEventsData( ctx, resourceDescriptor, *ImportedAnimationPtr, eventData ) );
            if ( result == Resource::CompilationResult::Failure )
            {
                return Error( "Failed to read animation events!" );
            }
        }
        Message( "Read Animation Events: %.3fms", timeTaken.ToFloat() );

        // Duration override
        //-------------------------------------------------------------------------

        if ( !animClip.IsSingleFrameAnimation() && resourceDescriptor.m_durationOverride != AnimationClipResourceDescriptor::DurationOverride::None )
        {
            float durationMultiplier = resourceDescriptor.m_durationOverrideValue;

            if ( resourceDescriptor.m_durationOverride == AnimationClipResourceDescriptor::DurationOverride::FixedValue )
            {
                EE_ASSERT( animClip.GetDuration() != 0.0f );
                durationMultiplier = resourceDescriptor.m_durationOverrideValue / animClip.GetDuration();
            }

            if ( durationMultiplier <= 0.0f )
            {
                return Error( "Invalid duration override value set!" );
            }

            //-------------------------------------------------------------------------

            animClip.m_duration *= durationMultiplier;

            for ( AnimationClip& secondaryClip : secondaryAnimData )
            {
                secondaryClip.m_duration *= durationMultiplier;
            }
        }

        // Serialize animation data
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( AnimationClip::s_version, AnimationClip::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );
        hdr.AddInstallDependency( resourceDescriptor.m_skeleton.GetResourceID() );

        // Add secondary animation skeletons as install dependencies
        for ( int32_t i = 0; i < numSecondaryAnims; i++ )
        {
            hdr.AddInstallDependency( resourceDescriptor.m_secondaryAnimations[i].m_skeleton.GetResourceID() );
        }

        Serialization::BinaryOutputArchive archive;
        archive << hdr << animClip;
        archive << eventData.m_syncEventMarkers;
        archive << eventData.m_collection;

        archive << numSecondaryAnims;
        for ( int32_t i = 0; i < numSecondaryAnims; i++ )
        {
            archive << secondaryAnimData[i];
        }

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            if ( ImportedAnimationPtr->HasWarnings() || result == Resource::CompilationResult::SuccessWithWarnings )
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

    Resource::CompilationResult AnimationClipCompiler::ReadImportedAnimation( DataPath const& skeletonPath, DataPath const& animationPath, TUniquePtr<Import::ImportedAnimation>& outAnimation, String const& animationName ) const
    {
        Timer<PlatformClock> timer;
        EE_ASSERT( outAnimation == nullptr );

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        timer.Start();

        SkeletonResourceDescriptor skeletonResourceDescriptor;
        if ( !TryLoadResourceDescriptor( skeletonPath, skeletonResourceDescriptor ) )
        {
            return Error( "Failed to load skeleton descriptor!" );
        }

        FileSystem::Path skeletonFilePath;
        if ( !ConvertDataPathToFilePath( skeletonResourceDescriptor.m_skeletonPath, skeletonFilePath ) )
        {
            return Error( "Invalid skeleton FBX data path: %s", skeletonResourceDescriptor.m_skeletonPath.GetString().c_str() );
        }

        Import::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        auto pImportedSkeleton = Import::ReadSkeleton( readerCtx, skeletonFilePath, skeletonResourceDescriptor.m_skeletonRootBoneName, skeletonResourceDescriptor.m_highLODBones );
        if ( pImportedSkeleton == nullptr || !pImportedSkeleton->IsValid() )
        {
            return Error( "Failed to read skeleton file: %s", skeletonFilePath.ToString().c_str() );
        }

        Message( "Read Source Skeleton Data: %.3fms", timer.GetElapsedTimeMilliseconds().ToFloat() );

        // Read animation data
        //-------------------------------------------------------------------------

        timer.Start();

        FileSystem::Path animationFilePath;
        if ( !ConvertDataPathToFilePath( animationPath, animationFilePath ) )
        {
            return Error( "Invalid animation data path: %s", animationPath.c_str() );
        }

        if ( !FileSystem::Exists( animationFilePath ) )
        {
            return Error( "Invalid animation file path: %s", animationFilePath.ToString().c_str() );
        }

        outAnimation = Import::ReadAnimation( readerCtx, animationFilePath, *pImportedSkeleton, animationName );
        if ( outAnimation == nullptr )
        {
            return Error( "Failed to read animation from source file" );
        }

        Message( "Read Source Animation Data: %.3fms", timer.GetElapsedTimeMilliseconds().ToFloat() );

        //-------------------------------------------------------------------------

        return Resource::CompilationResult::Success;
    }

    Resource::CompilationResult AnimationClipCompiler::MakeAdditive( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::ImportedAnimation& rawAnimData, bool isSecondaryAnimation ) const
    {
        EE_ASSERT( resourceDescriptor.m_additiveType != AnimationClipResourceDescriptor::AdditiveType::None );

        //-------------------------------------------------------------------------

        AnimationClipResourceDescriptor::AdditiveType actualAdditiveType = resourceDescriptor.m_additiveType;
        int32_t actualBaseFrameIdx = (int32_t) resourceDescriptor.m_additiveBaseFrameIndex;

        // Handle secondary animations separately
        if ( isSecondaryAnimation && actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip )
        {
            Warning( "Cant make a secondary animation additive based on another animation clip. Additive data will be relative to the first frame pose" );
            actualAdditiveType = AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame;
            actualBaseFrameIdx = 0;
        }

        //-------------------------------------------------------------------------

        if ( actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToSkeleton )
        {
            rawAnimData.MakeAdditiveRelativeToSkeleton();
        }
        else if ( actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame )
        {
            int32_t const baseFrameIdx = Math::Clamp( actualBaseFrameIdx, 0, rawAnimData.GetNumFrames() - 1 );
            rawAnimData.MakeAdditiveRelativeToFrame( baseFrameIdx );
        }
        else if ( actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip )
        {
            EE_ASSERT( !isSecondaryAnimation );

            if( !resourceDescriptor.m_additiveBaseAnimation.GetDataPath().IsValid() )
            {
                return Error( "Additive (RelativeToClip): No base animation clip set!" );
            }

            AnimationClipResourceDescriptor baseAnimResourceDescriptor;
            if ( !TryLoadResourceDescriptor( resourceDescriptor.m_additiveBaseAnimation.GetDataPath(), baseAnimResourceDescriptor ) )
            {
                return Error( "Additive (RelativeToClip): Failed to load base animation descriptor!" );
            }

            if ( baseAnimResourceDescriptor.m_skeleton != resourceDescriptor.m_skeleton )
            {
                return Error( "Additive (RelativeToClip): Base additive animation skeleton does not match animation! Expected: %s, instead got: %s", resourceDescriptor.m_skeleton.GetDataPath().c_str(), baseAnimResourceDescriptor.m_skeleton.GetDataPath().c_str() );
            }

            TUniquePtr<Import::ImportedAnimation> pBaseImportedAnimation = nullptr;
            if ( ReadImportedAnimation( baseAnimResourceDescriptor.m_skeleton.GetDataPath(), baseAnimResourceDescriptor.m_animationPath, pBaseImportedAnimation ) == Resource::CompilationResult::Failure )
            {
                return Error( "Additive (RelativeToClip): Failed to load base animation data!" );
            }

            rawAnimData.MakeAdditiveRelativeToAnimation( *pBaseImportedAnimation.get() );
        }

        //-------------------------------------------------------------------------

        return Resource::CompilationResult::Success;
    }

    Resource::CompilationResult AnimationClipCompiler::RegenerateRootMotion( AnimationClipResourceDescriptor const& resourceDescriptor, Import::ImportedAnimation* pImportedAnimation ) const
    {
        EE_ASSERT( pImportedAnimation != nullptr && pImportedAnimation->IsValid() );

        // Validate Root Motion Generation settings
        //-------------------------------------------------------------------------

        int32_t rootMotionGenerationBoneIdx = InvalidIndex;
        if ( resourceDescriptor.m_regenerateRootMotion )
        {
            if ( resourceDescriptor.m_rootMotionGenerationBoneID.IsValid() )
            {
                rootMotionGenerationBoneIdx = pImportedAnimation->GetSkeleton().GetBoneIndex( resourceDescriptor.m_rootMotionGenerationBoneID );
                if ( rootMotionGenerationBoneIdx == InvalidIndex )
                {
                    return Error( "Root Motion Generation: Skeleton doesnt contain specified bone: %s", resourceDescriptor.m_rootMotionGenerationBoneID.c_str() );
                }
            }
            else
            {
                return Error( "Root Motion Generation: No bone specified for root motion generation!" );
            }
        }

        // Calculate root position based on specified bone
        //-------------------------------------------------------------------------

        auto const& sourceBoneTrackData = pImportedAnimation->GetTrackData()[rootMotionGenerationBoneIdx];
        auto& rootTrackData = pImportedAnimation->GetTrackData()[0];
        Quaternion const preRotation( resourceDescriptor.m_rootMotionGenerationPreRotation );

        // Get the Z value for the root on the first frame, use this as the starting point for the root motion track
        auto& rootMotion = pImportedAnimation->GetRootMotion();
        float const flatZ = rootMotion.front().GetTranslation().GetZ();
        float const offsetZ = ( rootMotion.front().GetTranslation() - sourceBoneTrackData.m_modelSpaceTransforms[0].GetTranslation() ).GetZ();

        int32_t const numFrames = pImportedAnimation->GetNumFrames();
        for ( auto i = 0; i < numFrames; i++ )
        {
            Transform const& rootMotionTransform = sourceBoneTrackData.m_modelSpaceTransforms[i];

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
            rootTrackData.m_modelSpaceTransforms[i] = Transform( orientation, translation );
        }

        // Regenerate the local transforms taking into account the new root position
        pImportedAnimation->RegenerateLocalTransforms();
        pImportedAnimation->Finalize();

        //-------------------------------------------------------------------------

        return Resource::CompilationResult::Success;
    }

    Resource::CompilationResult AnimationClipCompiler::TransferAndCompressAnimationData( Import::ImportedAnimation const& rawAnimData, AnimationClip& animClip, IntRange const& limitRange, bool isSecondaryAnimation ) const
    {
        Resource::CompilationResult result = Resource::CompilationResult::Success;
        auto const& rawTrackData = rawAnimData.GetTrackData();
        uint32_t const numBones = rawAnimData.GetNumBones();
        int32_t const numOriginalFrames = rawAnimData.GetNumFrames();

        // Calculate frame limits
        //-------------------------------------------------------------------------
        // The limit range is an inclusive range that specifies the exact start and frame index to include

        int32_t frameIdxStart = 0;
        int32_t frameIdxEnd = numOriginalFrames;

        if ( limitRange.IsSetAndValid() )
        {
            if( limitRange.m_end >= numOriginalFrames )
            {
                if ( isSecondaryAnimation )
                {
                    result = Warning( "Secondary animation is shorter than the parent animations, last frame in the secondary animation will be repeated!" );
                }
                else
                {
                    result = Warning( "Frame range limit exceed available frames, clamping to end of animation!" );
                }
            }

            frameIdxStart = Math::Max( limitRange.m_begin, 0 );
            frameIdxEnd = isSecondaryAnimation ? limitRange.m_end + 1 : Math::Min( limitRange.m_end + 1, numOriginalFrames );
        }

        animClip.m_numFrames = ( frameIdxEnd == frameIdxStart ) ? 1 : frameIdxEnd - frameIdxStart;

        // Transfer basic animation data
        //-------------------------------------------------------------------------

        float const FPS = ( numOriginalFrames - 1 ) / rawAnimData.GetDuration().ToFloat();
        animClip.m_duration = ( animClip.IsSingleFrameAnimation() ) ? Seconds( 0.0f ) : Seconds( ( animClip.m_numFrames - 1 ) / FPS );
        animClip.m_isAdditive = rawAnimData.IsAdditive();

        // Transfer root motion (and duplicate frames if needed)
        //-------------------------------------------------------------------------

        RootMotionData& rootMotionData = animClip.m_rootMotion;
        rootMotionData.m_numFrames = animClip.m_numFrames;

        bool const shouldRepeatLastFrame = isSecondaryAnimation && frameIdxEnd >= numOriginalFrames;

        for ( int32_t frameIdx = frameIdxStart; frameIdx < frameIdxEnd; frameIdx++ )
        {
            int32_t actualFrameIdx = frameIdx;

            // Repeat last frame for secondary animations that are shorter than their parents
            if ( shouldRepeatLastFrame && frameIdx >= numOriginalFrames )
            {
                actualFrameIdx = numOriginalFrames - 1;
            }

            animClip.m_rootMotion.m_transforms.emplace_back( rawAnimData.GetRootMotion()[actualFrameIdx] );
        }

        // Calculate root motion extra data
        //-------------------------------------------------------------------------

        if ( animClip.IsSingleFrameAnimation() )
        {
            rootMotionData.m_transforms.clear();
            rootMotionData.m_totalDelta = Transform::Identity;
            rootMotionData.m_averageLinearVelocity = 0.0f;
            rootMotionData.m_averageAngularVelocity = 0.0f;
        }
        else // Calculate extra info
        {
            float totalDistance = 0.0f;
            float totalRotation = 0.0f;
            bool containsRootMotion = false;

            for ( int32_t i = 1; i < animClip.m_numFrames; i++ )
            {
                // Track deltas
                Transform const deltaRoot = Transform::DeltaNoScale( rootMotionData.m_transforms[i - 1], rootMotionData.m_transforms[i] );
                totalDistance += deltaRoot.GetTranslation().GetLength3();

                // If we have a rotation delta, accumulate the yaw value
                if ( !deltaRoot.GetRotation().IsIdentity() )
                {
                    Vector const deltaForward2D = deltaRoot.GetForwardVector().GetNormalized2();
                    Radians const deltaAngle = Math::GetYawAngleBetweenVectors( deltaForward2D, Vector::WorldBackward ).GetClamped360();
                    totalRotation += Math::Abs( (float) deltaAngle );
                }

                // Check for equality - if two transforms differ then we have root motion present
                if ( !rootMotionData.m_transforms[i].IsNearEqual( rootMotionData.m_transforms[i - 1] ) )
                {
                    containsRootMotion = true;
                }
            }

            if ( containsRootMotion )
            {
                rootMotionData.m_totalDelta = Transform::DeltaNoScale( rootMotionData.m_transforms[0], rootMotionData.m_transforms.back() );
                rootMotionData.m_averageLinearVelocity = totalDistance / animClip.GetDuration();
                rootMotionData.m_averageAngularVelocity = totalRotation / animClip.GetDuration();
            }
            else // No root motion, so clear all the identical transforms
            {
                rootMotionData.m_transforms.clear();
                rootMotionData.m_totalDelta = Transform::Identity;
                rootMotionData.m_averageLinearVelocity = 0;
                rootMotionData.m_averageAngularVelocity = 0;
            }
        }

        //-------------------------------------------------------------------------
        // Calculate track data ranges
        //-------------------------------------------------------------------------

        struct TrackRangeData
        {
            FloatRange                         m_translationValueRangeX;
            FloatRange                         m_translationValueRangeY;
            FloatRange                         m_translationValueRangeZ;
            FloatRange                         m_scaleValueRange;
            bool                               m_isRotationConstant = false;
        };

        TVector<TrackRangeData> trackRanges;
        trackRanges.resize( numBones );

        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            // Initialize range data
            TrackRangeData& trackRangeData = trackRanges[boneIdx];
            trackRangeData.m_translationValueRangeX = FloatRange();
            trackRangeData.m_translationValueRangeY = FloatRange();
            trackRangeData.m_translationValueRangeZ = FloatRange();
            trackRangeData.m_scaleValueRange = FloatRange();
            trackRangeData.m_isRotationConstant = true;

            // Calculate ranges for the frame range that we are compiling
            Import::ImportedAnimation::TrackData const& trackTransformData = rawTrackData[boneIdx];
            Quaternion previousRotation = trackTransformData.m_localTransforms[frameIdxStart].GetRotation();
            for ( int32_t frameIdx = frameIdxStart; frameIdx < frameIdxEnd; frameIdx++ )
            {
                int32_t actualFrameIdx = frameIdx;

                // Repeat last frame for secondary animations that are shorter than their parents
                if ( shouldRepeatLastFrame && frameIdx >= numOriginalFrames )
                {
                    actualFrameIdx = numOriginalFrames - 1;
                }

                Transform const& boneTransform = trackTransformData.m_localTransforms[actualFrameIdx];

                // Rotation
                if ( Quaternion::Distance( previousRotation, boneTransform.GetRotation() ) > Math::LargeEpsilon )
                {
                    trackRangeData.m_isRotationConstant = false;
                }

                // Translation
                Float3 const& translation = boneTransform.GetTranslation().ToFloat3();
                trackRangeData.m_translationValueRangeX.GrowRange( translation.m_x );
                trackRangeData.m_translationValueRangeY.GrowRange( translation.m_y );
                trackRangeData.m_translationValueRangeZ.GrowRange( translation.m_z );

                // Scale
                trackRangeData.m_scaleValueRange.GrowRange( boneTransform.GetScale() );
            }
        }

        //-------------------------------------------------------------------------
        // Create track settings
        //-------------------------------------------------------------------------

        static constexpr float const defaultQuantizationRangeLength = 0.1f;

        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            TrackRangeData const& trackRangeData = trackRanges[boneIdx];
            TrackCompressionSettings trackSettings;

            //-------------------------------------------------------------------------

            trackSettings.m_isRotationStatic = trackRangeData.m_isRotationConstant;
            trackSettings.m_constantRotation = trackSettings.m_isRotationStatic ? rawTrackData[boneIdx].m_localTransforms[0].GetRotation() : Quaternion::Identity;

            //-------------------------------------------------------------------------

            auto const& rawTranslationValueRangeX = trackRangeData.m_translationValueRangeX;
            auto const& rawTranslationValueRangeY = trackRangeData.m_translationValueRangeY;
            auto const& rawTranslationValueRangeZ = trackRangeData.m_translationValueRangeZ;

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

            FloatRange const& rawScaleValueRange = trackRangeData.m_scaleValueRange;
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

            animClip.m_trackCompressionSettings.emplace_back( trackSettings );
        }

        //-------------------------------------------------------------------------
        // Create 'pose wise' compressed data
        //-------------------------------------------------------------------------

        for ( int32_t frameIdx = frameIdxStart; frameIdx < frameIdxEnd; frameIdx++ )
        {
            int32_t actualFrameIdx = frameIdx;

            // Repeat last frame for secondary animations that are shorter than their parents
            if ( shouldRepeatLastFrame && frameIdx >= numOriginalFrames )
            {
                actualFrameIdx = numOriginalFrames - 1;
            }

            animClip.m_compressedPoseOffsets.emplace_back( (int32_t) animClip.m_compressedPoseData.size() );

            // Record all bone rotations
            for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                TrackCompressionSettings const& trackSettings = animClip.m_trackCompressionSettings[boneIdx];

                if ( !trackSettings.IsRotationTrackStatic() )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[actualFrameIdx];
                    Quaternion const rotation = rawBoneTransform.GetRotation();

                    Quantization::EncodedQuaternion const encodedQuat( rotation );
                    animClip.m_compressedPoseData.push_back( encodedQuat.GetData0() );
                    animClip.m_compressedPoseData.push_back( encodedQuat.GetData1() );
                    animClip.m_compressedPoseData.push_back( encodedQuat.GetData2() );
                }

                if ( !trackSettings.IsTranslationTrackStatic() )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[actualFrameIdx];
                    Vector const& translation = rawBoneTransform.GetTranslation();

                    uint16_t const m_x = Quantization::EncodeFloat( translation.GetX(), trackSettings.m_translationRangeX.m_rangeStart, trackSettings.m_translationRangeX.m_rangeLength );
                    uint16_t const m_y = Quantization::EncodeFloat( translation.GetY(), trackSettings.m_translationRangeY.m_rangeStart, trackSettings.m_translationRangeY.m_rangeLength );
                    uint16_t const m_z = Quantization::EncodeFloat( translation.GetZ(), trackSettings.m_translationRangeZ.m_rangeStart, trackSettings.m_translationRangeZ.m_rangeLength );

                    animClip.m_compressedPoseData.push_back( m_x );
                    animClip.m_compressedPoseData.push_back( m_y );
                    animClip.m_compressedPoseData.push_back( m_z );
                }

                if ( !trackSettings.IsScaleTrackStatic() )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_localTransforms[actualFrameIdx];
                    uint16_t const m_x = Quantization::EncodeFloat( rawBoneTransform.GetScale(), trackSettings.m_scaleRange.m_rangeStart, trackSettings.m_scaleRange.m_rangeLength );
                    animClip.m_compressedPoseData.push_back( m_x );
                }
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------

    Resource::CompilationResult AnimationClipCompiler::ProcessEventsData( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::ImportedAnimation const& rawAnimData, AnimationClipEventData& outEventData ) const
    {
        Resource::CompilationResult result = Resource::CompilationResult::Success;

        float const lastFrameIdx = float( rawAnimData.GetNumFrames() - 1 );
        FloatRange const animationTimeRange( 0, lastFrameIdx );
        Timeline::TrackContext trackContext( lastFrameIdx );

        // Reflect into runtime events
        //-------------------------------------------------------------------------

        int32_t numSyncTracks = 0;

        TVector<TTypeInstance<Event>> events;
        for ( TTypeInstance<Timeline::Track> const& track : resourceDescriptor.m_events.GetTracks() )
        {
            auto trackStatus = track->GetValidationStatus( trackContext );

            //-------------------------------------------------------------------------

            if ( trackStatus == Timeline::Track::Status::HasErrors )
            {
                if ( track->IsRenameable() )
                {
                    result = Warning( "Invalid animation event track (%s - %s) encountered!", track->GetName(), track->GetTypeName() );
                }
                else
                {
                    result = Warning( "Invalid animation event track (%s) encountered!", track->GetTypeName() );
                }

                // Skip invalid tracks
                continue;
            }

            //-------------------------------------------------------------------------

            if ( trackStatus == Timeline::Track::Status::HasWarnings )
            {
                if ( track->IsRenameable() )
                {
                    result = Warning( "Animation event track (Track: %s, Type: %s) has warnings: %s", track->GetName(), track->GetTypeName(), track->GetStatusMessage().c_str() );
                }
                else
                {
                    result = Warning( "Animation event track (%s) has warnings: %s", track->GetTypeName(), track->GetStatusMessage().c_str() );
                }
            }

            //-------------------------------------------------------------------------

            auto pEventTrack = Cast<EventTrack>( track.Get() );

            if ( pEventTrack->IsSyncTrack() )
            {
                numSyncTracks++;
            }

            Seconds const clipDuration = rawAnimData.GetDuration();

            for ( TTypeInstance<Timeline::TrackItem> const& item : track->GetItems() )
            {
                EventTrackItem const* pEventItem = Cast<EventTrackItem>( item.Get() );
                Event const* pEvent = pEventItem->GetEvent();
                if ( !pEvent->IsValid() )
                {
                    result = Warning( "Animation event track (%s) has warnings: %s", track->GetTypeName(), track->GetStatusMessage().c_str() );
                    continue;
                }

                // Add event
                //-------------------------------------------------------------------------

                // Ignore any event entirely outside the animation time range
                FloatRange eventTimeRange = item->GetTimeRange();
                if ( !animationTimeRange.Overlaps( eventTimeRange ) )
                {
                    result = Warning( "Event detected outside animation time range, event will be ignored" );
                    continue;
                }

                // Clamp events that extend out of the animation time range to the animation time range
                if ( !animationTimeRange.ContainsInclusive( eventTimeRange ) )
                {
                    result = Warning( "Event extend outside the valid animation time range, event will be clamped to animation range" );

                    eventTimeRange.m_begin = animationTimeRange.GetClampedValue( eventTimeRange.m_begin );
                    eventTimeRange.m_end = animationTimeRange.GetClampedValue( eventTimeRange.m_end );
                    EE_ASSERT( eventTimeRange.IsSetAndValid() );
                }

                // Set event time (in Seconds) and add new event
                TTypeInstance<Event>& eventInstance = events.emplace_back();
                eventInstance.CopyFrom( pEvent );
                eventInstance->m_startTime = ( eventTimeRange.m_begin / lastFrameIdx ) * clipDuration;
                eventInstance->m_duration = ( eventTimeRange.GetLength() / lastFrameIdx ) * clipDuration;

                // Create sync event
                //-------------------------------------------------------------------------

                if ( pEventTrack->IsSyncTrack() && numSyncTracks == 1 )
                {
                    Seconds const startTime = eventInstance->m_startTime / clipDuration;
                    outEventData.m_syncEventMarkers.emplace_back( SyncTrack::EventMarker( Percentage( startTime.ToFloat() ), pEvent->GetSyncEventID() ) );
                }
            }
        }

        if ( numSyncTracks > 1 )
        {
            result = Warning( "Multiple sync tracks detected, using the first one encountered!" );
        }

        // Transfer sorted events
        //-------------------------------------------------------------------------

        auto sortPredicate = [] ( TTypeInstance<Event> const& eventA, TTypeInstance<Event> const& eventB )
        {
            return eventA->GetStartTime() < eventA->GetStartTime();
        };

        eastl::sort( events.begin(), events.end(), sortPredicate );

        for ( TTypeInstance<Event>& event : events )
        {
            outEventData.m_collection.m_descriptors.emplace_back( TypeSystem::TypeDescriptor( *m_pTypeRegistry, event.Get() ) );
        }

        eastl::sort( outEventData.m_syncEventMarkers.begin(), outEventData.m_syncEventMarkers.end() );

        //-------------------------------------------------------------------------

        return result;
    }

    bool AnimationClipCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        // Try read descriptor
        //-------------------------------------------------------------------------

        FileSystem::Path const descriptorFilePath = resourceID.GetFileSystemPath( m_sourceDataDirectoryPath );
        AnimationClipResourceDescriptor resourceDescriptor;
        if ( !TryLoadResourceDescriptor( descriptorFilePath, resourceDescriptor ) )
        {
            Error( "Failed to read resource descriptor file: %s", descriptorFilePath.c_str() );
            return false;
        }

        // Add skeletons
        //-------------------------------------------------------------------------

        outReferencedResources.emplace_back( resourceDescriptor.m_skeleton.GetResourceID() );
        
        // Add secondary animation skeletons as install dependencies
        for ( auto const& secondaryAnim : resourceDescriptor.m_secondaryAnimations )
        {
            VectorEmplaceBackUnique( outReferencedResources, secondaryAnim.m_skeleton.GetResourceID() );
        }

        // Try read event tracks
        //-------------------------------------------------------------------------

        for ( TTypeInstance<Timeline::Track> const& track : resourceDescriptor.m_events.GetTracks() )
        {
            for ( TTypeInstance<Timeline::TrackItem> const& item : track->GetItems() )
            {
                EventTrackItem const* pEventItem = Cast<EventTrackItem>( item.Get() );
                Event const* pEvent = pEventItem->GetEvent();
                pEvent->GetTypeInfo()->GetReferencedResources( pEvent, outReferencedResources );
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }
}
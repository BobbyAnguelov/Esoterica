#include "ResourceCompiler_AnimationClip.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
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
#include "Base/Math/MathRandom.h"
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
        RegisterOutput<AnimationClip>();
    }

    void AnimationClipCompiler::GenerateCustomDependencyHashes( Resource::CompileContext const& ctx, Resource::CompileDependencyResourceInfo* pResourceToCompile ) const
    {
        EE_ASSERT( ctx.IsValid() );

        auto const pResourceDescriptor = ctx.GetDescriptor<AnimationClipResourceDescriptor>();

        TInlineVector<Resource::CompileDependencyResourceInfo*, 2> skeletonInfos;
        ctx.m_pResourceToCompile->GetResourceInfo( pResourceDescriptor->m_skeleton.GetDataPath(), skeletonInfos );

        for ( auto pSkeletonInfo : skeletonInfos )
        {
            if ( pSkeletonInfo->HasError() )
            {
                continue;
            }

            auto pSkeletonDesc = Cast<SkeletonResourceDescriptor>( pSkeletonInfo->m_pDescriptor );
            pSkeletonInfo->m_customSourceHash = pSkeletonDesc->GetClipDependencyCompileHash();
        }
    }

    Resource::CompilationResult AnimationClipCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        Resource::CompilationResult result = Resource::CompilationResult::Success;

        //-------------------------------------------------------------------------

        Milliseconds timeTaken = 0;

        // Read descriptor
        //-------------------------------------------------------------------------

        auto pResourceDescriptor = ctx.GetDescriptor<AnimationClipResourceDescriptor>();

        TVector<DataPath> secondarySkeletonPaths;
        for ( auto const& secondarySkeleton : pResourceDescriptor->m_secondarySkeletons )
        {
            if ( !secondarySkeleton.IsSet() )
            {
                continue;
            }

            if ( secondarySkeleton.GetDataPath() == pResourceDescriptor->m_skeleton.GetDataPath() )
            {
                return ctx.LogError( "Secondary animations are not allowed to be on the same skeleton as the parent" );
            }

            VectorEmplaceBackUnique( secondarySkeletonPaths, secondarySkeleton.GetDataPath() );
        }

        // Read animation data
        //-------------------------------------------------------------------------

        TUniquePtr<Import::Animation> importedAnimationPtr = nullptr;
        result = KeepHighestSeverityCompilationResult( result, ReadImportedAnimation( ctx, pResourceDescriptor->m_skeleton.GetDataPath(), secondarySkeletonPaths, pResourceDescriptor->m_animationPath, importedAnimationPtr, pResourceDescriptor->m_animationName ) );
        if ( result == Resource::CompilationResult::Failure )
        {
            return ctx.LogError( "Failed to read raw animation data!" );
        }

        // Validate frame limit
        //-------------------------------------------------------------------------

        IntRange limitFrameRange = pResourceDescriptor->m_limitFrameRange;

        if ( pResourceDescriptor->m_limitFrameRange.IsSet() )
        {
            if ( importedAnimationPtr->GetNumFrames() == 1 )
            {
                return ctx.LogError( "Having a frame limit range on a single frame animation doesn't make sense!" );
            }

            limitFrameRange.m_begin = Math::Clamp( pResourceDescriptor->m_limitFrameRange.m_begin, 0, (int32_t) importedAnimationPtr->GetNumFrames() - 1 );
            limitFrameRange.m_end = Math::Clamp( pResourceDescriptor->m_limitFrameRange.m_end, 0, (int32_t) importedAnimationPtr->GetNumFrames() - 1 ); // Inclusive range so we need to increment the index

            if ( !limitFrameRange.IsValid() )
            {
                return ctx.LogError( "Invalid frame limit range set!" );
            }
        }

        // Auto-generate root motion
        //-------------------------------------------------------------------------

        if ( pResourceDescriptor->m_regenerateRootMotion )
        {
            {
                ScopedTimer<PlatformClock> timer( timeTaken );
                result = KeepHighestSeverityCompilationResult( result, RegenerateRootMotion( ctx, *pResourceDescriptor, importedAnimationPtr.get() ) );
                if ( result == Resource::CompilationResult::Failure )
                {
                    return ctx.LogError( "Failed to generate root motion data!" );
                }
            }
            ctx.LogMessage( "Root Motion Generation: %.3fms", timeTaken.ToFloat() );
        }

        // Additive generation
        //-------------------------------------------------------------------------

        if ( pResourceDescriptor->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::None )
        {
            {
                ScopedTimer<PlatformClock> timer( timeTaken );

                AnimationClipResourceDescriptor::AdditiveType actualAdditiveType = pResourceDescriptor->m_additiveType;

                //-------------------------------------------------------------------------

                if ( actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToSkeleton )
                {
                    importedAnimationPtr->MakeAdditiveRelativeToSkeleton();
                }
                else if ( actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame )
                {
                    if ( pResourceDescriptor->m_additiveBaseFrameIndex < 0 )
                    {
                        return ctx.LogError( "Invalid additive base frame index!" );
                    }

                    if ( pResourceDescriptor->m_additiveBaseFrameIndex >= importedAnimationPtr->GetNumFrames() )
                    {
                        ctx.LogWarning( "Specified additive base frame index is higher than the number of frames available, clamping to the last frame!" );
                    }

                    int32_t const actualBaseFrameIdx = Math::Clamp( (int32_t) pResourceDescriptor->m_additiveBaseFrameIndex, 0, importedAnimationPtr->GetNumFrames() - 1 );
                    importedAnimationPtr->MakeAdditiveRelativeToFrame( actualBaseFrameIdx );
                }
                else if ( actualAdditiveType == AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip )
                {
                    if ( !pResourceDescriptor->m_additiveBaseAnimationPath.IsValid() )
                    {
                        return ctx.LogError( "No additive base animation clip set!" );
                    }

                    TUniquePtr<Import::Animation> importedBaseAnimationPtr = nullptr;
                    result = KeepHighestSeverityCompilationResult( result, ReadImportedAnimation( ctx, pResourceDescriptor->m_skeleton.GetDataPath(), secondarySkeletonPaths, pResourceDescriptor->m_additiveBaseAnimationPath, importedBaseAnimationPtr ) );
                    if ( result == Resource::CompilationResult::Failure )
                    {
                        return ctx.LogError( "Failed to read raw animation data for additive base animation!" );
                    }

                    int32_t const actualBaseFrameIdx = Math::Clamp( (int32_t) pResourceDescriptor->m_additiveBaseFrameIndex, -1, importedAnimationPtr->GetNumFrames() - 1 );
                    importedAnimationPtr->MakeAdditiveRelativeToAnimation( *importedBaseAnimationPtr.get(), actualBaseFrameIdx );
                }
            }

            ctx.LogMessage( "Additive Generation: %.3fms", timeTaken.ToFloat() );
        }

        // Compress raw animation data into runtime format
        //-------------------------------------------------------------------------

        AnimationClip animClip;
        TVector<AnimationClip> secondaryAnimClips;

        {
            ScopedTimer<PlatformClock> timer( timeTaken );
            animClip.m_skeleton = pResourceDescriptor->m_skeleton;
            result = KeepHighestSeverityCompilationResult( result, TransferAndCompressAnimationData( ctx, *importedAnimationPtr, importedAnimationPtr->GetPrimaryClip(), animClip, limitFrameRange, pResourceDescriptor->m_bonesToSampleInModelSpace, true ) );
            if ( result == Resource::CompilationResult::Failure )
            {
                return ctx.LogError( "Failed to compress animation!" );
            }

            for ( int32_t i = 0; i < importedAnimationPtr->GetNumSecondaryClips(); i++ )
            {
                Import::AnimationClip const& importedSecondaryClip = importedAnimationPtr->GetSecondaryClip( i );
                if ( importedSecondaryClip.m_hasData )
                {
                    AnimationClip& secondaryAnimClip = secondaryAnimClips.emplace_back();
                    secondaryAnimClip.m_skeleton = Resource::ResourcePtr( secondarySkeletonPaths[i] );
                    result = KeepHighestSeverityCompilationResult( result, TransferAndCompressAnimationData( ctx, *importedAnimationPtr, importedSecondaryClip, secondaryAnimClip, limitFrameRange, pResourceDescriptor->m_bonesToSampleInModelSpace, false ) );
                    if ( result == Resource::CompilationResult::Failure )
                    {
                        return ctx.LogError( "Failed to compress secondary animation!" );
                    }
                }
            }
        }
        ctx.LogMessage( "Compression: %.3fms", timeTaken.ToFloat() );

        // Handle events
        //-------------------------------------------------------------------------

        AnimationClipEventData eventData;
        {
            ScopedTimer<PlatformClock> timer( timeTaken );
            result = KeepHighestSeverityCompilationResult( result, ProcessEventsData( ctx, *pResourceDescriptor, *importedAnimationPtr, eventData ) );
            if ( result == Resource::CompilationResult::Failure )
            {
                return ctx.LogError( "Failed to read animation events!" );
            }
        }
        ctx.LogMessage( "Read Animation Events: %.3fms", timeTaken.ToFloat() );

        // Duration override
        //-------------------------------------------------------------------------

        if ( !animClip.IsSingleFrameAnimation() && pResourceDescriptor->m_durationOverride != AnimationClipResourceDescriptor::DurationOverride::None )
        {
            float durationMultiplier = pResourceDescriptor->m_durationOverrideValue;

            if ( pResourceDescriptor->m_durationOverride == AnimationClipResourceDescriptor::DurationOverride::FixedValue )
            {
                EE_ASSERT( animClip.GetDuration() != 0.0f );
                durationMultiplier = pResourceDescriptor->m_durationOverrideValue / animClip.GetDuration();
            }

            if ( durationMultiplier <= 0.0f )
            {
                return ctx.LogError( "Invalid duration override value set!" );
            }

            //-------------------------------------------------------------------------

            animClip.m_duration *= durationMultiplier;

            for ( AnimationClip& secondaryClip : secondaryAnimClips )
            {
                secondaryClip.m_duration *= durationMultiplier;
            }
        }

        // Serialize animation data
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( AnimationClip::s_version, AnimationClip::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        hdr.AddInstallDependency( pResourceDescriptor->m_skeleton.GetResourceID() );

        // Add secondary animation skeletons as install dependencies
        for ( auto const& secondarySkeleton : secondarySkeletonPaths )
        {
            hdr.AddInstallDependency( secondarySkeleton );
        }

        Serialization::BinaryOutputArchive archive;
        archive << hdr << animClip;
        archive << eventData.m_syncEventMarkers;
        archive << eventData.m_collection;

        archive << (int32_t) secondaryAnimClips.size();
        for ( AnimationClip const& secondaryClip : secondaryAnimClips )
        {
            archive << secondaryClip;
        }

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            if ( importedAnimationPtr->HasWarnings() || result == Resource::CompilationResult::SuccessWithWarnings )
            {
                return Resource::CompilationResult::SuccessWithWarnings;
            }
            else
            {
                return Resource::CompilationResult::Success;
            }
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }

    //-------------------------------------------------------------------------

    Resource::CompilationResult AnimationClipCompiler::ReadImportedAnimation( Resource::CompileContext const& ctx, DataPath const& skeletonPath, TVector<DataPath> const& secondarySkeletonPaths, DataPath const& animationPath, TUniquePtr<Import::Animation>& outAnimation, String const& animationName ) const
    {
        Timer<PlatformClock> timer;
        EE_ASSERT( outAnimation == nullptr );

        Import::ReaderContext readerCtx = { [&ctx]( char const* pString ) { ctx.LogWarning( pString ); }, [&ctx] ( char const* pString ) { ctx.LogError( pString ); } };

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        timer.Start();

        auto ReadSkeletonData = [&ctx, &readerCtx] ( DataPath const& skeletonPath, TUniquePtr<Import::Skeleton>& outSkeleton )
        {
            auto pSkeletonResourceDescriptor = ctx.GetDescriptor<SkeletonResourceDescriptor>( skeletonPath );

            FileSystem::Path const skeletonFilePath = pSkeletonResourceDescriptor->m_skeletonPath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );

            Import::Source const skeletonFileSource( skeletonFilePath, ctx.GetRawData( pSkeletonResourceDescriptor->m_skeletonPath ) );
            outSkeleton = Import::Importer::ReadSkeleton( readerCtx, skeletonFileSource, pSkeletonResourceDescriptor->m_skeletonRootBoneName, pSkeletonResourceDescriptor->m_highLODBones );
            if ( outSkeleton == nullptr || !outSkeleton->IsValid() )
            {
                ctx.LogError( "Failed to read skeleton file: %s", skeletonFilePath.ToString().c_str() );
                return false;
            }

            return true;
        };

        TUniquePtr<Import::Skeleton> primarySkeleton;
        if ( !ReadSkeletonData( skeletonPath, primarySkeleton ) )
        {
            return Resource::CompilationResult::Failure;
        }

        TVector<TUniquePtr<Import::Skeleton>> secondarySkeletons;
        for ( auto const& secondarySkeletonDataPath : secondarySkeletonPaths )
        {
            if ( !ReadSkeletonData( secondarySkeletonDataPath, secondarySkeletons.emplace_back() ) )
            {
                return Resource::CompilationResult::Failure;
            }
        }

        ctx.LogMessage( "Read Source Skeleton Data: %.3fms", timer.GetElapsedTimeMilliseconds().ToFloat() );

        // Read animation data
        //-------------------------------------------------------------------------

        timer.Start();

        FileSystem::Path const animationFilePath = animationPath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );
        if ( !FileSystem::Exists( animationFilePath ) )
        {
            return ctx.LogError( "Invalid animation file path: %s", animationFilePath.ToString().c_str() );
        }

        TVector<Import::Skeleton const*> secondarySkeletonPtrs;
        for ( auto const& ss : secondarySkeletons )
        {
            secondarySkeletonPtrs.emplace_back( ss.get() );
        }

        Import::Source const animFileSource( animationFilePath, ctx.GetRawData( animationPath ) );
        outAnimation = Import::Importer::ReadAnimation( readerCtx, animFileSource, primarySkeleton.get(), secondarySkeletonPtrs, animationName );
        if ( outAnimation == nullptr )
        {
            return ctx.LogError( "Failed to read animation from source file" );
        }

        ctx.LogMessage( "Read Source Animation Data: %.3fms", timer.GetElapsedTimeMilliseconds().ToFloat() );

        //-------------------------------------------------------------------------

        return Resource::CompilationResult::Success;;
    }

    Resource::CompilationResult AnimationClipCompiler::RegenerateRootMotion( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::Animation* pImportedAnimation ) const
    {
        EE_ASSERT( pImportedAnimation != nullptr && pImportedAnimation->IsValid() );

        Import::AnimationClip& primaryClip = pImportedAnimation->GetPrimaryClip();

        // Validate Root Motion Generation settings
        //-------------------------------------------------------------------------

        int32_t rootMotionGenerationBoneIdx = InvalidIndex;
        if ( resourceDescriptor.m_regenerateRootMotion )
        {
            if ( resourceDescriptor.m_rootMotionGenerationBoneID.IsValid() )
            {
                rootMotionGenerationBoneIdx = primaryClip.GetSkeleton().GetBoneIndex( resourceDescriptor.m_rootMotionGenerationBoneID );
                if ( rootMotionGenerationBoneIdx == InvalidIndex )
                {
                    return ctx.LogError( "Root Motion Generation: Skeleton doesnt contain specified bone: %s", resourceDescriptor.m_rootMotionGenerationBoneID.c_str() );
                }
            }
            else
            {
                return ctx.LogError( "Root Motion Generation: No bone specified for root motion generation!" );
            }
        }

        // Calculate root position based on specified bone
        //-------------------------------------------------------------------------

        auto const& sourceBoneTrackData = primaryClip.m_tracks[rootMotionGenerationBoneIdx];
        auto& rootTrackData = primaryClip.m_tracks[0];
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
        primaryClip.RecreateParentSpaceTransformsFromModelSpaceTransform();

        //-------------------------------------------------------------------------

        return Resource::CompilationResult::Success;
    }

    Resource::CompilationResult AnimationClipCompiler::TransferAndCompressAnimationData( Resource::CompileContext const& ctx, Import::Animation const& importedAnimation, Import::AnimationClip const& importedClip, AnimationClip& outAnimClip, IntRange const& limitRange, TVector<StringID> const& bonesToSampleInModelSpace, bool isPrimaryClip ) const
    {
        EE_ASSERT( importedClip.m_hasData );

        Resource::CompilationResult result = Resource::CompilationResult::Success;
        TVector<Import::AnimationClip::TrackData> const& rawTrackData = importedClip.GetTrackData();
        uint32_t const numBones = importedClip.GetNumBones();
        int32_t const numOriginalFrames = importedAnimation.GetNumFrames();

        //-------------------------------------------------------------------------
        // Calculate frame limits
        //-------------------------------------------------------------------------
        // The limit range is an inclusive range that specifies the exact start and frame index to include

        int32_t frameIdxStart = 0;
        int32_t frameIdxEnd = numOriginalFrames - 1;

        if ( limitRange.IsSetAndValid() )
        {
            if( limitRange.m_end >= numOriginalFrames )
            {
                ctx.LogWarning( "Frame range limit exceed available frames, clamping to end of animation!" );
            }

            frameIdxStart = Math::Max( limitRange.m_begin, 0 );
            frameIdxEnd = Math::Min( limitRange.m_end, numOriginalFrames - 1 );
        }

        outAnimClip.m_numFrames = ( frameIdxEnd == frameIdxStart ) ? 1 : frameIdxEnd - frameIdxStart + 1;

        //-------------------------------------------------------------------------
        // Transfer basic animation data
        //-------------------------------------------------------------------------

        float const FPS = ( numOriginalFrames - 1 ) / importedAnimation.GetDuration().ToFloat();
        outAnimClip.m_duration = ( outAnimClip.IsSingleFrameAnimation() ) ? Seconds( 0.0f ) : Seconds( ( outAnimClip.m_numFrames - 1 ) / FPS );
        outAnimClip.m_isAdditive = importedAnimation.IsAdditive();

        //-------------------------------------------------------------------------
        // Root Motion
        //-------------------------------------------------------------------------

        if ( isPrimaryClip )
        {
            // Transfer root motion (and duplicate frames if needed)
            //-------------------------------------------------------------------------

            RootMotionData& rootMotionData = outAnimClip.m_rootMotion;
            rootMotionData.m_numFrames = outAnimClip.m_numFrames;
            rootMotionData.m_totalDelta = Transform::Identity;
            rootMotionData.m_averageLinearVelocity = 0.0f;
            rootMotionData.m_averageAngularVelocity = 0.0f;

            EE_ASSERT( frameIdxEnd < numOriginalFrames );

            // Ensure that the root motion starts at the origin
            TVector<Transform> const& rootMotion = importedAnimation.GetRootMotion();
            Vector const rootMotionTranslationOffset = -rootMotion[frameIdxStart].GetTranslation();

            for ( int32_t frameIdx = frameIdxStart; frameIdx <= frameIdxEnd; frameIdx++ )
            {
                Transform& addedRootMotionTransform = outAnimClip.m_rootMotion.m_transforms.emplace_back( rootMotion[frameIdx] );
                addedRootMotionTransform.AddTranslation( rootMotionTranslationOffset );
            }

            // Calculate root motion extra data
            //-------------------------------------------------------------------------

            if ( outAnimClip.IsSingleFrameAnimation() )
            {
                rootMotionData.m_transforms.clear();
                rootMotionData.m_transforms.emplace_back( importedAnimation.GetRootMotion()[0] );
            }
            else // Calculate extra info
            {
                float totalDistance = 0.0f;
                float totalRotation = 0.0f;
                bool containsRootMotion = false;

                for ( int32_t i = 1; i < outAnimClip.m_numFrames; i++ )
                {
                    // Track deltas
                    Transform const deltaRoot = Transform::DeltaNoScale( rootMotionData.m_transforms[i - 1], rootMotionData.m_transforms[i] );
                    totalDistance += deltaRoot.GetTranslation().GetLength3();

                    // If we have a rotation delta, accumulate the yaw value
                    if ( !deltaRoot.GetRotation().IsIdentity() )
                    {
                        Vector const deltaForward2D = deltaRoot.GetForwardVector().GetNormalized2();
                        Radians const deltaAngle = Math::CalculateYawAngleBetweenVectors( deltaForward2D, Vector::WorldBackward ).GetClamped360();
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
                    rootMotionData.m_averageLinearVelocity = totalDistance / outAnimClip.GetDuration();
                    rootMotionData.m_averageAngularVelocity = totalRotation / outAnimClip.GetDuration();
                }
                else // No root motion, so clear all the identical transforms
                {
                    rootMotionData.m_transforms.clear();
                    rootMotionData.m_transforms.emplace_back( importedAnimation.GetRootMotion()[0] );
                }
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
            Import::AnimationClip::TrackData const& trackTransformData = rawTrackData[boneIdx];
            Quaternion previousRotation = trackTransformData.m_parentSpaceTransforms[frameIdxStart].GetRotation();
            for ( int32_t frameIdx = frameIdxStart; frameIdx <= frameIdxEnd; frameIdx++ )
            {
                EE_ASSERT( frameIdx < numOriginalFrames );

                Transform const& boneTransform = trackTransformData.m_parentSpaceTransforms[frameIdx];

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
        // Create track definitions
        //-------------------------------------------------------------------------

        static constexpr float const defaultQuantizationRangeLength = 0.1f;

        int32_t poseTrackReadOffset = 0;
        for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            TrackRangeData const& trackRangeData = trackRanges[boneIdx];
            TrackDefinition trackSettings;
            trackSettings.m_trackReadOffset = poseTrackReadOffset;

            //-------------------------------------------------------------------------

            trackSettings.m_isRotationStatic = trackRangeData.m_isRotationConstant;
            trackSettings.m_constantRotation = trackSettings.m_isRotationStatic ? rawTrackData[boneIdx].m_parentSpaceTransforms[0].GetRotation() : Quaternion::Identity;

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
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_parentSpaceTransforms[0];
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
                Transform const& rawBoneTransform = rawTrackData[boneIdx].m_parentSpaceTransforms[0];
                trackSettings.m_scaleRange = { rawBoneTransform.GetScale(), defaultQuantizationRangeLength };
                trackSettings.m_isScaleStatic = true;
            }
            else
            {
                trackSettings.m_scaleRange = { rawScaleValueRange.m_begin, Math::IsNearZero( rawScaleValueRangeLengthX ) ? defaultQuantizationRangeLength : rawScaleValueRangeLengthX };
            }

            //-------------------------------------------------------------------------

            outAnimClip.m_trackDefs.emplace_back( trackSettings );

            // Update track read offsetS
            //-------------------------------------------------------------------------

            if ( !trackSettings.IsRotationTrackStatic() )
            {
                poseTrackReadOffset += 3;
            }

            if ( !trackSettings.IsTranslationTrackStatic() )
            {
                poseTrackReadOffset += 3;
            }

            if ( !trackSettings.IsScaleTrackStatic() )
            {
                poseTrackReadOffset += 1;
            }
        }

        //-------------------------------------------------------------------------
        // Create 'pose wise' compressed track data
        //-------------------------------------------------------------------------

        for ( int32_t frameIdx = frameIdxStart; frameIdx <= frameIdxEnd; frameIdx++ )
        {
            EE_ASSERT( frameIdx < numOriginalFrames );

            outAnimClip.m_compressedPoseOffsets.emplace_back( (int32_t) outAnimClip.m_compressedPoseData.size() );

            for ( uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                TrackDefinition const& trackSettings = outAnimClip.m_trackDefs[boneIdx];

                if ( !trackSettings.IsRotationTrackStatic() )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_parentSpaceTransforms[frameIdx];
                    Quaternion const rotation = rawBoneTransform.GetRotation();

                    Quantization::EncodedQuaternion const encodedQuat( rotation );
                    outAnimClip.m_compressedPoseData.push_back( encodedQuat.GetData0() );
                    outAnimClip.m_compressedPoseData.push_back( encodedQuat.GetData1() );
                    outAnimClip.m_compressedPoseData.push_back( encodedQuat.GetData2() );
                }

                if ( !trackSettings.IsTranslationTrackStatic() )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_parentSpaceTransforms[frameIdx];
                    Vector const& translation = rawBoneTransform.GetTranslation();

                    uint16_t const tx = Quantization::EncodeFloat( translation.GetX(), trackSettings.m_translationRangeX.m_rangeStart, trackSettings.m_translationRangeX.m_rangeLength );
                    uint16_t const ty = Quantization::EncodeFloat( translation.GetY(), trackSettings.m_translationRangeY.m_rangeStart, trackSettings.m_translationRangeY.m_rangeLength );
                    uint16_t const tz = Quantization::EncodeFloat( translation.GetZ(), trackSettings.m_translationRangeZ.m_rangeStart, trackSettings.m_translationRangeZ.m_rangeLength );

                    outAnimClip.m_compressedPoseData.push_back( tx );
                    outAnimClip.m_compressedPoseData.push_back( ty );
                    outAnimClip.m_compressedPoseData.push_back( tz );
                }

                if ( !trackSettings.IsScaleTrackStatic() )
                {
                    Transform const& rawBoneTransform = rawTrackData[boneIdx].m_parentSpaceTransforms[frameIdx];
                    uint16_t const scl = Quantization::EncodeFloat( rawBoneTransform.GetScale(), trackSettings.m_scaleRange.m_rangeStart, trackSettings.m_scaleRange.m_rangeLength );
                    outAnimClip.m_compressedPoseData.push_back( scl );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Create model space sampling bone chain
        //-------------------------------------------------------------------------

        if ( isPrimaryClip && !bonesToSampleInModelSpace.empty() )
        {
            TVector<ModelSpaceSamplingChainLink> chain;
            TVector<int32_t> boneIndices;

            for ( StringID const &boneName : bonesToSampleInModelSpace )
            {
                int32_t boneIdx = outAnimClip.m_skeleton->GetBoneIndex( boneName );
                if ( boneIdx == InvalidIndex )
                {
                    ctx.LogWarning( "Invalid bone name specified in model space sampling list: %s", boneName.c_str() );
                }
                else
                {
                    boneIndices.emplace_back( boneIdx );
                    ModelSpaceSamplingChainLink& chainLink = chain.emplace_back();
                    chainLink.m_boneIdx = boneIdx;
                    chainLink.m_parentBoneIdx = outAnimClip.m_skeleton->GetParentBoneIndex( boneIdx );

                    int32_t parentBoneIdx = chainLink.m_parentBoneIdx;
                    while ( parentBoneIdx != InvalidIndex )
                    {
                        bool isNewLink = true;
                        for ( auto const &link : chain )
                        {
                            if ( link.m_boneIdx == parentBoneIdx )
                            {
                                isNewLink = false;
                                break;
                            }
                        }

                        // Add new new link
                        if ( isNewLink )
                        {
                            ModelSpaceSamplingChainLink& newParentChainLink = chain.emplace_back();
                            newParentChainLink.m_boneIdx = parentBoneIdx;
                            newParentChainLink.m_parentBoneIdx = outAnimClip.m_skeleton->GetParentBoneIndex( parentBoneIdx );
                        }

                        parentBoneIdx = outAnimClip.m_skeleton->GetParentBoneIndex( parentBoneIdx );
                    }
                }
            }

            // Sort chain by bone idx
            auto SortPredicate = [] ( ModelSpaceSamplingChainLink const &lhs, ModelSpaceSamplingChainLink const &rhs )
            {
                return lhs.m_boneIdx < rhs.m_boneIdx;
            };

            eastl::sort( chain.begin(), chain.end(), SortPredicate );

            // Setup chain indices
            for ( int32_t chainLinkIdx = 0; chainLinkIdx < (int32_t) chain.size(); chainLinkIdx++ )
            {
                // Check other links for the same 
                for ( int32_t parentChainLinkIdx = 0; parentChainLinkIdx < (int32_t) chain.size(); parentChainLinkIdx++ )
                {
                    if ( chain[parentChainLinkIdx].m_boneIdx == chain[chainLinkIdx].m_parentBoneIdx )
                    {
                        chain[chainLinkIdx].m_parentChainLinkIdx = parentChainLinkIdx;
                    }
                }
            }

            for ( int32_t boneIdx : boneIndices )
            {
                for ( int32_t chainLinkIdx = 0; chainLinkIdx < (int32_t) chain.size(); chainLinkIdx++ )
                {
                    if ( chain[chainLinkIdx].m_boneIdx == boneIdx )
                    {
                        outAnimClip.m_modelSpaceBoneSamplingIndices.emplace_back( chainLinkIdx );
                    }
                }
            }

            outAnimClip.m_modelSpaceSamplingChain = chain;
        }

        //-------------------------------------------------------------------------
        // Separate and Compress Float Channel Data
        //-------------------------------------------------------------------------

        Import::AnimationClip::FloatChannelData emptyChannel;
        emptyChannel.m_values.emplace_back( 0.0f );

        auto LookupChannel = [&importedClip, &emptyChannel] ( StringID channelID ) -> Import::AnimationClip::FloatChannelData const*
        {
            for ( auto& c : importedClip.m_floatChannels )
            {
                if ( c.m_ID == channelID )
                {
                    return &c;
                }
            }

            return &emptyChannel;
        };

        auto pSkeletonResourceDescriptor = ctx.GetDescriptor<SkeletonResourceDescriptor>( outAnimClip.m_skeleton.GetResourceID() );
        EE_ASSERT( pSkeletonResourceDescriptor != nullptr );
        size_t const numFloatChannelSets = pSkeletonResourceDescriptor->m_floatChannelSets.size();

        // Create channel/set links
        TVector<TVector<Import::AnimationClip::FloatChannelData const*>> channelSetData;
        channelSetData.resize( numFloatChannelSets );

        for ( size_t setIdx = 0; setIdx < numFloatChannelSets; setIdx++ )
        {
            FloatChannelSet const& channelSet = pSkeletonResourceDescriptor->m_floatChannelSets[setIdx];
            size_t const numChannels = channelSet.GetNumChannels();
            channelSetData[setIdx].resize( numChannels );

            for ( size_t channelIdx = 0; channelIdx < numChannels; channelIdx++ )
            {
                channelSetData[setIdx][channelIdx] = LookupChannel( channelSet.m_channelIDs[channelIdx] );
            }
        }

        // Compress channels
        for ( size_t setIdx = 0; setIdx < numFloatChannelSets; setIdx++ )
        {
            // Does this set have any data?
            bool hasSetData = false;
            for ( auto const& channel : channelSetData[setIdx] )
            {
                if ( !channel->HasValue() )
                {
                    hasSetData = true;
                    break;
                }
            }

            if ( !hasSetData )
            {
                continue;
            }

            // Create new channel data
            FloatChannelData& channelData = outAnimClip.m_floatChannelSetData.emplace_back();
            channelData.m_setID = pSkeletonResourceDescriptor->m_floatChannelSets[setIdx].m_ID;

            // Build channel ranges
            for ( auto const& pImportedChannel : channelSetData[setIdx] )
            {
                bool const isStatic = pImportedChannel->m_values.size() == 1;
                EE_ASSERT( isStatic || pImportedChannel->m_values.size() == int32_t( outAnimClip.m_numFrames ) );

                FloatRange rawFloatChannelRange( pImportedChannel->m_values[0] );
                for ( int32_t floatValueIdx = 1; floatValueIdx < pImportedChannel->m_values.size(); ++floatValueIdx )
                {
                    rawFloatChannelRange.GrowRange( pImportedChannel->m_values[floatValueIdx] );
                }

                FloatChannelData::ChannelSettings& channelSettings = channelData.m_channelSettings.emplace_back();
                channelSettings.m_isStatic = isStatic;
                channelSettings.m_range.m_rangeStart = rawFloatChannelRange.m_begin;
                channelSettings.m_range.m_rangeLength = rawFloatChannelRange.m_end - rawFloatChannelRange.m_begin;
            }

            // Bake out key per frame compressed values for all non-static channels.
            for ( int32_t frameIdx = 0; frameIdx < outAnimClip.m_numFrames; frameIdx++ )
            {
                channelData.m_compressedOffsets.emplace_back( (uint32_t) channelData.m_compressedData.size() );

                for ( int32_t channelIdx = 0; channelIdx < channelSetData[setIdx].size(); channelIdx++ )
                {
                    FloatChannelData::ChannelSettings const &channelSettings = channelData.m_channelSettings[channelIdx];
                    if ( channelSettings.m_isStatic )
                    {
                        continue;
                    }

                    uint16_t const nQuantizedVal = Quantization::EncodeFloat( channelSetData[setIdx][channelIdx]->m_values[frameIdx], channelSettings.m_range);
                    channelData.m_compressedData.emplace_back( nQuantizedVal );
                }
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------

    Resource::CompilationResult AnimationClipCompiler::ProcessEventsData( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::Animation const& rawAnimData, AnimationClipEventData& outEventData ) const
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
                    ctx.LogWarning( "Invalid animation event track (%s - %s) encountered!", track->GetName(), track->GetTypeName() );
                }
                else
                {
                    ctx.LogWarning( "Invalid animation event track (%s) encountered!", track->GetTypeName() );
                }

                // Skip invalid tracks
                continue;
            }

            //-------------------------------------------------------------------------

            if ( trackStatus == Timeline::Track::Status::HasWarnings )
            {
                if ( track->IsRenameable() )
                {
                    ctx.LogWarning( "Animation event track (Track: %s, Type: %s) has warnings: %s", track->GetName(), track->GetTypeName(), track->GetStatusMessage().c_str() );
                }
                else
                {
                    ctx.LogWarning( "Animation event track (%s) has warnings: %s", track->GetTypeName(), track->GetStatusMessage().c_str() );
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
                    ctx.LogWarning( "Animation event track (%s) has warnings: %s", track->GetTypeName(), track->GetStatusMessage().c_str() );
                    continue;
                }

                // Add event
                //-------------------------------------------------------------------------

                // Ignore any event entirely outside the animation time range
                FloatRange eventTimeRange = item->GetTimeRange();
                if ( !animationTimeRange.Overlaps( eventTimeRange ) )
                {
                    ctx.LogWarning( "Event detected outside animation time range, event will be ignored" );
                    continue;
                }

                // Clamp events that extend out of the animation time range to the animation time range
                if ( !animationTimeRange.ContainsInclusive( eventTimeRange ) )
                {
                    ctx.LogWarning( "Event extend outside the valid animation time range, event will be clamped to animation range" );

                    eventTimeRange.m_begin = animationTimeRange.GetClampedValue( eventTimeRange.m_begin );
                    eventTimeRange.m_end = animationTimeRange.GetClampedValue( eventTimeRange.m_end );
                    EE_ASSERT( eventTimeRange.IsSetAndValid() );
                }

                // Set event time and add new event
                TTypeInstance<Event>& eventInstance = events.emplace_back();
                eventInstance.CopyFrom( pEvent );

                if ( lastFrameIdx != 0 )
                {
                    eventInstance->m_startTime = ( eventTimeRange.m_begin / lastFrameIdx );
                    eventInstance->m_duration = ( eventTimeRange.GetLength() / lastFrameIdx );
                }
                else
                {
                    eventInstance->m_startTime = 0.0f;
                    eventInstance->m_duration = 0.0f;
                }

                // Create sync event
                //-------------------------------------------------------------------------

                if ( pEventTrack->IsSyncTrack() && numSyncTracks == 1 )
                {
                    Seconds const startTime = eventInstance->m_startTime.ToFloat() * clipDuration;

                    auto const iter = VectorFind( outEventData.m_syncEventMarkers, startTime, [] ( SyncTrack::EventMarker const& evt, Seconds startTime ) { return Math::IsNearEqual( evt.m_startTime.ToFloat(), startTime.ToFloat() ); } );
                    if ( iter != outEventData.m_syncEventMarkers.end() )
                    {
                        result = ctx.LogError( "Multiple sync events with the same start time detected!" );
                        return result;
                    }

                    outEventData.m_syncEventMarkers.emplace_back( Percentage( startTime.ToFloat() ), pEvent->GetSyncEventID() );
                }
            }
        }

        if ( numSyncTracks > 1 )
        {
            ctx.LogWarning( "Multiple sync tracks detected, using the first one encountered!" );
        }

        // Transfer sorted events
        //-------------------------------------------------------------------------

        auto sortPredicate = [] ( TTypeInstance<Event> const& eventA, TTypeInstance<Event> const& eventB )
        {
            return eventA->GetStartTime() < eventB->GetStartTime();
        };

        eastl::sort( events.begin(), events.end(), sortPredicate );

        for ( TTypeInstance<Event>& event : events )
        {
            outEventData.m_collection.m_descriptors.emplace_back( TypeSystem::TypeDescriptor( ctx.m_typeRegistry, event.Get() ) );
        }

        eastl::sort( outEventData.m_syncEventMarkers.begin(), outEventData.m_syncEventMarkers.end() );

        //-------------------------------------------------------------------------

        return result;
    }
}
#include "FbxAnimation.h"
#include "FbxSceneContext.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class FbxRawAnimation : public RawAnimation
    {
        friend class FbxAnimationFileReader;
    };

    //-------------------------------------------------------------------------

    class FbxAnimationFileReader
    {

    public:

        static TUniquePtr<EE::RawAssets::RawAnimation> ReadAnimation( FileSystem::Path const& sourceFilePath, RawSkeleton const& rawSkeleton, String const& animationName )
        {
            EE_ASSERT( sourceFilePath.IsValid() && rawSkeleton.IsValid() );

            TUniquePtr<RawAnimation> pAnimation( EE::New<RawAnimation>( rawSkeleton ) );
            FbxRawAnimation* pRawAnimation = (FbxRawAnimation*) pAnimation.get();

            Fbx::FbxSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                // Find the required anim stack
                //-------------------------------------------------------------------------

                FbxAnimStack* pAnimStack = nullptr;
                if ( !animationName.empty() )
                {
                    pAnimStack = sceneCtx.FindAnimStack( animationName.c_str() );
                    if ( pAnimStack == nullptr )
                    {
                        pRawAnimation->LogError( "Could not find requested animation (%s) in FBX scene", animationName.c_str() );
                        return pAnimation;
                    }
                }
                else // Take the first anim stack present
                {
                    auto const numAnimStacks = sceneCtx.m_pScene->GetSrcObjectCount<FbxAnimStack>();
                    if ( numAnimStacks == 0 )
                    {
                        pRawAnimation->LogError( "No animations found in the FBX scene", animationName.c_str() );
                        return pAnimation;
                    }

                    pAnimStack = sceneCtx.m_pScene->GetSrcObject<FbxAnimStack>( 0 );
                }

                EE_ASSERT( pAnimStack != nullptr );

                // Read animation data
                //-------------------------------------------------------------------------

                sceneCtx.m_pScene->SetCurrentAnimationStack( pAnimStack );

                // Read animation start and end times
                FbxTime duration;
                FbxTakeInfo const* pTakeInfo = sceneCtx.m_pScene->GetTakeInfo( pAnimStack->GetNameWithoutNameSpacePrefix() );
                if ( pTakeInfo != nullptr )
                {
                    duration = pTakeInfo->mLocalTimeSpan.GetDuration();

                    pRawAnimation->m_start = (float) pTakeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
                    pRawAnimation->m_end = (float) pTakeInfo->mLocalTimeSpan.GetStop().GetSecondDouble();
                    pRawAnimation->m_duration = (float) duration.GetSecondDouble();
                }
                else // Take the time line value
                {
                    FbxTimeSpan timeLineSpan;
                    sceneCtx.m_pScene->GetGlobalSettings().GetTimelineDefaultTimeSpan( timeLineSpan );
                    duration = timeLineSpan.GetDuration();

                    pRawAnimation->m_start = (float) timeLineSpan.GetStart().GetSecondDouble();
                    pRawAnimation->m_end = (float) timeLineSpan.GetStop().GetSecondDouble();
                    pRawAnimation->m_duration = (float) duration.GetSecondDouble();
                }

                // Calculate frame rate
                FbxTime::EMode mode = duration.GetGlobalTimeMode();
                double frameRate = duration.GetFrameRate( mode );

                // Set sampling rate and allocate memory
                pRawAnimation->m_samplingFrameRate = (float) duration.GetFrameRate( mode );
                float const samplingTimeStep = 1.0f / pRawAnimation->m_samplingFrameRate;
                pRawAnimation->m_numFrames = (uint32_t) Math::Round( pRawAnimation->GetDuration() / samplingTimeStep ) + 1;

                // Read animation data
                //-------------------------------------------------------------------------

                ReadTrackData( sceneCtx, *pRawAnimation );
            }
            else
            {
                pRawAnimation->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pAnimation;
        }

        static bool ReadTrackData( Fbx::FbxSceneContext const& sceneCtx, FbxRawAnimation& rawAnimation )
        {
            int32_t const numBones = rawAnimation.m_skeleton.GetNumBones();
            float const samplingTimeStep = 1.0f / rawAnimation.m_samplingFrameRate;
            uint32_t const maxKeys = rawAnimation.m_numFrames * 3;

            rawAnimation.m_tracks.resize( numBones );

            //-------------------------------------------------------------------------

            FbxAnimEvaluator* pEvaluator = sceneCtx.m_pScene->GetAnimationEvaluator();
            for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                StringID const& boneName = rawAnimation.m_skeleton.GetBoneName( boneIdx );
                RawAnimation::TrackData& animTrack = rawAnimation.m_tracks[boneIdx];
                FbxNode* pBoneNode = sceneCtx.m_pScene->FindNodeByName( boneName.c_str() );

                // Get the parent node for non-root bones
                FbxNode* pParentBoneNode = nullptr;
                if ( boneIdx != 0 )
                {
                    int32_t const parentBoneIdx = rawAnimation.m_skeleton.GetParentBoneIndex( boneIdx );
                    EE_ASSERT( parentBoneIdx != InvalidIndex );
                    StringID const parentBoneName = rawAnimation.m_skeleton.GetBoneName( parentBoneIdx );
                    pParentBoneNode = sceneCtx.m_pScene->FindNodeByName( parentBoneName.c_str() );
                    if ( pParentBoneNode == nullptr )
                    {
                        rawAnimation.LogError( "Error: Animation skeleton and animation data dont match. Skeleton bone (%s) not found in animation.", boneName.c_str() );
                        return false;
                    }
                }

                // Find a node that matches skeleton joint
                if ( pBoneNode == nullptr )
                {
                    rawAnimation.LogWarning( "Warning: No animation track found for bone (%s), Using skeleton bind pose instead.", boneName.c_str() );

                    // Store skeleton reference pose for bone
                    for ( auto i = 0u; i < rawAnimation.m_numFrames; i++ )
                    {
                        animTrack.m_localTransforms.emplace_back( rawAnimation.m_skeleton.GetLocalTransform( boneIdx ) );
                    }
                }
                else
                {
                    bool const isChildOfRoot = rawAnimation.m_skeleton.GetParentBoneIndex( boneIdx ) == 0;

                    // Reserve keys in animation tracks
                    animTrack.m_localTransforms.reserve( maxKeys );

                    // Sample animation data
                    float currentTime = rawAnimation.m_start;
                    for ( auto frameIdx = 0u; frameIdx < rawAnimation.m_numFrames; frameIdx++ )
                    {
                        FbxAMatrix nodeLocalTransform;

                        // Handle root separately as the root bone's global and local spaces are the same
                        if ( boneIdx == 0 )
                        {
                            nodeLocalTransform = pEvaluator->GetNodeGlobalTransform( pBoneNode, FbxTimeSeconds( currentTime ) );
                        }
                        else // Read the global transforms and convert to local
                        {
                            // Calculate local transform
                            FbxAMatrix const nodeParentGlobalTransform = pEvaluator->GetNodeGlobalTransform( pParentBoneNode, FbxTimeSeconds( currentTime ) );
                            FbxAMatrix const nodeGlobalTransform = pEvaluator->GetNodeGlobalTransform( pBoneNode, FbxTimeSeconds( currentTime ) );
                            nodeLocalTransform = nodeParentGlobalTransform.Inverse() * nodeGlobalTransform;
                        }

                        // Manually scale translation
                        Transform localTransform = sceneCtx.ConvertMatrixToTransform( nodeLocalTransform );
                        localTransform.SetTranslation( localTransform.GetTranslation() * sceneCtx.GetScaleConversionMultiplier() );
                        animTrack.m_localTransforms.emplace_back( localTransform );
                         
                        // Step time
                        currentTime += samplingTimeStep;
                    }
                }
            }

            return true;
        }
    };
}

//-------------------------------------------------------------------------

namespace EE::Fbx
{
    TUniquePtr<EE::RawAssets::RawAnimation> ReadAnimation( FileSystem::Path const& animationFilePath, RawAssets::RawSkeleton const& rawSkeleton, String const& takeName )
    {
        return RawAssets::FbxAnimationFileReader::ReadAnimation( animationFilePath, rawSkeleton, takeName );
    }
}
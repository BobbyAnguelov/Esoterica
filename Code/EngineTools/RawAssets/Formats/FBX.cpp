#include "Fbx.h"
#include "EngineTools/RawAssets/RawSkeleton.h"
#include "EngineTools/RawAssets/RawAnimation.h"
#include "EngineTools/RawAssets/RawMesh.h"

//-------------------------------------------------------------------------

using namespace fbxsdk;
using namespace EE::RawAssets;

//-------------------------------------------------------------------------
// Scene Context
//-------------------------------------------------------------------------

namespace EE::Fbx
{
    FbxSceneContext::FbxSceneContext( FileSystem::Path const& filePath, float additionalScalingFactor )
    {
        if ( !FileSystem::Exists( filePath ) )
        {
            m_error.sprintf( "Specified FBX file doesn't exist: %s", filePath.c_str() );
            return;
        }

        // Import the FBX file
        //-------------------------------------------------------------------------

        m_pManager = FbxManager::Create();

        auto pImporter = FbxImporter::Create( m_pManager, "EE Importer" );
        if ( !pImporter->Initialize( filePath, -1, m_pManager->GetIOSettings() ) )
        {
            m_error.sprintf( "Failed to load specified FBX file ( %s ) : %s", filePath.c_str(), pImporter->GetStatus().GetErrorString() );
            return;
        }

        m_pScene = FbxScene::Create( m_pManager, "ImportScene" );
        if ( !pImporter->Import( m_pScene ) )
        {
            m_error.sprintf( "Failed to load specified FBX file ( %s ) : %s", filePath.c_str(), pImporter->GetStatus().GetErrorString() );
            pImporter->Destroy();
            return;
        }
        pImporter->Destroy();

        // Check file format
        //-------------------------------------------------------------------------

        FILE* fp = nullptr;
        int32_t errcode = fopen_s( &fp, filePath.c_str(), "r" );
        EE_ASSERT( errcode == 0 );

        char cmpBuffer[19] = { 0 };
        fread( cmpBuffer, 1, 18, fp );
        fclose( fp );

        if ( strcmp( cmpBuffer, "Kaydara FBX Binary" ) != 0 )
        {
            m_warning.sprintf( "Reading ASCII FBX files will be slower than binary. It is highly recommended that you convert all FBX input files to binary!" );
        }

        // Co-ordinate system scene conversion
        //-------------------------------------------------------------------------

        auto& globalSettings = m_pScene->GetGlobalSettings();

        FbxAxisSystem const originalAxisSystem = globalSettings.GetAxisSystem();

        int32_t sign = 0;
        switch ( originalAxisSystem.GetUpVector( sign ) )
        {
            case FbxAxisSystem::eXAxis:
            {
                m_originalUpAxis = sign >= 0 ? Axis::X : Axis::NegX;
            }
            break;

            case FbxAxisSystem::eYAxis:
            {
                m_originalUpAxis = sign >= 0 ? Axis::Y : Axis::NegY;
            }
            break;

            case FbxAxisSystem::eZAxis:
            {
                m_originalUpAxis = sign >= 0 ? Axis::Z : Axis::NegZ;
            }
            break;
        }

        if ( originalAxisSystem != FbxAxisSystem::MayaZUp || originalAxisSystem != FbxAxisSystem::Max )
        {
            FbxAxisSystem::MayaZUp.DeepConvertScene( m_pScene );
        }

        // Unit conversion
        //-------------------------------------------------------------------------
        // DO NOT USE THE FBX UNIT CONVERSION FUNCTIONS AS THEY JUST INTRODUCE SCALES INTO ALL TRANSFORMS!

        FbxSystemUnit const originalSystemUnits = globalSettings.GetSystemUnit();
        m_scaleConversionMultiplier = (float) originalSystemUnits.GetConversionFactorTo( FbxSystemUnit::m );
        m_scaleConversionMultiplier *= additionalScalingFactor;
    }

    FbxSceneContext::~FbxSceneContext()
    {
        if ( m_pScene != nullptr ) m_pScene->Destroy();
        if ( m_pManager != nullptr ) m_pManager->Destroy();
    }

    //-------------------------------------------------------------------------

    static void FindAllNodes( FbxNode* pCurrentNode, FbxNodeAttribute::EType nodeType, TVector<FbxNode*>& results )
    {
        EE_ASSERT( pCurrentNode != nullptr );

        // Read node attribute
        //-------------------------------------------------------------------------

        FbxNodeAttribute::EType attribType = FbxNodeAttribute::EType::eUnknown;

        FbxNodeAttribute* pNodeAttribute = pCurrentNode->GetNodeAttribute();
        if ( pNodeAttribute != nullptr )
        {
            attribType = pNodeAttribute->GetAttributeType();
        }

        // Return node or continue search
        //-------------------------------------------------------------------------

        if ( attribType == nodeType )
        {
            results.emplace_back( pCurrentNode );
        }
        else // Search children
        {
            for ( auto i = 0; i < pCurrentNode->GetChildCount(); i++ )
            {
                FindAllNodes( pCurrentNode->GetChild( i ), nodeType, results );
            }
        }
    }

    void FbxSceneContext::FindAllNodesOfType( FbxNodeAttribute::EType nodeType, TVector<FbxNode*>& results ) const
    {
        EE_ASSERT( IsValid() );
        auto pRootNode = m_pScene->GetRootNode();
        FindAllNodes( pRootNode, nodeType, results );
    }

    static FbxNode* FindNodeByName( FbxNode* pCurrentNode, FbxNodeAttribute::EType nodeType, char const* pNodeNameToFind )
    {
        EE_ASSERT( pCurrentNode != nullptr && pNodeNameToFind != nullptr );

        // Read node attribute
        //-------------------------------------------------------------------------

        FbxNodeAttribute::EType attribType = FbxNodeAttribute::EType::eUnknown;

        FbxNodeAttribute* pNodeAttribute = pCurrentNode->GetNodeAttribute();
        if ( pNodeAttribute != nullptr )
        {
            attribType = pNodeAttribute->GetAttributeType();
        }

        // Return node or continue search
        //-------------------------------------------------------------------------

        if ( attribType == nodeType && pCurrentNode->GetNameWithoutNameSpacePrefix() == pNodeNameToFind )
        {
            return pCurrentNode;
        }
        else // Search children
        {
            for ( auto i = 0; i < pCurrentNode->GetChildCount(); i++ )
            {
                FbxNode* pResultNode = FindNodeByName( pCurrentNode->GetChild( i ), nodeType, pNodeNameToFind );
                if ( pResultNode != nullptr )
                {
                    return pResultNode;
                }
            }
        }

        return nullptr;
    }

    FbxNode* FbxSceneContext::FindNodeOfTypeByName( FbxNodeAttribute::EType nodeType, char const* pNodeNameToFind ) const
    {
        EE_ASSERT( IsValid() );
        auto pRootNode = m_pScene->GetRootNode();
        return FindNodeByName( pRootNode, nodeType, pNodeNameToFind );
    }

    //-------------------------------------------------------------------------

    void FbxSceneContext::FindAllAnimStacks( TVector<FbxAnimStack*>& results ) const
    {
        EE_ASSERT( IsValid() );

        auto const numAnimStacks = m_pScene->GetSrcObjectCount<FbxAnimStack>();
        for ( auto i = 0; i < numAnimStacks; i++ )
        {
            results.push_back( m_pScene->GetSrcObject<FbxAnimStack>( i ) );
        }
    }

    FbxAnimStack* FbxSceneContext::FindAnimStack( char const* pTakeName ) const
    {
        EE_ASSERT( IsValid() && pTakeName != nullptr );

        auto const numAnimStacks = m_pScene->GetSrcObjectCount<FbxAnimStack>();
        if ( numAnimStacks == 0 )
        {
            return nullptr;
        }

        for ( auto i = 0; i < numAnimStacks; i++ )
        {
            auto pAnimStack = m_pScene->GetSrcObject<FbxAnimStack>( i );
            if ( pAnimStack->GetNameWithoutNameSpacePrefix() == pTakeName )
            {
                return pAnimStack;
            }
        }

        return nullptr;
    }
}

//-------------------------------------------------------------------------
// Skeleton
//-------------------------------------------------------------------------

namespace EE::Fbx
{
    class FbxRawSkeleton : public RawSkeleton
    {
        friend class FbxSkeletonFileReader;
        friend class FbxAnimationFileReader;
        friend class FbxMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class FbxSkeletonFileReader
    {
    public:

        static TUniquePtr<RawSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawSkeleton> pSkeleton( EE::New<RawSkeleton>() );

            FbxRawSkeleton* pRawSkeleton = (FbxRawSkeleton*) pSkeleton.get();

            //-------------------------------------------------------------------------

            Fbx::FbxSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                if ( sceneCtx.HasWarningOccurred() )
                {
                    pRawSkeleton->LogWarning( sceneCtx.GetWarningMessage().c_str() );
                }

                ReadSkeleton( sceneCtx, skeletonRootBoneName, *pRawSkeleton );
            }
            else
            {
                pRawSkeleton->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pSkeleton;
        }

        static void ReadSkeleton( Fbx::FbxSceneContext const& sceneCtx, String const& skeletonRootBoneName, FbxRawSkeleton& rawSkeleton )
        {
            TVector<FbxNode*> skeletonRootNodes;
            sceneCtx.FindAllNodesOfType( FbxNodeAttribute::eSkeleton, skeletonRootNodes );

            auto const numSkeletons = skeletonRootNodes.size();
            if ( numSkeletons == 0 )
            {
                rawSkeleton.LogError( "No Skeletons found in FBX scene" );
                return;
            }

            FbxNode* pSkeletonToUse = nullptr;
            if ( !skeletonRootBoneName.empty() )
            {
                for ( auto& pSkeletonNode : skeletonRootNodes )
                {
                    // Check skeleton node name
                    if ( skeletonRootBoneName == Fbx::GetNameWithoutNamespace( pSkeletonNode ) )
                    {
                        pSkeletonToUse = pSkeletonNode;
                        break;
                    }

                    // Check null parents
                    if ( auto pParentNode = pSkeletonNode->GetParent() )
                    {
                        if ( auto pNodeAttribute = pParentNode->GetNodeAttribute() )
                        {
                            if ( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull )
                            {
                                pSkeletonToUse = pParentNode;
                                break;
                            }
                        }
                    }
                }

                if ( pSkeletonToUse == nullptr )
                {
                    rawSkeleton.LogError( "Couldn't find specified skeleton root: %s", skeletonRootBoneName.c_str() );
                    return;
                }
            }
            else
            {
                pSkeletonToUse = skeletonRootNodes[0];
            }

            // Finalize skeleton transforms
            //-------------------------------------------------------------------------

            rawSkeleton.m_name = StringID( Fbx::GetNameWithoutNamespace( pSkeletonToUse ) );
            ReadBoneHierarchy( rawSkeleton, sceneCtx, pSkeletonToUse, -1 );
            rawSkeleton.CalculateLocalTransforms();
        }

        static void ReadBoneHierarchy( FbxRawSkeleton& rawSkeleton, Fbx::FbxSceneContext const& sceneCtx, FbxNode* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr && ( pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton || pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eNull ) );

            auto const boneIdx = (int32_t) rawSkeleton.m_bones.size();
            rawSkeleton.m_bones.push_back( RawSkeleton::BoneData( Fbx::GetNameWithoutNamespace( pNode ) ) );
            rawSkeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Read Bone transform
            FbxAMatrix const nodeTransform = pNode->EvaluateGlobalTransform();
            rawSkeleton.m_bones[boneIdx].m_globalTransform = sceneCtx.ConvertMatrixToTransform( nodeTransform );
            rawSkeleton.m_bones[boneIdx].m_globalTransform.SetTranslation( rawSkeleton.m_bones[boneIdx].m_globalTransform.GetTranslation() * sceneCtx.GetScaleConversionMultiplier() );

            // Read child bones
            auto const numChildren = pNode->GetChildCount();
            for ( int i = 0; i < numChildren; i++ )
            {
                FbxNode* pChildNode = pNode->GetChild( i );
                auto const attributeType = pChildNode->GetNodeAttribute()->GetAttributeType();
                if ( attributeType == FbxNodeAttribute::eSkeleton ) // We only support a null root node, all children need to be bones
                {
                    ReadBoneHierarchy( rawSkeleton, sceneCtx, pChildNode, boneIdx );
                }
            }
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<RawSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
    {
        return FbxSkeletonFileReader::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
    }

    void ReadSkeleton( FbxSceneContext const& sceneCtx, String const& skeletonRootBoneName, RawSkeleton& skeleton )
    {
        return FbxSkeletonFileReader::ReadSkeleton( sceneCtx, skeletonRootBoneName, (FbxRawSkeleton&) skeleton );
    }
}

//-------------------------------------------------------------------------
// Animation
//-------------------------------------------------------------------------

namespace EE::Fbx
{
    class FbxRawAnimation : public RawAnimation
    {
        friend class FbxAnimationFileReader;
    };

    //-------------------------------------------------------------------------

    class FbxAnimationFileReader
    {

    public:

        static TUniquePtr<RawAnimation> ReadAnimation( FileSystem::Path const& sourceFilePath, RawSkeleton const& rawSkeleton, String const& animationName )
        {
            EE_ASSERT( sourceFilePath.IsValid() && rawSkeleton.IsValid() );

            TUniquePtr<RawAnimation> pAnimation( EE::New<RawAnimation>( rawSkeleton ) );
            FbxRawAnimation* pRawAnimation = (FbxRawAnimation*) pAnimation.get();

            Fbx::FbxSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                if ( sceneCtx.HasWarningOccurred() )
                {
                    pRawAnimation->LogWarning( sceneCtx.GetWarningMessage().c_str() );
                }

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
                float samplingStartTime = 0;
                FbxTime duration;
                FbxTakeInfo const* pTakeInfo = sceneCtx.m_pScene->GetTakeInfo( Fbx::GetNameWithoutNamespace( pAnimStack ) );
                if ( pTakeInfo != nullptr )
                {
                    samplingStartTime = (float) pTakeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
                    duration = pTakeInfo->mLocalTimeSpan.GetDuration();
                    pRawAnimation->m_duration = (float) duration.GetSecondDouble();
                }
                else // Take the time line value
                {
                    FbxTimeSpan timeLineSpan;
                    sceneCtx.m_pScene->GetGlobalSettings().GetTimelineDefaultTimeSpan( timeLineSpan );

                    samplingStartTime = (float) timeLineSpan.GetStart().GetSecondDouble();
                    duration = timeLineSpan.GetDuration();
                    pRawAnimation->m_duration = (float) duration.GetSecondDouble();
                }

                // Calculate frame rate
                FbxTime::EMode mode = duration.GetGlobalTimeMode();

                // Set sampling rate and allocate memory
                pRawAnimation->m_samplingFrameRate = (float) duration.GetFrameRate( mode );
                float const samplingTimeStep = 1.0f / pRawAnimation->m_samplingFrameRate;
                pRawAnimation->m_numFrames = (uint32_t) Math::Round( pRawAnimation->GetDuration() / samplingTimeStep ) + 1;

                // Read animation data
                //-------------------------------------------------------------------------

                TVector<FbxNode*> boneToNodeMapping;
                if ( GenerateNodeMappings( sceneCtx, *pRawAnimation, boneToNodeMapping ) )
                {
                    ReadTrackData( sceneCtx, *pRawAnimation, boneToNodeMapping, samplingStartTime );
                }
            }
            else
            {
                pRawAnimation->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pAnimation;
        }

        static bool GenerateNodeMappings( Fbx::FbxSceneContext const& sceneCtx, FbxRawAnimation& rawAnimation, TVector<FbxNode*>& outBoneToNodeMapping )
        {
            struct NodeInfo
            {
                NodeInfo() = default;

                NodeInfo( FbxNode* pNode )
                    : m_pNode( pNode )
                {
                    m_nameWithoutNamespace = Fbx::GetNameWithoutNamespace( m_pNode );
                }

                FbxNode*    m_pNode = nullptr;
                String      m_nameWithoutNamespace;
            };

            //-------------------------------------------------------------------------

            String missingBonesStr;

            TVector<NodeInfo> nodes;
            nodes.reserve( sceneCtx.m_pScene->GetNodeCount() );
            for ( int32_t n = 0; n < sceneCtx.m_pScene->GetNodeCount(); n++ )
            {
                nodes.emplace_back( sceneCtx.m_pScene->GetNode( n ) );
            }

            //-------------------------------------------------------------------------

            int32_t const numBones = rawAnimation.m_skeleton.GetNumBones();
            outBoneToNodeMapping.resize( numBones, nullptr );

            for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                StringID const& boneName = rawAnimation.m_skeleton.GetBoneName( boneIdx );

                for ( auto const& nodeInfo : nodes )
                {
                    if ( nodeInfo.m_nameWithoutNamespace == boneName.c_str() )
                    {
                        outBoneToNodeMapping[boneIdx] = nodeInfo.m_pNode;
                        break;
                    }
                }

                if ( outBoneToNodeMapping[boneIdx] == nullptr )
                {
                    missingBonesStr.append_sprintf( "%s, ", boneName.c_str() );
                }
            }

            //-------------------------------------------------------------------------

            if ( !missingBonesStr.empty() )
            {
                missingBonesStr = missingBonesStr.substr( 0, missingBonesStr.length() - 2 );
                rawAnimation.LogError( "Failed to find tracks for bones: %s", missingBonesStr.c_str() );
                return false;
            }

            return true;
        }

        static bool ReadTrackData( Fbx::FbxSceneContext const& sceneCtx, FbxRawAnimation& rawAnimation, TVector<FbxNode*> const& boneToNodeMapping, float samplingStartTime )
        {
            int32_t const numBones = rawAnimation.m_skeleton.GetNumBones();
            float const samplingTimeStep = 1.0f / rawAnimation.m_samplingFrameRate;
            uint32_t const maxKeys = rawAnimation.m_numFrames * 3;

            rawAnimation.m_tracks.resize( numBones );

            //-------------------------------------------------------------------------

            FbxAnimEvaluator* pEvaluator = sceneCtx.m_pScene->GetAnimationEvaluator();
            for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                RawAnimation::TrackData& animTrack = rawAnimation.m_tracks[boneIdx];
                FbxNode* pBoneNode = boneToNodeMapping[boneIdx];

                // Get the parent node for non-root bones
                FbxNode* pParentBoneNode = nullptr;
                if ( boneIdx != 0 )
                {
                    int32_t const parentBoneIdx = rawAnimation.m_skeleton.GetParentBoneIndex( boneIdx );
                    EE_ASSERT( parentBoneIdx != InvalidIndex );
                    pParentBoneNode = boneToNodeMapping[parentBoneIdx];
                }

                // Find a node that matches skeleton joint
                if ( pBoneNode == nullptr )
                {
                    StringID const& boneName = rawAnimation.m_skeleton.GetBoneName( boneIdx );
                    rawAnimation.LogWarning( "Warning: No animation track found for bone (%s), Using skeleton bind pose instead.", boneName.c_str() );

                    // Store skeleton reference pose for bone
                    for ( auto i = 0; i < rawAnimation.m_numFrames; i++ )
                    {
                        animTrack.m_localTransforms.emplace_back( rawAnimation.m_skeleton.GetLocalTransform( boneIdx ) );
                    }
                }
                else
                {
                    // Reserve keys in animation tracks
                    animTrack.m_localTransforms.reserve( maxKeys );

                    // Sample animation data
                    float currentTime = samplingStartTime;
                    for ( auto frameIdx = 0; frameIdx < rawAnimation.m_numFrames; frameIdx++ )
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

    //-------------------------------------------------------------------------

    TUniquePtr<RawAnimation> ReadAnimation( FileSystem::Path const& animationFilePath, RawSkeleton const& rawSkeleton, String const& takeName )
    {
        return FbxAnimationFileReader::ReadAnimation( animationFilePath, rawSkeleton, takeName );
    }
}

//-------------------------------------------------------------------------
// Mesh
//-------------------------------------------------------------------------

namespace EE::Fbx
{
    class FbxRawMesh : public RawMesh
    {
        friend class FbxMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class FbxMeshFileReader
    {
    public:

        template<typename ElementType, typename ElementDataType>
        static ElementDataType GetElementData( ElementType* pElement, int32_t const ctrlPointIdx, int32_t const vertexIdx )
        {
            ElementDataType data;
            switch ( pElement->GetMappingMode() )
            {
                case FbxGeometryElement::eByControlPoint:
                {
                    switch ( pElement->GetReferenceMode() )
                    {
                        case FbxGeometryElement::eDirect:
                        {
                            data = pElement->GetDirectArray().GetAt( ctrlPointIdx );
                        }
                        break;

                        case FbxGeometryElement::eIndexToDirect:
                        {
                            int32_t const dataElementIdx = pElement->GetIndexArray().GetAt( ctrlPointIdx );
                            data = pElement->GetDirectArray().GetAt( dataElementIdx );
                        }
                        break;

                        default:
                        break;
                    }
                }
                break;

                case FbxGeometryElement::eByPolygonVertex:
                {
                    switch ( pElement->GetReferenceMode() )
                    {
                        case FbxGeometryElement::eDirect:
                        {
                            data = pElement->GetDirectArray().GetAt( vertexIdx );
                        }
                        break;

                        case FbxGeometryElement::eIndexToDirect:
                        {
                            int32_t const dataElementIdx = pElement->GetIndexArray().GetAt( vertexIdx );
                            data = pElement->GetDirectArray().GetAt( dataElementIdx );
                        }
                        break;

                        default:
                        break;
                    }
                }
                break;

                default:
                break;
            }

            return data;
        }

        //-------------------------------------------------------------------------

        static TUniquePtr<RawMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawMesh> pMesh( EE::New<RawMesh>() );
            FbxRawMesh* pRawMesh = (FbxRawMesh*) pMesh.get();

            //-------------------------------------------------------------------------

            Fbx::FbxSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                if ( sceneCtx.HasWarningOccurred() )
                {
                    pRawMesh->LogWarning( sceneCtx.GetWarningMessage().c_str() );
                }

                ReadAllSubmeshes( *pRawMesh, sceneCtx, meshesToInclude );
            }
            else
            {
                pRawMesh->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pMesh;
        }

        static TUniquePtr<RawMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences = 4 )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawMesh> pMesh( EE::New<RawMesh>() );
            FbxRawMesh* pRawMesh = (FbxRawMesh*) pMesh.get();

            //-------------------------------------------------------------------------

            Fbx::FbxSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                if ( sceneCtx.HasWarningOccurred() )
                {
                    pRawMesh->LogWarning( sceneCtx.GetWarningMessage().c_str() );
                }

                pRawMesh->m_maxNumberOfBoneInfluences = maxBoneInfluences;
                pRawMesh->m_isSkeletalMesh = true;

                // Read mesh data
                //-------------------------------------------------------------------------

                ReadAllSubmeshes( *pRawMesh, sceneCtx, meshesToInclude );

                if ( pRawMesh->HasErrors() )
                {
                    return pMesh;
                }

                // Since the bind pose is in global space, calculate the local space transforms for the skeleton
                FbxRawSkeleton& rawSkeleton = (FbxRawSkeleton&) pRawMesh->m_skeleton;
                rawSkeleton.CalculateLocalTransforms();
            }
            else
            {
                pRawMesh->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pMesh;
        }

        //-------------------------------------------------------------------------

        static void ReadAllSubmeshes( FbxRawMesh& rawMesh, Fbx::FbxSceneContext const& sceneCtx, TVector<String> const& meshesToInclude )
        {
            FbxGeometryConverter geomConverter( sceneCtx.m_pManager );
            geomConverter.SplitMeshesPerMaterial( sceneCtx.m_pScene, true );

            // Find all meshes in the sceneCtx
            //-------------------------------------------------------------------------

            struct FoundMesh
            {
                FbxMesh* m_pMesh;
                bool isSkinned;
            };

            TInlineVector<FoundMesh, 20> meshes;
            auto numGeometries = sceneCtx.m_pScene->GetGeometryCount();

            if ( numGeometries == 0 )
            {
                rawMesh.m_errors.push_back( "No meshes present in the sceneCtx" );
                return;
            }

            for ( auto i = 0; i < numGeometries; i++ )
            {
                auto pGeometry = sceneCtx.m_pScene->GetGeometry( i );
                if ( pGeometry->Is<FbxMesh>() )
                {
                    FbxMesh* pMesh = static_cast<FbxMesh*>( pGeometry );
                    meshes.push_back( { pMesh, pMesh->GetDeformerCount( FbxDeformer::eSkin ) > 0 } );
                }
            }

            // For each mesh found perform necessary corrections and read mesh data
            // Note: this needs to be done in two passes since these operations reorder the geometries in the sceneCtx and pScene->GetGeometry( x ) doesnt return what you expect
            bool meshFound = false;
            TVector<TVector<uint32_t>> controlPointVertexMapping;
            for ( auto foundMesh : meshes )
            {
                if ( rawMesh.m_isSkeletalMesh && !foundMesh.isSkinned )
                {
                    continue;
                }

                auto pMesh = foundMesh.m_pMesh;

                // Only process specified meshes
                if ( !meshesToInclude.empty() )
                {
                    FbxString const meshName = Fbx::GetNameWithoutNamespace( pMesh->GetNode() );
                    if ( !VectorContains( meshesToInclude, meshName ) )
                    {
                        continue;
                    }
                }

                //-------------------------------------------------------------------------

                // Prepare the mesh for reading
                meshFound = true;
                if ( !PrepareMesh( sceneCtx, geomConverter, pMesh, rawMesh ) )
                {
                    return;
                }

                // Allocate space for the control point mapping
                controlPointVertexMapping.clear();
                controlPointVertexMapping.resize( pMesh->GetControlPointsCount() );

                // Create new geometry section
                RawMesh::GeometrySection& meshData = rawMesh.m_geometrySections.emplace_back( RawMesh::GeometrySection() );
                meshData.m_name = Fbx::GetNameWithoutNamespace( pMesh->GetNode() );
                meshData.m_numUVChannels = pMesh->GetElementUVCount();

                if ( !ReadMeshData( sceneCtx, pMesh, meshData, controlPointVertexMapping ) )
                {
                    return;
                }

                if ( rawMesh.m_isSkeletalMesh )
                {
                    if ( !ReadSkinningData( rawMesh, sceneCtx, pMesh, meshData, controlPointVertexMapping ) )
                    {
                        return;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( !meshFound )
            {
                InlineString meshNames;
                for ( auto const& meshName : meshesToInclude )
                {
                    meshNames.append_sprintf( "%s, ", meshName.c_str() );
                }
                meshNames = meshNames.substr( 0, meshNames.length() - 2 );

                rawMesh.m_errors.push_back( String( String::CtorSprintf(), "Couldn't find any specified meshes: %s", meshNames.c_str() ) );
            }
        }

        //-------------------------------------------------------------------------

        static bool PrepareMesh( Fbx::FbxSceneContext const& sceneCtx, FbxGeometryConverter& geomConverter, fbxsdk::FbxMesh*& pMesh, FbxRawMesh& rawMesh )
        {
            pMesh->RemoveBadPolygons();

            // Ensure that the mesh is triangulated
            if ( !pMesh->IsTriangleMesh() )
            {
                pMesh = (fbxsdk::FbxMesh*) geomConverter.Triangulate( pMesh, true );
                EE_ASSERT( pMesh != nullptr && pMesh->IsTriangleMesh() );
            }

            // Generate normals if they're not available or not in the right format
            bool const hasNormals = pMesh->GetElementNormalCount() > 0;
            bool const hasPerVertexNormals = hasNormals && pMesh->GetElementNormal()->GetMappingMode() == FbxLayerElement::eByPolygonVertex;
            if ( !hasPerVertexNormals )
            {
                if ( !pMesh->GenerateNormals( true, false, false ) )
                {
                    rawMesh.m_errors.push_back( String( String::CtorSprintf(), "Failed to regenerate mesh normals for mesh: %s", Fbx::GetNameWithoutNamespace( pMesh ) ) );
                    return false;
                }
            }

            // Generate smoothing groups if they doesnt exist
            if ( pMesh->GetElementSmoothingCount() == 0 )
            {
                geomConverter.ComputeEdgeSmoothingFromNormals( pMesh );
                geomConverter.ComputePolygonSmoothingFromEdgeSmoothing( pMesh );
            }

            EE_ASSERT( pMesh->GetElementNormal()->GetMappingMode() == FbxLayerElement::eByPolygonVertex );
            return true;
        }

        static bool ReadMeshData( Fbx::FbxSceneContext const& sceneCtx, fbxsdk::FbxMesh* pMesh, FbxRawMesh::GeometrySection& geometryData, TVector<TVector<uint32_t>>& controlPointVertexMapping )
        {
            EE_ASSERT( pMesh != nullptr && pMesh->IsTriangleMesh() );

            FbxAMatrix const meshNodeGlobalTransform = sceneCtx.GetNodeGlobalTransform( pMesh->GetNode() );

            //-------------------------------------------------------------------------

            // Check winding order - TODO: actually figure out how to get the real value, right now we assume CCW
            geometryData.m_clockwiseWinding = false;
            FbxVector4 const meshScale = meshNodeGlobalTransform.GetS();
            uint32_t const numNegativeAxes = ( ( meshScale[0] < 0 ) ? 1 : 0 ) + ( ( meshScale[1] < 0 ) ? 1 : 0 ) + ( ( meshScale[2] < 0 ) ? 1 : 0 );
            if ( numNegativeAxes == 1 || numNegativeAxes == 3 )
            {
                geometryData.m_clockwiseWinding = !geometryData.m_clockwiseWinding;
            }

            // Reserve memory for mesh data
            int32_t const numPolygons = pMesh->GetPolygonCount();
            int32_t const numVertices = numPolygons * 3;
            geometryData.m_vertices.reserve( numVertices );

            for ( int32_t polygonIdx = 0; polygonIdx < numPolygons; polygonIdx++ )
            {
                for ( int32_t vertexIdx = 0; vertexIdx < 3; vertexIdx++ )
                {
                    RawMesh::VertexData vert;

                    // Get vertex position
                    //-------------------------------------------------------------------------

                    int32_t const ctrlPointIdx = pMesh->GetPolygonVertex( polygonIdx, vertexIdx );
                    FbxVector4 const meshVertex = meshNodeGlobalTransform.MultT( pMesh->GetControlPoints()[ctrlPointIdx] );
                    vert.m_position = sceneCtx.ConvertVector3AndFixScale( meshVertex );
                    vert.m_position.m_w = 1.0f;

                    // Get vertex color
                    //-------------------------------------------------------------------------

                    FbxLayerElementVertexColor* pColorElement = pMesh->GetElementVertexColor();
                    if ( pColorElement != nullptr )
                    {
                        FbxColor const color = GetElementData<FbxLayerElementVertexColor, FbxColor>( pColorElement, ctrlPointIdx, vertexIdx );
                        vert.m_color = Float4( (float) color.mRed, (float) color.mGreen, (float) color.mBlue, (float) color.mAlpha );
                    }

                    // Get vertex normal
                    //-------------------------------------------------------------------------

                    EE_ASSERT( pMesh->GetElementNormal() != nullptr );
                    FbxVector4 meshNormal;
                    pMesh->GetPolygonVertexNormal( polygonIdx, vertexIdx, meshNormal );
                    vert.m_normal = sceneCtx.ConvertVector3( meshNormal ).GetNormalized3();

                    // Get vertex tangent and bi-normals
                    //-------------------------------------------------------------------------

                    FbxGeometryElementTangent* pTangentElement = pMesh->GetElementTangent();
                    if ( pTangentElement != nullptr )
                    {
                        FbxVector4 const tangent = GetElementData<FbxGeometryElementTangent, FbxVector4>( pTangentElement, ctrlPointIdx, vertexIdx );
                        vert.m_tangent = sceneCtx.ConvertVector3( tangent ).GetNormalized3();
                    }

                    FbxGeometryElementBinormal* pBinormalElement = pMesh->GetElementBinormal();
                    if ( pBinormalElement != nullptr )
                    {
                        FbxVector4 const binormal = GetElementData<FbxGeometryElementBinormal, FbxVector4>( pBinormalElement, ctrlPointIdx, vertexIdx );
                        vert.m_binormal = sceneCtx.ConvertVector3( binormal ).GetNormalized3();
                    }

                    // Get vertex UV
                    //-------------------------------------------------------------------------

                    int32_t const numUVChannelsForMeshSection = pMesh->GetElementUVCount();
                    for ( auto i = 0; i < numUVChannelsForMeshSection; ++i )
                    {
                        FbxGeometryElementUV* pTexcoordElement = pMesh->GetElementUV( i );
                        FbxVector2 texCoord( 0, 0 );

                        switch ( pTexcoordElement->GetMappingMode() )
                        {
                            case FbxGeometryElement::eByControlPoint:
                            {
                                switch ( pTexcoordElement->GetReferenceMode() )
                                {
                                    case FbxLayerElementUV::eDirect:
                                    {
                                        texCoord = pTexcoordElement->GetDirectArray().GetAt( ctrlPointIdx );
                                    }
                                    break;

                                    case FbxLayerElementUV::eIndexToDirect:
                                    {
                                        int32_t const texCoordIdx = pTexcoordElement->GetIndexArray().GetAt( ctrlPointIdx );
                                        texCoord = pTexcoordElement->GetDirectArray().GetAt( texCoordIdx );
                                    }
                                    break;

                                    default:
                                    break;
                                }
                            }
                            break;

                            // This is special for UVs
                            case FbxGeometryElement::eByPolygonVertex:
                            {
                                switch ( pTexcoordElement->GetReferenceMode() )
                                {
                                    case FbxLayerElementUV::eDirect:
                                    case FbxLayerElementUV::eIndexToDirect:
                                    {
                                        int32_t const textureUVIndex = pMesh->GetTextureUVIndex( polygonIdx, vertexIdx );
                                        texCoord = pTexcoordElement->GetDirectArray().GetAt( textureUVIndex );
                                    }
                                    break;

                                    default:
                                    break;
                                }
                            }
                            break;

                            //-------------------------------------------------------------------------

                            default:
                            break;
                        }

                        vert.m_texCoords.push_back( Float2( (float) texCoord[0], 1.0f - (float) texCoord[1] ) );
                    }

                    // Ensure we always have UV's set
                    if ( vert.m_texCoords.empty() )
                    {
                        vert.m_texCoords.emplace_back( Float2::Zero );
                    }

                    // Add vertex to mesh data
                    //-------------------------------------------------------------------------

                    // Check whether we are referencing a new vertex or should use the index to an existing one?
                    uint32_t existingVertexIdx = (uint32_t) InvalidIndex;
                    auto& vertexIndices = controlPointVertexMapping[ctrlPointIdx];
                    for ( auto const& idx : vertexIndices )
                    {
                        if ( vert == geometryData.m_vertices[idx] )
                        {
                            existingVertexIdx = idx;
                            break;
                        }
                    }

                    // The vertex already exists, so just add its index
                    if ( existingVertexIdx != (uint32_t) InvalidIndex )
                    {
                        EE_ASSERT( existingVertexIdx < geometryData.m_vertices.size() );
                        geometryData.m_indices.push_back( existingVertexIdx );
                    }
                    else // Create new vertex
                    {
                        geometryData.m_indices.push_back( (uint32_t) geometryData.m_vertices.size() );
                        vertexIndices.push_back( (uint32_t) geometryData.m_vertices.size() );
                        geometryData.m_vertices.push_back( vert );
                    }
                }
            }

            return true;
        }

        //-------------------------------------------------------------------------

        static FbxNode* FindSkeletonForSkin( FbxSkin* pSkin )
        {
            FbxNode* pSkeletonRootNode = nullptr;

            //-------------------------------------------------------------------------

            FbxCluster* pCluster = pSkin->GetCluster( 0 );
            FbxNode* pBoneNode = pCluster->GetLink();

            while ( pBoneNode != nullptr )
            {
                // Traverse hierarchy to find the root of the skeleton
                FbxNodeAttribute* pNodeAttribute = pBoneNode->GetNodeAttribute();
                if ( pNodeAttribute != nullptr && pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton )
                {
                    pSkeletonRootNode = pBoneNode;
                    pBoneNode = pBoneNode->GetParent();
                }
                else
                {
                    pBoneNode = nullptr;
                }
            }

            // If the root of the skeleton has a null root, make that the root as this is a common practice
            // TODO: should we make this a setting in the descriptor?
            if ( auto pParentNode = pSkeletonRootNode->GetParent() )
            {
                if ( auto pNodeAttribute = pParentNode->GetNodeAttribute() )
                {
                    if ( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull )
                    {
                        pSkeletonRootNode = pParentNode;
                    }
                }
            }

            return pSkeletonRootNode;
        }

        static bool ReadSkinningData( FbxRawMesh& rawMesh, Fbx::FbxSceneContext const& sceneCtx, fbxsdk::FbxMesh* pMesh, RawMesh::GeometrySection& geometryData, TVector<TVector<uint32_t>>& controlPointVertexMapping )
        {
            EE_ASSERT( pMesh != nullptr && pMesh->IsTriangleMesh() && rawMesh.m_isSkeletalMesh );

            // Check whether skinning data exists for this mesh
            int32_t const numSkins = pMesh->GetDeformerCount( FbxDeformer::eSkin );
            if ( numSkins == 0 )
            {
                rawMesh.LogError( "No skin data found!" );
                return false;
            }

            if ( numSkins > 1 )
            {
                rawMesh.LogWarning( "More than one skin found for mesh (%s), compiling only the first skin found!", geometryData.m_name.c_str() );
            }

            // Read skin
            //-------------------------------------------------------------------------

            FbxSkin* pSkin = static_cast<FbxSkin*>( pMesh->GetDeformer( 0, FbxDeformer::eSkin ) );
            FbxSkin::EType const skinningType = pSkin->GetSkinningType();
            if ( skinningType != FbxSkin::eRigid && skinningType != FbxSkin::eLinear )
            {
                rawMesh.LogError( "Unsupported skinning type for mesh (%s), only rigid and linear skinning supported!", geometryData.m_name.c_str() );
                return false;
            }

            // Read skeleton
            //-------------------------------------------------------------------------

            auto pSkeletonNode = FindSkeletonForSkin( pSkin );
            if ( pSkeletonNode == nullptr )
            {
                rawMesh.LogError( "Couldn't find skeleton for mesh!", geometryData.m_name.c_str() );
                return false;
            }

            // Check if we have already read a skeleton for this mesh and if the skeleton matches
            if ( rawMesh.m_skeleton.IsValid() )
            {
                StringID const skeletonRootName( Fbx::GetNameWithoutNamespace( pSkeletonNode ) );
                if ( rawMesh.GetSkeleton().GetRootBoneName() != skeletonRootName )
                {
                    rawMesh.LogError( "Different skeletons detected for the various sub-meshes. Expected: %s, Got: %s", rawMesh.GetSkeleton().GetRootBoneName().c_str(), skeletonRootName.c_str() );
                    return false;
                }
            }
            else // Try to read the skeleton
            {
                Fbx::ReadSkeleton( sceneCtx, Fbx::GetNameWithoutNamespace( pSkeletonNode ), rawMesh.m_skeleton );
                if ( !rawMesh.m_skeleton.IsValid() )
                {
                    rawMesh.LogError( "Failed to read mesh skeleton", geometryData.m_name.c_str() );
                    return false;
                }
            }

            // Read skinning cluster data (i.e. bone skin mapping)
            //-------------------------------------------------------------------------

            FbxRawSkeleton& rawSkeleton = static_cast<FbxRawSkeleton&>( rawMesh.m_skeleton );

            auto const numClusters = pSkin->GetClusterCount();
            for ( auto c = 0; c < numClusters; c++ )
            {
                FbxCluster* pCluster = pSkin->GetCluster( c );

                FbxNode* pClusterNode = pCluster->GetLink();
                if ( pClusterNode == nullptr )
                {
                    rawMesh.LogError( "No node linked to cluster: %s", pCluster->GetName() );
                    return false;
                }

                StringID const clusterNodeName( Fbx::GetNameWithoutNamespace( pClusterNode ) );
                FbxCluster::ELinkMode const mode = pCluster->GetLinkMode();
                if ( mode == FbxCluster::eAdditive )
                {
                    rawMesh.LogError( "Unsupported link mode for joint: %s", clusterNodeName.c_str() );
                    return false;
                }

                // Find bone in skeleton that matches this cluster name
                int32_t const boneIdx = rawMesh.m_skeleton.GetBoneIndex( clusterNodeName );
                if ( boneIdx == InvalidIndex )
                {
                    rawMesh.LogError( "Unknown bone link node encountered in FBX scene (%s)", clusterNodeName.c_str() );
                    return false;
                }

                //-------------------------------------------------------------------------

                // Read bind pose
                FbxAMatrix clusterLinkTransform;
                pCluster->GetTransformLinkMatrix( clusterLinkTransform );
                rawSkeleton.m_bones[boneIdx].m_globalTransform = sceneCtx.ConvertMatrixToTransform( clusterLinkTransform );

                // Manually scale translation
                rawSkeleton.m_bones[boneIdx].m_globalTransform.SetTranslation( rawSkeleton.m_bones[boneIdx].m_globalTransform.GetTranslation() * sceneCtx.GetScaleConversionMultiplier() );

                // Add bone indices and weight to all influenced vertices
                auto const* pControlPointIndices = pCluster->GetControlPointIndices();
                auto const* pControlPointWeights = pCluster->GetControlPointWeights();
                auto const numControlPointIndices = pCluster->GetControlPointIndicesCount();

                for ( auto i = 0; i < numControlPointIndices; i++ )
                {
                    EE_ASSERT( pControlPointIndices[i] < pMesh->GetControlPointsCount() );
                    if ( pControlPointWeights[i] > 0.f )
                    {
                        auto const& vertexIndices = controlPointVertexMapping[pControlPointIndices[i]];
                        for ( auto v = 0; v < vertexIndices.size(); v++ )
                        {
                            auto const vertexIdx = vertexIndices[v];
                            geometryData.m_vertices[vertexIdx].m_boneIndices.push_back( boneIdx );
                            geometryData.m_vertices[vertexIdx].m_boneWeights.push_back( (float) pControlPointWeights[i] );
                        }
                    }
                }
            }

            // Ensure we have <= the max number of skinning influences per vertex
            //-------------------------------------------------------------------------

            bool vertexInfluencesReduced = false;
            auto const numVerts = geometryData.m_vertices.size();
            for ( auto v = 0; v < numVerts; v++ )
            {
                auto& vert = geometryData.m_vertices[v];
                auto numInfluences = vert.m_boneIndices.size();

                if ( numInfluences > rawMesh.m_maxNumberOfBoneInfluences )
                {
                    while ( vert.m_boneIndices.size() > rawMesh.m_maxNumberOfBoneInfluences )
                    {
                        // Remove lowest influence
                        int32_t smallestWeightIdx = 0;
                        for ( auto f = 1; f < vert.m_boneIndices.size(); f++ )
                        {
                            if ( vert.m_boneWeights[f] < vert.m_boneWeights[smallestWeightIdx] )
                            {
                                smallestWeightIdx = f;
                            }
                        }

                        vert.m_boneWeights.erase( vert.m_boneWeights.begin() + smallestWeightIdx );
                        vert.m_boneIndices.erase( vert.m_boneIndices.begin() + smallestWeightIdx );
                    }

                    vertexInfluencesReduced = true;

                    // Re-normalize weights
                    float const totalWeight = vert.m_boneWeights[0] + vert.m_boneWeights[1] + vert.m_boneWeights[2] + vert.m_boneWeights[3];
                    for ( auto i = 1; i < rawMesh.m_maxNumberOfBoneInfluences; i++ )
                    {
                        vert.m_boneWeights[i] /= totalWeight;
                    }
                }
            }

            if ( vertexInfluencesReduced )
            {
                rawMesh.LogWarning( "More than %d skinning influences detected per bone for mesh (%s), this is not supported - influences have been reduced to %d", rawMesh.m_maxNumberOfBoneInfluences, geometryData.m_name.c_str(), rawMesh.m_maxNumberOfBoneInfluences );
            }

            //-------------------------------------------------------------------------

            rawSkeleton.CalculateLocalTransforms();
            return true;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<RawMesh> Fbx::ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
    {
        return FbxMeshFileReader::ReadStaticMesh( sourceFilePath, meshesToInclude );
    }

    TUniquePtr<RawMesh> Fbx::ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences )
    {
        return FbxMeshFileReader::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
    }
}
#include "FbxSkeleton.h"
#include "FbxSceneContext.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class FbxRawSkeleton : public RawSkeleton
    {
        friend class FbxSkeletonFileReader;
    };

    //-------------------------------------------------------------------------

    class FbxSkeletonFileReader
    {
    public:

        static TUniquePtr<EE::RawAssets::RawSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawSkeleton> pSkeleton( EE::New<RawSkeleton>() );

            FbxRawSkeleton* pRawSkeleton = (FbxRawSkeleton*) pSkeleton.get();

            //-------------------------------------------------------------------------

            Fbx::FbxSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
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
                    if ( pSkeletonNode->GetNameWithoutNameSpacePrefix() == skeletonRootBoneName )
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

            rawSkeleton.m_name = StringID( (char const*) pSkeletonToUse->GetNameWithoutNameSpacePrefix() );
            ReadBoneHierarchy( rawSkeleton, sceneCtx, pSkeletonToUse, -1 );
            rawSkeleton.CalculateLocalTransforms();
        }

        static void ReadBoneHierarchy( FbxRawSkeleton& rawSkeleton, Fbx::FbxSceneContext const& sceneCtx, fbxsdk::FbxNode* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr && ( pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton || pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eNull ) );

            auto const boneIdx = (int32_t) rawSkeleton.m_bones.size();
            rawSkeleton.m_bones.push_back( RawSkeleton::BoneData( (char const*) pNode->GetNameWithoutNameSpacePrefix() ) );
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
}

//-------------------------------------------------------------------------

namespace EE
{
    TUniquePtr<EE::RawAssets::RawSkeleton> Fbx::ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
    {
        return RawAssets::FbxSkeletonFileReader::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
    }

    void Fbx::ReadSkeleton( FbxSceneContext const& sceneCtx, String const& skeletonRootBoneName, RawAssets::RawSkeleton& skeleton )
    {
        return RawAssets::FbxSkeletonFileReader::ReadSkeleton( sceneCtx, skeletonRootBoneName, (RawAssets::FbxRawSkeleton&) skeleton );
    }
}
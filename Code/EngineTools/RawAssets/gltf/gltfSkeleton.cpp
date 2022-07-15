#include "gltfSkeleton.h"
#include "gltfSceneContext.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class gltfRawSkeleton : public RawSkeleton
    {
        friend class gltfSkeletonFileReader;
    };

    //-------------------------------------------------------------------------

    class gltfSkeletonFileReader
    {

    public:

        static TUniquePtr<EE::RawAssets::RawSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawSkeleton> pSkeleton( EE::New<RawSkeleton>() );
            gltfRawSkeleton* pRawSkeleton = (gltfRawSkeleton*) pSkeleton.get();

            //-------------------------------------------------------------------------

            gltf::gltfSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                ReadSkeleton( sceneCtx, skeletonRootBoneName, *pRawSkeleton );
            }
            else
            {
                pRawSkeleton->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pSkeleton;
        }

        static void ReadSkeleton( gltf::gltfSceneContext const& sceneCtx, String const& skeletonRootBoneName, gltfRawSkeleton& rawSkeleton )
        {
            auto pSceneData = sceneCtx.GetSceneData();

            if ( pSceneData->skins_count == 0 )
            {
                return;
            }

            cgltf_node* pSkeletonToRead = nullptr;
            if ( !skeletonRootBoneName.empty() )
            {
                for ( int32_t i = 0; i < pSceneData->skins_count; i++ )
                {
                    if ( pSceneData->skins[i].joints_count > 0 && pSceneData->skins[i].joints[0]->name == skeletonRootBoneName )
                    {
                        pSkeletonToRead = pSceneData->skins[i].joints[0];
                        break;
                    }
                }
            }
            else // Use first skin we find
            {
                if ( pSceneData->skins[0].joints_count > 0 )
                {
                    pSkeletonToRead = pSceneData->skins[0].joints[0];
                }
            }

            //-------------------------------------------------------------------------

            if ( pSkeletonToRead == nullptr )
            {
                if ( skeletonRootBoneName.empty() )
                {
                    rawSkeleton.LogError( "Failed to find any skeletons" );
                }
                else
                {
                    rawSkeleton.LogError( "Failed to find specified skeleton: %s", skeletonRootBoneName.c_str() );
                }
                return;
            }

            rawSkeleton.m_name = StringID( (char const*) pSkeletonToRead->name );
            ReadBoneHierarchy( rawSkeleton, sceneCtx, pSkeletonToRead, -1 );

            if ( rawSkeleton.GetNumBones() > 0 )
            {
                rawSkeleton.m_bones[0].m_localTransform = sceneCtx.ApplyUpAxisCorrection( rawSkeleton.m_bones[0].m_localTransform );
                rawSkeleton.CalculateGlobalTransforms();
            }
        }

        static void ReadBoneHierarchy( gltfRawSkeleton& rawSkeleton, gltf::gltfSceneContext const& sceneCtx, cgltf_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr );

            auto const boneIdx = (int32_t) rawSkeleton.m_bones.size();
            rawSkeleton.m_bones.push_back( RawSkeleton::BoneData( pNode->name ) );
            rawSkeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Default Bone transform
            rawSkeleton.m_bones[boneIdx].m_localTransform = sceneCtx.GetNodeTransform( pNode );

            // Read child bones
            for ( int i = 0; i < pNode->children_count; i++ )
            {
                cgltf_node* pChildNode = pNode->children[i];
                ReadBoneHierarchy( rawSkeleton, sceneCtx, pChildNode, boneIdx );
            }
        }
    };
}

//-------------------------------------------------------------------------

namespace EE::gltf
{
    TUniquePtr<EE::RawAssets::RawSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
    {
        return RawAssets::gltfSkeletonFileReader::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
    }
}
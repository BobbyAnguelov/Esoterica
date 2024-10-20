#include "ResourceLoader_AnimationSkeleton.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    SkeletonLoader::SkeletonLoader()
    {
        m_loadableTypes.push_back( Skeleton::GetStaticResourceTypeID() );
    }

    bool SkeletonLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        Skeleton* pSkeleton = EE::New<Skeleton>();
        archive << *pSkeleton;
        EE_ASSERT( pSkeleton->IsValid() );
        pResourceRecord->SetResourceData( pSkeleton );

        TVector<BoneMask::SerializedData> serializedBoneMasks;
        archive << serializedBoneMasks;

        // Preview
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        archive << pSkeleton->m_previewMeshID;
        archive << pSkeleton->m_previewAttachmentSocketID;
        #endif

        // Create bone masks
        //-------------------------------------------------------------------------

        pSkeleton->m_boneMasks.reserve( serializedBoneMasks.size() );
        for ( auto const& serializedBoneMask : serializedBoneMasks )
        {
            EE_ASSERT( pSkeleton->GetBoneMaskIndex( serializedBoneMask.m_ID ) == InvalidIndex ); // Ensure unique IDs
            pSkeleton->m_boneMasks.emplace_back( pSkeleton, serializedBoneMask );
        }

        // Calculate global reference pose
        //-------------------------------------------------------------------------

        int32_t const numBones = pSkeleton->GetNumBones();
        pSkeleton->m_modelSpaceReferencePose.resize( numBones );

        pSkeleton->m_modelSpaceReferencePose[0] = pSkeleton->m_parentSpaceReferencePose[0];
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            int32_t const parentIdx = pSkeleton->GetParentBoneIndex( boneIdx );
            pSkeleton->m_modelSpaceReferencePose[boneIdx] = pSkeleton->m_parentSpaceReferencePose[boneIdx] * pSkeleton->m_modelSpaceReferencePose[parentIdx];
        }

        //-------------------------------------------------------------------------

        return true;
    }
}
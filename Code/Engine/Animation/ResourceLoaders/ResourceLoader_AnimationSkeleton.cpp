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

    Resource::LoadResult SkeletonLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        Skeleton* pSkeleton = EE::New<Skeleton>();
        ( *pArchive ) << *pSkeleton;
        EE_ASSERT( pSkeleton->IsValid() );
        pResourceRecord->SetResourceData( pSkeleton );

        TVector<BoneMaskSetDefinition> boneMaskSets;
        ( *pArchive ) << boneMaskSets;

        // Preview
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        ( *pArchive ) << pSkeleton->m_previewMeshID;
        #endif

        // Create bone masks
        //-------------------------------------------------------------------------

        auto PrintMissingBonesWarning = [] ( StringID setID, BoneWeightList const& weightList, TInlineVector<StringID, 10> const& missingBones )
        {
            if ( !missingBones.empty() )
            {
                for ( StringID boneID : missingBones )
                {
                    EE_LOG_WARNING( LogCategory::Animation, "Skeleton Loader", "Couldn't find bone (%s) in skeleton(%s) while serializing bone mask (%s)", boneID.c_str(), weightList.m_skeletonID.c_str(), setID.c_str() );
                }
            }
        };

        TInlineVector<StringID, 10> missingBones;

        pSkeleton->m_boneMaskSets.reserve( boneMaskSets.size() );
        for ( BoneMaskSetDefinition const& serializedBoneMaskSet : boneMaskSets )
        {
            EE_ASSERT( pSkeleton->GetBoneMaskSetIndex( serializedBoneMaskSet.m_ID ) == InvalidIndex ); // Ensure unique IDs
            EE_ASSERT( serializedBoneMaskSet.m_primaryWeightList.IsValid() );

            BoneMaskSet& boneMaskSet = pSkeleton->m_boneMaskSets.emplace_back();
            boneMaskSet.m_ID = serializedBoneMaskSet.m_ID;
            boneMaskSet.m_primaryMask = BoneMask( pSkeleton, serializedBoneMaskSet.m_primaryWeightList, &missingBones );

            PrintMissingBonesWarning( serializedBoneMaskSet.m_ID, serializedBoneMaskSet.m_primaryWeightList, missingBones );

            for ( auto const& secondaryWeightList : serializedBoneMaskSet.m_secondaryWeightLists )
            {
                EE_ASSERT( secondaryWeightList.IsValid() );

                Skeleton const* pFoundSecondarySkeleton = nullptr;
                for ( auto const& secondarySkeletonRecord : pSkeleton->m_secondarySkeletons )
                {
                    if ( secondarySkeletonRecord.m_skeleton == secondaryWeightList.m_skeletonID )
                    {
                        pFoundSecondarySkeleton = secondarySkeletonRecord.m_skeleton.GetPtr<Skeleton>();
                        break;
                    }
                }

                if ( pFoundSecondarySkeleton != nullptr )
                {
                    boneMaskSet.m_secondaryMasks.emplace_back( pFoundSecondarySkeleton, secondaryWeightList, &missingBones );
                    PrintMissingBonesWarning( serializedBoneMaskSet.m_ID, serializedBoneMaskSet.m_primaryWeightList, missingBones );
                }
                else
                {
                    EE_LOG_WARNING( LogCategory::Animation, "Skeleton Loader", "Could find secondary skeleton for bone mask: %s", secondaryWeightList.m_skeletonID.c_str() );
                }
            }
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

        return Resource::LoadResult::Complete;
    }

    Resource::LoadResult SkeletonLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pSkeleton = pResourceRecord->GetResourceData<Skeleton>();

        // Set secondary skeletons
        for ( auto& secondarySkeleton : pSkeleton->m_secondarySkeletons )
        {
            EE_ASSERT( secondarySkeleton.m_skeleton.GetResourceID().IsValid() );
            secondarySkeleton.m_skeleton = GetInstallDependency( installDependencies, secondarySkeleton.m_skeleton.GetResourceID() );
        }

        return Resource::LoadResult::Complete;
    }
}
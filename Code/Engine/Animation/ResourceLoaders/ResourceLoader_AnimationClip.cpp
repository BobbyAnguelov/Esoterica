#include "ResourceLoader_AnimationClip.h"
#include "Engine/Animation/AnimationClip.h"
#include "Base/TypeSystem/TypeDescriptors.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationClipLoader::AnimationClipLoader()
    {
        m_loadableTypes.push_back( AnimationClip::GetStaticResourceTypeID() );
    }

    void AnimationClipLoader::SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry )
    {
        EE_ASSERT( pTypeRegistry != nullptr );
        m_pTypeRegistry = pTypeRegistry;
    }

    Resource::ResourceLoader::LoadResult AnimationClipLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT(  m_pTypeRegistry != nullptr );

        auto pAnimation = EE::New<AnimationClip>();
        archive << *pAnimation;
        pResourceRecord->SetResourceData( pAnimation );

        // Read sync events
        //-------------------------------------------------------------------------

        TInlineVector<SyncTrack::EventMarker, 10> syncEventMarkers;
        archive << syncEventMarkers;
        pAnimation->m_syncTrack = SyncTrack( syncEventMarkers, 0 );

        // Read events
        //-------------------------------------------------------------------------

        TypeSystem::TypeDescriptorCollection collectionDesc;
        archive << collectionDesc;

        collectionDesc.CalculateCollectionRequirements( *m_pTypeRegistry );
        TypeSystem::TypeDescriptorCollection::InstantiateStaticCollection( *m_pTypeRegistry, collectionDesc, pAnimation->m_events );

        // Read secondary animations
        //-------------------------------------------------------------------------

        int32_t numSecondaryAnimations = 0;
        archive << numSecondaryAnimations;

        for ( int32_t secondaryAnimIdx = 0; secondaryAnimIdx < numSecondaryAnimations; secondaryAnimIdx++ )
        {
            auto pSecondaryAnimation = EE::New<AnimationClip>();
            archive << *pSecondaryAnimation;
            pAnimation->m_secondaryAnimations.emplace_back( pSecondaryAnimation );
        }

        return Resource::ResourceLoader::LoadResult::Succeeded;
    }

    void AnimationClipLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pAnimClip = pResourceRecord->GetResourceData<AnimationClip>();
        if ( pAnimClip != nullptr )
        {
            // Delete all secondary animations
            for ( auto pSecondaryAnimation : pAnimClip->m_secondaryAnimations )
            {
                EE::Delete( pSecondaryAnimation );
            }

            // Release allocated events collection
            if ( !pAnimClip->m_events.empty() )
            {
                TypeSystem::TypeDescriptorCollection::DestroyStaticCollection( pAnimClip->m_events );
            }
        }

        ResourceLoader::Unload( resourceID, pResourceRecord );
    }

    Resource::ResourceLoader::LoadResult AnimationClipLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pAnimClip = pResourceRecord->GetResourceData<AnimationClip>();
        EE_ASSERT( pAnimClip->m_skeleton.GetResourceID().IsValid() );

        // Set primary skeleton
        pAnimClip->m_skeleton = GetInstallDependency( installDependencies, pAnimClip->m_skeleton.GetResourceID() );
        EE_ASSERT( pAnimClip->IsValid() );

        // Set secondary skeletons
        for ( auto pSecondaryAnimation : pAnimClip->m_secondaryAnimations )
        {
            const_cast<AnimationClip*>( pSecondaryAnimation )->m_skeleton = GetInstallDependency( installDependencies, pSecondaryAnimation->m_skeleton.GetResourceID() );
            EE_ASSERT( pSecondaryAnimation->IsValid() );
        }

        ResourceLoader::Install( resourceID, installDependencies, pResourceRecord );

        return ResourceLoader::LoadResult::Succeeded;
    }
}
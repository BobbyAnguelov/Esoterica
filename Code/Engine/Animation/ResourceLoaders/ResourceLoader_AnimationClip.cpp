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

    Resource::LoadResult AnimationClipLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        EE_ASSERT(  m_pTypeRegistry != nullptr );

        auto pAnimation = EE::New<AnimationClip>();
        ( *pArchive ) << *pAnimation;
        pResourceRecord->SetResourceData( pAnimation );

        // Read sync events
        //-------------------------------------------------------------------------

        TInlineVector<SyncTrack::EventMarker, 10> syncEventMarkers;
        ( *pArchive ) << syncEventMarkers;
        pAnimation->m_syncTrack = SyncTrack( syncEventMarkers, 0 );

        // Read events
        //-------------------------------------------------------------------------

        TypeSystem::TypeDescriptorCollection collectionDesc;
        ( *pArchive ) << collectionDesc;

        collectionDesc.CalculateCollectionRequirements( *m_pTypeRegistry );
        TypeSystem::TypeDescriptorCollection::InstantiateStaticCollection( *m_pTypeRegistry, collectionDesc, pAnimation->m_events );

        // Read secondary animations
        //-------------------------------------------------------------------------

        int32_t numSecondaryAnimations = 0;
        ( *pArchive ) << numSecondaryAnimations;

        for ( int32_t secondaryAnimIdx = 0; secondaryAnimIdx < numSecondaryAnimations; secondaryAnimIdx++ )
        {
            auto pSecondaryAnimation = EE::New<AnimationClip>();
            ( *pArchive ) << *pSecondaryAnimation;
            pAnimation->m_secondaryAnimations.emplace_back( pSecondaryAnimation );
        }

        return Resource::LoadResult::Complete;
    }

    Resource::LoadResult AnimationClipLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
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

        // Set float channel skeletons
        //-------------------------------------------------------------------------

        for ( auto& fcs : pAnimClip->m_floatChannelSetData )
        {
            fcs.m_pSkeleton = pAnimClip->m_skeleton.GetPtr();
        }

        for ( auto pSecondaryAnimation : pAnimClip->m_secondaryAnimations )
        {
            for ( auto& fcs : const_cast<AnimationClip*>( pSecondaryAnimation )->m_floatChannelSetData )
            {
                fcs.m_pSkeleton = pSecondaryAnimation->m_skeleton.GetPtr();
            }
        }

        return Resource::LoadResult::Complete;
    }

    Resource::UnloadResult AnimationClipLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
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

        return Resource::UnloadResult::Complete;
    }

}
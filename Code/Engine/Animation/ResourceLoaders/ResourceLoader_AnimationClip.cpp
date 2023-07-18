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

    bool AnimationClipLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
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

        return true;
    }

    void AnimationClipLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pAnimation = pResourceRecord->GetResourceData<AnimationClip>();
        if ( pAnimation != nullptr )
        {
            // Release allocated events collection
            if ( !pAnimation->m_events.empty() )
            {
                TypeSystem::TypeDescriptorCollection::DestroyStaticCollection( pAnimation->m_events );
            }
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }

    Resource::InstallResult AnimationClipLoader::Install( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const
    {
        auto pAnimData = pResourceRecord->GetResourceData<AnimationClip>();
        EE_ASSERT( pAnimData->m_skeleton.GetResourceID().IsValid() );

        pAnimData->m_skeleton = GetInstallDependency( installDependencies, pAnimData->m_skeleton.GetResourceID() );
        EE_ASSERT( pAnimData->IsValid() );

        ResourceLoader::Install( resID, pResourceRecord, installDependencies );

        return Resource::InstallResult::Succeeded;
    }
}
#include "ResourceLoader_EntityCollection.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionLoader::EntityCollectionLoader()
        : m_pTypeRegistry( nullptr )
    {
        m_loadableTypes.push_back( EntityCollection::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( EntityMapDescriptor::GetStaticResourceTypeID() );
    }

    void EntityCollectionLoader::SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry )
    {
        EE_ASSERT( pTypeRegistry != nullptr );
        m_pTypeRegistry = pTypeRegistry;
    }

    Resource::LoadResult EntityCollectionLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        EE_ASSERT( m_pTypeRegistry != nullptr );

        EntityCollection* pCollectionDesc = nullptr;

        if ( resourceID.GetResourceTypeID() == EntityMapDescriptor::GetStaticResourceTypeID() )
        {
            auto pMap = EE::New<EntityMapDescriptor>();
            ( *pArchive ) << *pMap;
            pCollectionDesc = pMap;
        }
        else  if ( resourceID.GetResourceTypeID() == EntityCollection::GetStaticResourceTypeID() )
        {
            auto pEC = EE::New<EntityCollection>();
            ( *pArchive ) << *pEC;
            pCollectionDesc = pEC;
        }

        // Set loaded resource
        pResourceRecord->SetResourceData( pCollectionDesc );
        return Resource::LoadResult::Complete;
    }
}
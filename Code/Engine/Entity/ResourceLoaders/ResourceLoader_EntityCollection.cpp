#include "ResourceLoader_EntityCollection.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionLoader::EntityCollectionLoader()
        : m_pTypeRegistry( nullptr )
    {
        m_loadableTypes.push_back( EntityCollectionDescriptor::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( EntityMapDescriptor::GetStaticResourceTypeID() );
    }

    void EntityCollectionLoader::SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry )
    {
        EE_ASSERT( pTypeRegistry != nullptr );
        m_pTypeRegistry = pTypeRegistry;
    }

    bool EntityCollectionLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pTypeRegistry != nullptr );

        EntityCollectionDescriptor* pCollectionDesc = nullptr;

        if ( resID.GetResourceTypeID() == EntityMapDescriptor::GetStaticResourceTypeID() )
        {
            auto pMap = EE::New<EntityMapDescriptor>();
            archive << *pMap;
            pCollectionDesc = pMap;
        }
        else  if ( resID.GetResourceTypeID() == EntityCollectionDescriptor::GetStaticResourceTypeID() )
        {
            auto pEC = EE::New<EntityCollectionDescriptor>();
            archive << *pEC;
            pCollectionDesc = pEC;
        }

        // Set loaded resource
        pResourceRecord->SetResourceData( pCollectionDesc );
        return true;
    }
}
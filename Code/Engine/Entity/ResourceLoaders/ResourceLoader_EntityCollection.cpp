#include "ResourceLoader_EntityCollection.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionLoader::EntityCollectionLoader()
        : m_pTypeRegistry( nullptr )
    {
        m_loadableTypes.push_back( SerializedEntityCollection::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( SerializedEntityMap::GetStaticResourceTypeID() );
    }

    void EntityCollectionLoader::SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry )
    {
        EE_ASSERT( pTypeRegistry != nullptr );
        m_pTypeRegistry = pTypeRegistry;
    }

    bool EntityCollectionLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pTypeRegistry != nullptr );

        SerializedEntityCollection* pCollectionDesc = nullptr;

        if ( resID.GetResourceTypeID() == SerializedEntityMap::GetStaticResourceTypeID() )
        {
            auto pMap = EE::New<SerializedEntityMap>();
            archive << *pMap;
            pCollectionDesc = pMap;
        }
        else  if ( resID.GetResourceTypeID() == SerializedEntityCollection::GetStaticResourceTypeID() )
        {
            auto pEC = EE::New<SerializedEntityCollection>();
            archive << *pEC;
            pCollectionDesc = pEC;
        }

        // Set loaded resource
        pResourceRecord->SetResourceData( pCollectionDesc );
        return true;
    }
}
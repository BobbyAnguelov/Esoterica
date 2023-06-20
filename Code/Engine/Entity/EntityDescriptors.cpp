#include "EntityDescriptors.h"

#include "Entity.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Profiling.h"
#include "System/Threading/TaskSystem.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    int32_t SerializedEntityDescriptor::FindComponentIndex( StringID const& componentName ) const
    {
        EE_ASSERT( componentName.IsValid() );

        int32_t const numComponents = (int32_t) m_components.size();
        for ( int32_t i = 0; i < numComponents; i++ )
        {
            if ( m_components[i].m_name == componentName )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    #if EE_DEVELOPMENT_TOOLS
    void SerializedEntityDescriptor::ClearAllSerializedIDs()
    {
        m_transientEntityID.Clear();

        for ( auto& component : m_components )
        {
            component.m_transientComponentID.Clear();
        }
    }
    #endif

    //-------------------------------------------------------------------------

    TVector<SerializedEntityCollection::SearchResult> SerializedEntityCollection::GetComponentsOfType( TypeSystem::TypeRegistry const& typeRegistry, TypeSystem::TypeID typeID, bool allowDerivedTypes )
    {
        TVector<SearchResult> foundComponents;

        for ( auto& entityDesc : m_entityDescriptors )
        {
            for ( auto& componentDesc : entityDesc.m_components )
            {
                if ( componentDesc.m_typeID == typeID )
                {
                    auto& result = foundComponents.emplace_back( SearchResult() );
                    result.m_pEntity = &entityDesc;
                    result.m_pComponent = &componentDesc;
                }
                else if ( allowDerivedTypes )
                {
                    auto pTypeInfo = typeRegistry.GetTypeInfo( componentDesc.m_typeID );
                    EE_ASSERT( pTypeInfo != nullptr );

                    if ( pTypeInfo->IsDerivedFrom( typeID ) )
                    {
                        auto& result = foundComponents.emplace_back( SearchResult() );
                        result.m_pEntity = &entityDesc;
                        result.m_pComponent = &componentDesc;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        return foundComponents;
    }

    #if EE_DEVELOPMENT_TOOLS
    void SerializedEntityCollection::Clear()
    {
        m_entityDescriptors.clear();
        m_entityLookupMap.clear();
        m_entitySpatialAttachmentInfo.clear();
    }

    void SerializedEntityCollection::SetCollectionData( TVector<SerializedEntityDescriptor>&& entityDescriptors )
    {
        // Set entity descriptors
        //-------------------------------------------------------------------------

        m_entityDescriptors.swap( entityDescriptors );
        int32_t const numEntities = (int32_t) m_entityDescriptors.size();

        // Generate spatial hierarchy depths
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            auto& entityDesc = m_entityDescriptors[i];
            EE_ASSERT( entityDesc.IsValid() );
            entityDesc.m_spatialHierarchyDepth = 0;

            if ( entityDesc.IsSpatialEntity() )
            {
                // Calculate the spatial hierarchy depth
                StringID parentID = entityDesc.m_spatialParentName;
                while ( parentID.IsValid() )
                {
                    entityDesc.m_spatialHierarchyDepth++;

                    int32_t const parentIdx = m_entityLookupMap[parentID];
                    EE_ASSERT( parentIdx != InvalidIndex );
                    parentID = m_entityDescriptors[parentIdx].m_spatialParentName;
                }
            }
        }

        // Sort entity desc array by depth
        //-------------------------------------------------------------------------

        auto SortComparator = [] ( SerializedEntityDescriptor const& entityA, SerializedEntityDescriptor const& entityB )
        {
            EE_ASSERT( entityA.m_spatialHierarchyDepth >= 0 && entityA.m_spatialHierarchyDepth >= 0 );
            return entityA.m_spatialHierarchyDepth < entityB.m_spatialHierarchyDepth;
        };

        eastl::sort( m_entityDescriptors.begin(), m_entityDescriptors.end(), SortComparator );

        // Create lookup map
        //-------------------------------------------------------------------------

        m_entityLookupMap.clear();
        m_entityLookupMap.reserve( numEntities );

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            m_entityLookupMap.insert( TPair<StringID, int32_t>( m_entityDescriptors[i].m_name, i ) );
        }

        // Generate spatial attachment info
        //-------------------------------------------------------------------------

        m_entitySpatialAttachmentInfo.clear();
        m_entitySpatialAttachmentInfo.reserve( m_entityDescriptors.size() );

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            auto const& entityDesc = m_entityDescriptors[i];
            if ( !entityDesc.IsSpatialEntity() || !entityDesc.HasSpatialParent() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            SpatialAttachmentInfo attachmentInfo;
            attachmentInfo.m_entityIdx = i;
            attachmentInfo.m_parentEntityIdx = FindEntityIndex( entityDesc.m_spatialParentName );

            if ( attachmentInfo.m_parentEntityIdx != InvalidIndex )
            {
                m_entitySpatialAttachmentInfo.push_back( attachmentInfo );
            }
        }
    }

    void SerializedEntityCollection::GetAllReferencedResources( TVector<ResourceID>& outReferencedResources ) const
    {
        outReferencedResources.clear();

        TypeSystem::TypeID const resourceIDTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::ResourceID );
        TypeSystem::TypeID const resourcePathTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::ResourcePath );
        TypeSystem::TypeID const resourcePtrTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::ResourcePtr );
        TypeSystem::TypeID const templateResourcePtrTypeID = TypeSystem::CoreTypeRegistry::GetTypeID( TypeSystem::CoreTypeID::TResourcePtr );

        for ( auto const& entityDesc : m_entityDescriptors )
        {
            for ( auto const& componentDesc : entityDesc.m_components )
            {
                for ( auto const& propertyDesc : componentDesc.m_properties )
                {
                    if ( propertyDesc.m_typeID == resourceIDTypeID || propertyDesc.m_typeID == resourcePathTypeID || propertyDesc.m_typeID == resourcePtrTypeID || propertyDesc.m_typeID == templateResourcePtrTypeID )
                    {
                        VectorEmplaceBackUnique<ResourceID>( outReferencedResources, ResourceID( propertyDesc.m_stringValue ) );
                    }
                }
            }
        }
    }
    #endif
}
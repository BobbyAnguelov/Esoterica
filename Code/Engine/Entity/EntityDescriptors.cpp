#include "EntityDescriptors.h"
#include "System/Log.h"
#include "Entity.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Profiling.h"
#include "System/Threading/TaskSystem.h"

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
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    void SerializedEntityCollection::Reserve( int32_t numEntities )
    {
        m_entityDescriptors.reserve( numEntities );
        m_entityLookupMap.reserve( numEntities );
    }

    void SerializedEntityCollection::GenerateSpatialAttachmentInfo()
    {
        m_entitySpatialAttachmentInfo.clear();
        m_entitySpatialAttachmentInfo.reserve( m_entityDescriptors.size() );

        int32_t const numEntities = (int32_t) m_entityDescriptors.size();
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

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
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
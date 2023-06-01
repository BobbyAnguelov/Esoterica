#include "TypeRegistry.h"
#include "PropertyPath.h"
#include "ResourceInfo.h"
#include "EnumInfo.h"
#include "TypeInfo.h"
#include "System/Log.h"
#include "DefaultTypeInfos.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    TypeRegistry::TypeRegistry()
    {
        TTypeInfo<IReflectedType>::RegisterType( *this );
    }

    TypeRegistry::~TypeRegistry()
    {
        TTypeInfo<IReflectedType>::UnregisterType( *this );
        EE_ASSERT( m_registeredEnums.empty() && m_registeredTypes.empty() && m_registeredResourceTypes.empty() );
    }

    //-------------------------------------------------------------------------

    TypeInfo const* TypeRegistry::RegisterType( TypeInfo const* pTypeInfo )
    {
        EE_ASSERT( pTypeInfo != nullptr );
        EE_ASSERT( pTypeInfo->m_ID.IsValid() && !CoreTypeRegistry::IsCoreType( pTypeInfo->m_ID ) );
        EE_ASSERT( m_registeredTypes.find( pTypeInfo->m_ID ) == m_registeredTypes.end() );
        m_registeredTypes.insert( eastl::pair<TypeID, TypeInfo const*>( pTypeInfo->m_ID, pTypeInfo ) );
        return m_registeredTypes[pTypeInfo->m_ID];
    }

    void TypeRegistry::UnregisterType( TypeInfo const* pTypeInfo )
    {
        EE_ASSERT( pTypeInfo != nullptr );
        EE_ASSERT( pTypeInfo->m_ID.IsValid() && !CoreTypeRegistry::IsCoreType( pTypeInfo->m_ID ) );
        auto iter = m_registeredTypes.find( pTypeInfo->m_ID );
        EE_ASSERT( iter != m_registeredTypes.end() );
        EE_ASSERT( iter->second == pTypeInfo );
        m_registeredTypes.erase( iter );
    }

    TypeInfo const* TypeRegistry::GetTypeInfo( TypeID typeID ) const
    {
        EE_ASSERT( typeID.IsValid() && !CoreTypeRegistry::IsCoreType( typeID ) );
        auto iter = m_registeredTypes.find( typeID );
        if ( iter != m_registeredTypes.end() )
        {
            return iter->second;
        }
        else
        {
            return nullptr;
        }
    }

    PropertyInfo const* TypeRegistry::ResolvePropertyPath( TypeInfo const* pTypeInfo, PropertyPath const& pathID ) const
    {
        TypeInfo const* pParentTypeInfo = pTypeInfo;
        PropertyInfo const* pFoundPropertyInfo = nullptr;

        // Resolve property path
        size_t const numPathElements = pathID.GetNumElements();
        size_t const lastElementIdx = numPathElements - 1;
        for ( size_t i = 0; i < numPathElements; i++ )
        {
            pFoundPropertyInfo = pParentTypeInfo->GetPropertyInfo( pathID[i].m_propertyID );
            if ( pFoundPropertyInfo == nullptr )
            {
                break;
            }

            if ( i != lastElementIdx )
            {
                // If this occurs, we have an invalid path as each element must contain other properties
                if ( CoreTypeRegistry::IsCoreType( pFoundPropertyInfo->m_typeID ) && !pFoundPropertyInfo->IsArrayProperty() )
                {
                    EE_LOG_WARNING( "TypeSystem", "Type Registry", "Cant resolve malformed property path" );
                    pFoundPropertyInfo = nullptr;
                    break;
                }

                // Get the type desc of the property
                pParentTypeInfo = GetTypeInfo( pFoundPropertyInfo->m_typeID );
                if ( pParentTypeInfo == nullptr )
                {
                    EE_LOG_ERROR( "TypeSystem", "Type Registry", "Cant resolve property path since it contains an unknown type" );
                    pFoundPropertyInfo = nullptr;
                }
            }
        }

        return pFoundPropertyInfo;
    }

    bool TypeRegistry::IsTypeDerivedFrom( TypeID typeID, TypeID parentTypeID ) const
    {
        EE_ASSERT( typeID.IsValid() && parentTypeID.IsValid() );
        EE_ASSERT( !CoreTypeRegistry::IsCoreType( typeID ) && !CoreTypeRegistry::IsCoreType( parentTypeID ) );

        auto pTypeInfo = GetTypeInfo( typeID );
        EE_ASSERT( pTypeInfo != nullptr );

        return pTypeInfo->IsDerivedFrom( parentTypeID );
    }

    TVector<TypeInfo const*> TypeRegistry::GetAllTypes( bool includeAbstractTypes, bool sortAlphabetically ) const
    {
        TVector<TypeInfo const*> types;

        for ( auto const& typeInfoPair : m_registeredTypes )
        {
            if ( !includeAbstractTypes && typeInfoPair.second->IsAbstractType() )
            {
                continue;
            }

            types.emplace_back( typeInfoPair.second );
        }

        if ( sortAlphabetically )
        {
            auto sortPredicate = [] ( TypeSystem::TypeInfo const* const& pTypeInfoA, TypeSystem::TypeInfo const* const& pTypeInfoB )
            {
                #if EE_DEVELOPMENT_TOOLS
                return pTypeInfoA->m_friendlyName < pTypeInfoB->m_friendlyName;
                #else
                return strcmp( pTypeInfoA->m_ID.c_str(), pTypeInfoB->m_ID.c_str() );
                #endif
            };

            eastl::sort( types.begin(), types.end(), sortPredicate );
        }

        return types;
    }

    TVector<TypeInfo const*> TypeRegistry::GetAllDerivedTypes( TypeID parentTypeID, bool includeParentTypeInResults, bool includeAbstractTypes, bool sortAlphabetically ) const
    {
        TVector<TypeInfo const*> matchingTypes;

        for ( auto const& typeInfoPair : m_registeredTypes )
        {
            if ( !includeParentTypeInResults && typeInfoPair.first == parentTypeID )
            {
                continue;
            }

            if ( !includeAbstractTypes && typeInfoPair.second->IsAbstractType() )
            {
                continue;
            }

            if ( typeInfoPair.second->IsDerivedFrom( parentTypeID ) )
            {
                matchingTypes.emplace_back( typeInfoPair.second );
            }
        }

        if ( sortAlphabetically )
        {
            auto sortPredicate = [] ( TypeSystem::TypeInfo const* const& pTypeInfoA, TypeSystem::TypeInfo const* const& pTypeInfoB )
            {
                #if EE_DEVELOPMENT_TOOLS
                return pTypeInfoA->m_friendlyName < pTypeInfoB->m_friendlyName;
                #else
                return strcmp( pTypeInfoA->m_ID.c_str(), pTypeInfoB->m_ID.c_str() );
                #endif
            };

            eastl::sort( matchingTypes.begin(), matchingTypes.end(), sortPredicate );
        }

        return matchingTypes;
    }

    TInlineVector<EE::TypeSystem::TypeID, 5> TypeRegistry::GetAllCastableTypes( IReflectedType const* pType ) const
    {
        EE_ASSERT( pType != nullptr );
        TInlineVector<EE::TypeSystem::TypeID, 5> parentTypeIDs;
        auto pParentTypeInfo = pType->GetTypeInfo()->m_pParentTypeInfo;
        while ( pParentTypeInfo != nullptr )
        {
            parentTypeIDs.emplace_back( pParentTypeInfo->m_ID );
            pParentTypeInfo = pParentTypeInfo->m_pParentTypeInfo;
        }
        return parentTypeIDs;
    }

    bool TypeRegistry::AreTypesInTheSameHierarchy( TypeID typeA, TypeID typeB ) const 
    {
        auto pTypeInfoA = GetTypeInfo( typeA );
        auto pTypeInfoB = GetTypeInfo( typeB );
        return AreTypesInTheSameHierarchy( pTypeInfoA, pTypeInfoB );
    }

    bool TypeRegistry::AreTypesInTheSameHierarchy( TypeInfo const* pTypeInfoA, TypeInfo const* pTypeInfoB ) const
    {
        if ( pTypeInfoA->IsDerivedFrom( pTypeInfoB->m_ID ) )
        {
            return true;
        }

        if ( pTypeInfoB->IsDerivedFrom( pTypeInfoA->m_ID ) )
        {
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    EnumInfo const* TypeRegistry::RegisterEnum( EnumInfo const& type )
    {
        EE_ASSERT( type.m_ID.IsValid() );
        EE_ASSERT( m_registeredEnums.find( type.m_ID ) == m_registeredEnums.end() );
        m_registeredEnums[type.m_ID] = EE::New<EnumInfo>( type );
        return m_registeredEnums[type.m_ID];
    }

    void TypeRegistry::UnregisterEnum( TypeID const typeID )
    {
        EE_ASSERT( typeID.IsValid() );
        auto iter = m_registeredEnums.find( typeID );
        EE_ASSERT( iter != m_registeredEnums.end() );
        EE::Delete<EnumInfo>( iter->second );
        m_registeredEnums.erase( iter );
    }

    EnumInfo const* TypeRegistry::GetEnumInfo( TypeID enumID ) const
    {
        EE_ASSERT( enumID.IsValid() );
        auto iter = m_registeredEnums.find( enumID );
        if ( iter != m_registeredEnums.end() )
        {
            return iter->second;
        }
        else
        {
            return nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void TypeRegistry::RegisterResourceTypeID( ResourceInfo const& resourceInfo )
    {
        EE_ASSERT( resourceInfo.IsValid() && !CoreTypeRegistry::IsCoreType( resourceInfo.m_typeID ) );
        EE_ASSERT( m_registeredResourceTypes.find( resourceInfo.m_typeID ) == m_registeredResourceTypes.end() );
        m_registeredResourceTypes.insert( eastl::pair<TypeID, ResourceInfo>( resourceInfo.m_typeID, resourceInfo ) );
    }

    void TypeRegistry::UnregisterResourceTypeID( TypeID typeID )
    {
        EE_ASSERT( typeID.IsValid() && !CoreTypeRegistry::IsCoreType( typeID ) );
        auto iter = m_registeredResourceTypes.find( typeID );
        EE_ASSERT( iter != m_registeredResourceTypes.end() );
        m_registeredResourceTypes.erase( iter );
    }

    bool TypeRegistry::IsRegisteredResourceType( TypeID typeID ) const
    {
        return m_registeredResourceTypes.find( typeID ) != m_registeredResourceTypes.end();
    }

    bool TypeRegistry::IsRegisteredResourceType( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& pair : m_registeredResourceTypes )
        {
            if ( pair.second.m_resourceTypeID == resourceTypeID )
            {
                return true;
            }
        }

        return false;
    }

    ResourceInfo const* TypeRegistry::GetResourceInfoForType( TypeID typeID ) const
    {
        auto iter = m_registeredResourceTypes.find( typeID );
        if ( iter != m_registeredResourceTypes.end() )
        {
            return &iter->second;
        }

        //-------------------------------------------------------------------------

        EE_HALT();
        return nullptr;
    }

    ResourceInfo const* TypeRegistry::GetResourceInfoForResourceType( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& resourceInfo : m_registeredResourceTypes )
        {
            if ( resourceInfo.second.m_resourceTypeID == resourceTypeID )
            {
                return &resourceInfo.second;
            }
        }

        //-------------------------------------------------------------------------

        EE_HALT();
        return nullptr;
    }

    bool TypeRegistry::IsResourceTypeDerivedFrom( ResourceTypeID childResourceTypeID, ResourceTypeID parentResourceTypeID ) const
    {
        if ( childResourceTypeID == parentResourceTypeID )
        {
            return true;
        }

        auto pChildResourceInfo = GetResourceInfoForResourceType( childResourceTypeID );
        EE_ASSERT( pChildResourceInfo != nullptr );
        return VectorContains( pChildResourceInfo->m_parentTypes, parentResourceTypeID );
    }

    //-------------------------------------------------------------------------

    size_t TypeRegistry::GetTypeByteSize( TypeID typeID ) const
    {
        EE_ASSERT( typeID.IsValid() );

        if ( IsCoreType( typeID ) )
        {
            return CoreTypeRegistry::GetTypeSize( typeID );
        }
        else
        {
            auto pChildTypeInfo = GetTypeInfo( typeID );
            EE_ASSERT( pChildTypeInfo != nullptr );
            return pChildTypeInfo->m_size;
        }
    }

    TVector<ResourceTypeID> TypeRegistry::GetAllDerivedResourceTypes( ResourceTypeID resourceTypeID ) const
    {
        TVector<ResourceTypeID> derivedResourceTypes;
        for ( auto const& pair: m_registeredResourceTypes )
        {
            if ( pair.second.m_resourceTypeID == resourceTypeID )
            {
                continue;
            }

            if ( VectorContains( pair.second.m_parentTypes, resourceTypeID ) )
            {
                derivedResourceTypes.emplace_back( pair.second.m_resourceTypeID );
            }
        }

        return derivedResourceTypes;
    }
}
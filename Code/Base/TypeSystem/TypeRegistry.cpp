#include "TypeRegistry.h"
#include "PropertyPath.h"
#include "ResourceInfo.h"
#include "DataFileInfo.h"
#include "EnumInfo.h"
#include "TypeInfo.h"
#include "ReflectedType.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    template<>
    class TTypeInfo<IReflectedType> final : public TypeInfo
    {
    public:

        static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )
        {
            IReflectedType::s_pTypeInfo = EE::New<TTypeInfo<IReflectedType>>();
            typeRegistry.RegisterType( IReflectedType::s_pTypeInfo );
        }

        static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )
        {
            typeRegistry.UnregisterType( IReflectedType::s_pTypeInfo );
            EE::Delete( IReflectedType::s_pTypeInfo );
        }

    public:

        TTypeInfo()
        {
            m_ID = TypeSystem::TypeID( "EE::IReflectedType" );
            m_size = sizeof( IReflectedType );
            m_alignment = alignof( IReflectedType );

            #if EE_DEVELOPMENT_TOOLS
            m_friendlyName = "IReflectedType";
            #endif
        }

        virtual IReflectedType* CreateType() const override
        {
            EE_HALT(); // Error! Trying to instantiate an abstract entity system!
            return nullptr;
        }

        virtual void CreateTypeInPlace( IReflectedType* pAllocatedMemory ) const override { EE_HALT(); }
        virtual void ResetType( IReflectedType* pTypeInstance ) const override { EE_HALT(); }
        virtual void CopyProperties( IReflectedType* pTypeInstance, IReflectedType const* pRHS ) const override {}
        virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override {}
        virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override {}
        virtual void GetReferencedResources( IReflectedType const* pType, TVector<ResourceID>& outReferencedResources ) const override {}

        virtual LoadingStatus GetResourceLoadingStatus( IReflectedType* pType ) const override
        {
            return LoadingStatus::Loaded;
        }

        virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType* pType ) const override
        {
            return LoadingStatus::Unloaded;
        }

        virtual uint8_t* GetArrayElementDataPtr( IReflectedType* pType, uint64_t arrayID, size_t arrayIdx ) const override
        {
            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint64_t arrayID ) const override
        {
            EE_UNREACHABLE_CODE();
            return 0;
        }

        virtual size_t GetArrayElementSize( uint64_t arrayID ) const override
        {
            EE_UNREACHABLE_CODE();
            return 0;
        }

        virtual void SetArraySize( IReflectedType* pTypeInstance, uint64_t arrayID, size_t size ) const override { EE_UNREACHABLE_CODE(); }
        virtual void ClearArray( IReflectedType* pTypeInstance, uint64_t arrayID ) const override { EE_UNREACHABLE_CODE(); }
        virtual void AddArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID ) const override { EE_UNREACHABLE_CODE(); }
        virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID, size_t insertIdx ) const { EE_UNREACHABLE_CODE(); }
        virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const { EE_UNREACHABLE_CODE(); }
        virtual void RemoveArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID, size_t arrayIdx ) const override { EE_UNREACHABLE_CODE(); }

        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType* pType, uint64_t propertyID ) const override
        {
            EE_UNREACHABLE_CODE();
            return ResourceTypeID();
        }

        virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const override { return false; }
        virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint64_t propertyID, int32_t arrayIdx = InvalidIndex ) const override { return false; }
        virtual void ResetToDefault( IReflectedType* pTypeInstance, uint64_t propertyID ) const override {}
    };
}

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    TypeRegistry::~TypeRegistry()
    {
        EE_ASSERT( m_registeredEnums.empty() );
        EE_ASSERT( m_registeredTypes.empty() );
        EE_ASSERT( m_registeredResourceTypes.empty() );

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( m_registeredDataFileTypes.empty() );
        #endif
    }

    void TypeRegistry::RegisterInternalTypes()
    {
        TTypeInfo<IReflectedType>::RegisterType( *this );
    }

    void TypeRegistry::UnregisterInternalTypes()
    {
        TTypeInfo<IReflectedType>::UnregisterType( *this );
    }

    //-------------------------------------------------------------------------
    // Type Info
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
    // Enum Info
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
    // Resource Info
    //-------------------------------------------------------------------------

    void TypeRegistry::RegisterResourceTypeID( ResourceInfo const& resourceInfo )
    {
        EE_ASSERT( resourceInfo.IsValid() );
        EE_ASSERT( m_registeredResourceTypes.find( resourceInfo.m_resourceTypeID) == m_registeredResourceTypes.end() );
        m_registeredResourceTypes.insert( eastl::pair<ResourceTypeID, ResourceInfo*>( resourceInfo.m_resourceTypeID, EE::New<ResourceInfo>( resourceInfo ) ) );
    }

    void TypeRegistry::UnregisterResourceTypeID( ResourceTypeID resourceTypeID )
    {
        EE_ASSERT( resourceTypeID.IsValid() );
        auto iter = m_registeredResourceTypes.find( resourceTypeID );
        EE_ASSERT( iter != m_registeredResourceTypes.end() );
        EE::Delete( iter->second );
        m_registeredResourceTypes.erase( iter );
    }

    bool TypeRegistry::IsRegisteredResourceType( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& pair : m_registeredResourceTypes )
        {
            if ( pair.second->m_resourceTypeID == resourceTypeID )
            {
                return true;
            }
        }

        return false;
    }

    ResourceInfo const* TypeRegistry::GetResourceInfo( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& resourceInfo : m_registeredResourceTypes )
        {
            if ( resourceInfo.second->m_resourceTypeID == resourceTypeID )
            {
                return resourceInfo.second;
            }
        }

        return nullptr;
    }

    ResourceInfo const* TypeRegistry::GetResourceInfo( TypeID typeID ) const
    {
        for ( auto const& resourceInfo : m_registeredResourceTypes )
        {
            if ( resourceInfo.second->m_typeID == typeID )
            {
                return resourceInfo.second;
            }
        }

        return nullptr;
    }

    bool TypeRegistry::IsResourceTypeDerivedFrom( ResourceTypeID childResourceTypeID, ResourceTypeID parentResourceTypeID ) const
    {
        if ( childResourceTypeID == parentResourceTypeID )
        {
            return true;
        }

        auto pChildResourceInfo = GetResourceInfo( childResourceTypeID );
        EE_ASSERT( pChildResourceInfo != nullptr );
        return VectorContains( pChildResourceInfo->m_parentTypes, parentResourceTypeID );
    }

    TVector<ResourceTypeID> TypeRegistry::GetAllDerivedResourceTypes( ResourceTypeID resourceTypeID ) const
    {
        TVector<ResourceTypeID> derivedResourceTypes;
        for ( auto const& pair: m_registeredResourceTypes )
        {
            if ( pair.second->m_resourceTypeID == resourceTypeID )
            {
                continue;
            }

            if ( VectorContains( pair.second->m_parentTypes, resourceTypeID ) )
            {
                derivedResourceTypes.emplace_back( pair.second->m_resourceTypeID );
            }
        }

        return derivedResourceTypes;
    }

    //-------------------------------------------------------------------------
    // Data File Info
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void TypeRegistry::RegisterDataFileInfo( DataFileInfo const& dataFileInfo )
    {
        EE_ASSERT( dataFileInfo.IsValid() );
        EE_ASSERT( m_registeredDataFileTypes.find( dataFileInfo.m_typeID ) == m_registeredDataFileTypes.end() );
        m_registeredDataFileTypes.insert( eastl::pair<TypeID, DataFileInfo*>( dataFileInfo.m_typeID, EE::New<DataFileInfo>( dataFileInfo ) ) );
    }

    void TypeRegistry::UnregisterDataFileInfo( TypeID typeID )
    {
        EE_ASSERT( typeID.IsValid() );
        auto iter = m_registeredDataFileTypes.find( typeID );
        EE_ASSERT( iter != m_registeredDataFileTypes.end() );
        EE::Delete( iter->second );
        m_registeredDataFileTypes.erase( iter );
    }

    bool TypeRegistry::IsRegisteredDataFileType( TypeID typeID ) const
    {
        for ( auto const& pair : m_registeredDataFileTypes )
        {
            if ( pair.second->m_typeID == typeID )
            {
                return true;
            }
        }

        return false;
    }

    bool TypeRegistry::IsRegisteredDataFileType( uint32_t extFourCC ) const
    {
        for ( auto const& pair : m_registeredDataFileTypes )
        {
            if ( pair.second->m_extensionFourCC == extFourCC )
            {
                return true;
            }
        }

        return false;
    }

    DataFileInfo const* TypeRegistry::GetDataFileInfo( TypeID typeID ) const
    {
        for ( auto const& dataFileInfo : m_registeredDataFileTypes )
        {
            if ( dataFileInfo.second->m_typeID == typeID )
            {
                return dataFileInfo.second;
            }
        }

        return nullptr;
    }
    #endif
}
#pragma once

#include "ResourceInfo.h"
#include "TypeInfo.h"
#include "CoreTypeIDs.h"
#include "System/Systems.h"
#include <typeinfo>

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EnumInfo;
    class PropertyInfo;
    class PropertyPath;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API TypeRegistry : public ISystem
    {
    public:

        EE_SYSTEM( TypeRegistry );

    public:

        TypeRegistry();
        ~TypeRegistry();

        //-------------------------------------------------------------------------
        // Type Info
        //-------------------------------------------------------------------------

        TypeInfo const* RegisterType( TypeInfo const* pType );
        void UnregisterType( TypeInfo const* pType );

        // Returns the type information for a given type ID
        TypeInfo const* GetTypeInfo( TypeID typeID ) const;

        // Returns the type information for a given type
        template<typename T, typename = std::enable_if_t<std::is_base_of<EE::IReflectedType, T>::value>>
        TypeInfo const* GetTypeInfo() const
        {
            return T::s_pTypeInfo;
        }

        // Returns the size of a given type
        size_t GetTypeByteSize( TypeID typeID ) const;

        // Returns the resolved property info for a given path
        PropertyInfo const* ResolvePropertyPath( TypeInfo const* pTypeInfo, PropertyPath const& pathID ) const;

        // Does a given type derive from a given parent type
        bool IsTypeDerivedFrom( TypeID typeID, TypeID parentTypeID ) const;

        // Return all known types
        TVector<TypeInfo const*> GetAllTypes( bool includeAbstractTypes = true, bool sortAlphabetically = false ) const;

        // Return all types that derived from a specified type
        TVector<TypeInfo const*> GetAllDerivedTypes( TypeID parentTypeID, bool includeParentTypeInResults = false, bool includeAbstractTypes = true, bool sortAlphabetically = false ) const;

        // Get all the types that this type is allowed to be cast to
        TInlineVector<TypeID, 5> GetAllCastableTypes( IReflectedType const* pType ) const;

        // Are these two types in the same derivation chain (i.e. does either derive from the other )
        bool AreTypesInTheSameHierarchy( TypeID typeA, TypeID typeB ) const;

        // Are these two types in the same derivation chain (i.e. does either derive from the other )
        bool AreTypesInTheSameHierarchy( TypeInfo const* pTypeInfoA, TypeInfo const* pTypeInfoB ) const;

        //-------------------------------------------------------------------------
        // Enum Info
        //-------------------------------------------------------------------------

        EnumInfo const* RegisterEnum( EnumInfo const& type );
        void UnregisterEnum( TypeID typeID );

        // Returns the enum type information for a given type ID
        EnumInfo const* GetEnumInfo( TypeID enumID ) const;

        // Returns the enum type information for a given type
        template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
        EnumInfo const* GetEnumInfo() const
        {
            char const* pEnumName = typeid( T ).name();
            TypeID const enumTypeID( pEnumName + 5 );
            return GetEnumInfo( enumTypeID );
        }

        //-------------------------------------------------------------------------
        // Resource Info
        //-------------------------------------------------------------------------

        void RegisterResourceTypeID( ResourceInfo const& resourceInfo );
        void UnregisterResourceTypeID( TypeID typeID );

        inline THashMap<TypeID, ResourceInfo> const& GetRegisteredResourceTypes() const { return m_registeredResourceTypes; }
        bool IsRegisteredResourceType( TypeID typeID ) const;
        bool IsRegisteredResourceType( ResourceTypeID resourceTypeID ) const;
        bool IsResourceTypeDerivedFrom( ResourceTypeID resourceTypeID, ResourceTypeID parentResourceTypeID ) const;
        ResourceInfo const* GetResourceInfoForType( TypeID typeID ) const;
        ResourceInfo const* GetResourceInfoForResourceType( ResourceTypeID resourceTypeID ) const;
        TVector<ResourceTypeID> GetAllDerivedResourceTypes( ResourceTypeID resourceTypeID ) const;

    private:

        THashMap<TypeID, TypeInfo const*>       m_registeredTypes;
        THashMap<TypeID, EnumInfo*>             m_registeredEnums;
        THashMap<TypeID, ResourceInfo>          m_registeredResourceTypes;
    };
}
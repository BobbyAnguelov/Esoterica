#pragma once

#include "CoreTypeIDs.h"
#include "Base/Systems.h"
#include "Base/Types/HashMap.h"
#include "Base/Resource/ResourceTypeID.h"
#include <typeinfo>

//-------------------------------------------------------------------------

namespace EE
{
    class IReflectedType;
}

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class TypeInfo;
    class EnumInfo;
    class PropertyInfo;
    class PropertyPath;
    struct ResourceInfo;
    struct DataFileInfo;

    //-------------------------------------------------------------------------

    class EE_BASE_API TypeRegistry : public ISystem
    {
    public:

        EE_SYSTEM( TypeRegistry );

    public:

        TypeRegistry() = default;
        ~TypeRegistry();

        void RegisterInternalTypes();
        void UnregisterInternalTypes();

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

        // Returns the resolved property info for a given path
        PropertyInfo const* ResolvePropertyPath( TypeInfo const* pTypeInfo, PropertyPath const& pathID ) const;

        // Is this type registered?
        inline bool IsRegisteredType( TypeID typeID ) const { return m_registeredTypes.find( typeID ) != m_registeredTypes.end(); }

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
        void UnregisterResourceTypeID( ResourceTypeID typeID );

        inline THashMap<ResourceTypeID, ResourceInfo*> const& GetRegisteredResourceTypes() const { return m_registeredResourceTypes; }
        ResourceInfo const* GetResourceInfo( TypeID typeID ) const;
        ResourceInfo const* GetResourceInfo( ResourceTypeID resourceTypeID ) const;
        bool IsRegisteredResourceType( ResourceTypeID resourceTypeID ) const;
        inline bool IsRegisteredResourceType( TypeID typeID ) const { return GetResourceInfo( typeID ) != nullptr; }
        bool IsResourceTypeDerivedFrom( ResourceTypeID resourceTypeID, ResourceTypeID parentResourceTypeID ) const;
        TVector<ResourceTypeID> GetAllDerivedResourceTypes( ResourceTypeID resourceTypeID ) const;

        //-------------------------------------------------------------------------
        // Data File Info
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void RegisterDataFileInfo( DataFileInfo const& dataFileInfo );
        void UnregisterDataFileInfo( TypeID typeID );

        inline THashMap<TypeID, DataFileInfo*> const& GetRegisteredDataFileTypes() const { return m_registeredDataFileTypes; }
        bool IsRegisteredDataFileType( TypeID typeID ) const;
        bool IsRegisteredDataFileType( uint32_t extFourCC ) const;
        DataFileInfo const* GetDataFileInfo( TypeID typeID ) const;
        #endif

    private:

        THashMap<TypeID, TypeInfo const*>           m_registeredTypes;
        THashMap<TypeID, EnumInfo*>                 m_registeredEnums;
        THashMap<ResourceTypeID, ResourceInfo*>     m_registeredResourceTypes;
        THashMap<TypeID, DataFileInfo*>             m_registeredDataFileTypes;
    };
}
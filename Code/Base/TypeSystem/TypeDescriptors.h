#pragma once
#include "Base/_Module/API.h"
#include "ReflectedType.h"
#include "PropertyPath.h"
#include "CoreTypeIDs.h"
#include "CoreTypeConversions.h"
#include "TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class TypeRegistry;
    class TypeInfo;

    //-------------------------------------------------------------------------
    // Basic descriptor of a reflected property
    //-------------------------------------------------------------------------

    struct EE_BASE_API PropertyDescriptor
    {
        EE_SERIALIZE( m_path, m_byteValue );

    public:

        PropertyDescriptor() = default;

        inline bool IsValid() const { return m_path.IsValid() && !m_byteValue.empty(); }

    public:

        PropertyPath                                                m_path; // The path to the property we are describing (potentially including array indices)
        Blob                                                        m_byteValue;
        String                                                      m_stringValue; // Not serialized
    };

    //-------------------------------------------------------------------------
    // Type Descriptor
    //-------------------------------------------------------------------------
    // A serialized description of a EE type, intended for binary serialization of type data (i.e. compiled data)

    class EE_BASE_API TypeDescriptor
    {
        EE_SERIALIZE( m_typeID, m_properties );

    public:

        // Create a type descriptor from a given reflected instance
        static void DescribeType( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, TypeDescriptor& outDesc );

        // Get all referenced resources for a given type instance
        // Note: referenced resources are appended to the back of the supplied vector. It is the user's responsibility to clear it before calling this function (if needed).
        static void GetAllReferencedResources( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, TVector<ResourceID>& outReferencedResources );

    public:

        TypeDescriptor() = default;
        TypeDescriptor( TypeID typeID ) : m_typeID( typeID ) { EE_ASSERT( m_typeID.IsValid() ); }
        TypeDescriptor( TypeRegistry const& typeRegistry, IReflectedType* pTypeInstance );

        inline bool IsValid() const { return m_typeID.IsValid(); }

        // Type Creation
        //-------------------------------------------------------------------------

        // Create a new instance of the described type!
        template<typename T>
        [[nodiscard]] inline T* CreateType( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo ) const
        {
            EE_ASSERT( pTypeInfo != nullptr && pTypeInfo->m_ID == m_typeID );
            EE_ASSERT( pTypeInfo->IsDerivedFrom<T>() );

            // Create new instance
            IReflectedType* pTypeInstance = pTypeInfo->CreateType();
            EE_ASSERT( pTypeInstance != nullptr );

            // Set properties
            RestorePropertyState( typeRegistry, pTypeInfo, pTypeInstance );
            pTypeInstance->PostDeserialize();
            return reinterpret_cast<T*>( pTypeInstance );
        }

        // Create a new instance of the described type! This function is slower since it has to look up the type info first, if you can prefer using the version above!
        template<typename T>
        [[nodiscard]] inline T* CreateType( TypeRegistry const& typeRegistry ) const
        {
            TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( m_typeID );
            return CreateType<T>( typeRegistry, pTypeInfo );
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template<typename T>
        [[nodiscard]] inline T* CreateTypeInPlace( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, IReflectedType* pAllocatedMemoryForInstance ) const
        {
            EE_ASSERT( pTypeInfo != nullptr && pTypeInfo->m_ID == m_typeID );
            EE_ASSERT( pTypeInfo->IsDerivedFrom<T>() );

            // Create new instance
            EE_ASSERT( pAllocatedMemoryForInstance != nullptr );
            pTypeInfo->CreateTypeInPlace( pAllocatedMemoryForInstance );

            // Set properties
            RestorePropertyState( typeRegistry, pTypeInfo, pAllocatedMemoryForInstance );
            pAllocatedMemoryForInstance->PostDeserialize();
            return reinterpret_cast<T*>( pAllocatedMemoryForInstance );
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template<typename T>
        [[nodiscard]] inline T* CreateTypeInPlace( TypeRegistry const& typeRegistry, void* pAllocatedMemoryForInstance ) const
        {
            TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( m_typeID );
            return CreateTypeInPlace<T>( typeRegistry, pTypeInfo, pAllocatedMemoryForInstance );
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template<typename T>
        inline void CreateTypeInPlace( TypeRegistry const& typeRegistry, T* pTypeInstance ) const
        {
            pTypeInstance->~T();
            TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( m_typeID );
            T* pCreatedType = CreateTypeInPlace<T>( typeRegistry, pTypeInfo, pTypeInstance );
        }

        // Properties
        //-------------------------------------------------------------------------

        PropertyDescriptor* GetProperty( PropertyPath const& path );
        inline PropertyDescriptor const* GetProperty( PropertyPath const& path ) const { return const_cast<TypeDescriptor*>( this )->GetProperty( path ); }
        void RemovePropertyValue( PropertyPath const& path );

    private:

        void* RestorePropertyState( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, IReflectedType* pTypeInstance ) const;

    public:

        TypeID                                                      m_typeID;
        TInlineVector<PropertyDescriptor, 6>                        m_properties;
    };

    //-------------------------------------------------------------------------
    // Type Descriptor Collection
    //-------------------------------------------------------------------------
    // Generally only useful for when serializing a set of types all derived from the same base type
    // This collection can be instantiate in one of two ways
    // * Statically - all types are created in a single contiguous array of memory, this is immutable
    // * Dynamically - each type is individually allocated, these types can be destroyed individually at runtime

    struct EE_BASE_API TypeDescriptorCollection
    {
        EE_SERIALIZE( m_descriptors );

    public:

        template<typename T>
        static void* InstantiateStaticCollection( TypeRegistry const& typeRegistry, TypeDescriptorCollection const& collection, TVector<T*>& outTypes )
        {
            EE_ASSERT( collection.m_typeSizes.size() == collection.m_descriptors.size() ); // Did you forget to run the calculate requirements function?
            EE_ASSERT( collection.m_typeSizes.size() == collection.m_typePaddings.size() ); // Did you forget to run the calculate requirements function?

            void* pRawMemory = EE::Alloc( collection.m_totalRequiredSize, collection.m_requiredAlignment );
            uint8_t* pTypeMemory = (uint8_t*) pRawMemory;
            int32_t const numDescs = (int32_t) collection.m_descriptors.size();
            for ( int32_t i = 0; i < numDescs; i++ )
            {
                pTypeMemory += collection.m_typePaddings[i];
                outTypes.emplace_back( collection.m_descriptors[i].CreateTypeInPlace<T>( typeRegistry, collection.m_typeInfos[i], (IReflectedType*) pTypeMemory ) );
                pTypeMemory += collection.m_typeSizes[i];
            }

            return pRawMemory;
        }

        template<typename T>
        static void DestroyStaticCollection( TVector<T*>& types )
        {
            EE_ASSERT( !types.empty() );
            void* pMemoryBlock = types[0];
            for ( auto pType : types )
            {
                pType->~T();
            }
            EE::Free( pMemoryBlock );
        }

        template<typename T>
        static void InstantiateDynamicCollection( TypeRegistry const& typeRegistry, TypeDescriptorCollection const& collection, TVector<T*>& outTypes )
        {
            int32_t const numDescs = (int32_t) collection.m_descriptors.size();
            for ( int32_t i = 0; i < numDescs; i++ )
            {
                outTypes.emplace_back( collection.m_descriptors[i].CreateType<T>( typeRegistry, collection.m_typeInfos[i] ) );
            }
        }

        template<typename T>
        static void DestroyDynamicCollection( TVector<T*>& types )
        {
            for ( auto& pType : types )
            {
                EE::Delete( pType );
            }
            types.clear();
        }

    public:

        void Reset();

        // Calculates all the necessary information needed to instantiate this collection statically (aka in a single immutable block)
        void CalculateCollectionRequirements( TypeRegistry const& typeRegistry );

    public:

        TVector<TypeDescriptor>                                     m_descriptors;
        int32_t                                                     m_totalRequiredSize = -1;
        int32_t                                                     m_requiredAlignment = -1;
        TVector<TypeInfo const*>                                    m_typeInfos;
        TVector<uint32_t>                                           m_typeSizes;
        TVector<uint32_t>                                           m_typePaddings;
    };
}
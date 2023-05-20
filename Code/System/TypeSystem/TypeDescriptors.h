#pragma once
#include "System/_Module/API.h"
#include "ReflectedType.h"
#include "PropertyPath.h"
#include "CoreTypeIDs.h"
#include "CoreTypeConversions.h"
#include "TypeRegistry.h"

//-------------------------------------------------------------------------
// Basic descriptor of a reflected property
//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class TypeRegistry;
    class TypeInfo;

    struct EE_SYSTEM_API PropertyDescriptor
    {
        EE_SERIALIZE( m_path, m_byteValue );

        PropertyDescriptor() = default;

        PropertyDescriptor( TypeRegistry const& typeRegistry, PropertyPath const& path, PropertyInfo const& propertyInfo, String const& stringValue )
            : m_path( path )
        {
            EE_ASSERT( m_path.IsValid() );
            Conversion::ConvertStringToBinary( typeRegistry, propertyInfo, stringValue, m_byteValue );

            #if EE_DEVELOPMENT_TOOLS
            m_stringValue = stringValue;
            m_typeID = propertyInfo.m_typeID;
            m_templatedArgumentTypeID = propertyInfo.m_templateArgumentTypeID;
            #endif
        }

        PropertyDescriptor( TypeRegistry const& typeRegistry, PropertyPath const& path, TypeID propertyTypeID, TypeID propertyTemplatedArgumentTypeID, String const& stringValue )
            : m_path( path )
        {
            EE_ASSERT( m_path.IsValid() && !stringValue.empty() );
            Conversion::ConvertStringToBinary( typeRegistry, propertyTypeID, propertyTemplatedArgumentTypeID, stringValue, m_byteValue );

            #if EE_DEVELOPMENT_TOOLS
            m_stringValue = stringValue;
            m_typeID = propertyTypeID;
            m_templatedArgumentTypeID = propertyTemplatedArgumentTypeID;
            #endif
        }

        inline bool IsValid() const { return m_path.IsValid() && !m_byteValue.empty(); }

        // Value Access
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        template<typename T>
        inline T GetValue( TypeRegistry const& typeRegistry ) const
        {
            TypeID const coreTypeID = GetCoreTypeID<T>();

            T value;
            ConvertStringToNativeType( typeRegistry, coreTypeID, TypeID(), m_stringValue, value );
            return value;
        }
        #endif

        // Tools only constructors
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        PropertyDescriptor( PropertyPath const& path, char const* pValue, TypeID typeID, TypeID templatedArgumentTypeID = TypeID() )
            : m_path( path )
            , m_stringValue( pValue )
            , m_typeID( typeID )
            , m_templatedArgumentTypeID( templatedArgumentTypeID )
        {
            EE_ASSERT( path.IsValid() );
        }
        #endif

    public:

        PropertyPath                                                m_path;
        Blob                                            m_byteValue;

        // Not-serialized - used in tooling
        #if EE_DEVELOPMENT_TOOLS
        String                                                      m_stringValue;
        TypeID                                                      m_typeID;
        TypeID                                                      m_templatedArgumentTypeID;
        #endif
    };

    //-------------------------------------------------------------------------
    // Type Descriptor
    //-------------------------------------------------------------------------
    // A serialized description of a EE type with all property overrides

    class EE_SYSTEM_API TypeDescriptor
    {
        EE_SERIALIZE( m_typeID, m_properties );

    public:

        TypeDescriptor() = default;
        TypeDescriptor( TypeID typeID ) : m_typeID( typeID ) { EE_ASSERT( m_typeID.IsValid() ); }
        TypeDescriptor( TypeRegistry const& typeRegistry, IReflectedType* pTypeInstance, bool shouldSetPropertyStringValues = false );

        inline bool IsValid() const { return m_typeID.IsValid(); }

        // Descriptor Creation
        //-------------------------------------------------------------------------

        void DescribeTypeInstance( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, bool shouldSetPropertyStringValues );

        // Type Creation
        //-------------------------------------------------------------------------

        // Create a new instance of the described type!
        template<typename T>
        [[nodiscard]] inline T* CreateTypeInstance( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo ) const
        {
            EE_ASSERT( pTypeInfo != nullptr && pTypeInfo->m_ID == m_typeID );
            EE_ASSERT( pTypeInfo->IsDerivedFrom<T>() );

            // Create new instance
            void* pTypeInstance = pTypeInfo->CreateType();
            EE_ASSERT( pTypeInstance != nullptr );

            // Set properties
            SetPropertyValues( typeRegistry, pTypeInfo, pTypeInstance );
            return reinterpret_cast<T*>( pTypeInstance );
        }

        // Create a new instance of the described type! This function is slower since it has to look up the type info first, if you can prefer using the version above!
        template<typename T>
        [[nodiscard]] inline T* CreateTypeInstance( TypeRegistry const& typeRegistry ) const
        {
            TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( m_typeID );
            return CreateTypeInstance<T>( typeRegistry, pTypeInfo );
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template<typename T>
        [[nodiscard]] inline T* CreateTypeInstanceInPlace( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, IReflectedType* pAllocatedMemoryForInstance ) const
        {
            EE_ASSERT( pTypeInfo != nullptr && pTypeInfo->m_ID == m_typeID );
            EE_ASSERT( pTypeInfo->IsDerivedFrom<T>() );

            // Create new instance
            EE_ASSERT( pAllocatedMemoryForInstance != nullptr );
            pTypeInfo->CreateTypeInPlace( pAllocatedMemoryForInstance );

            // Set properties
            SetPropertyValues( typeRegistry, pTypeInfo, pAllocatedMemoryForInstance );
            return reinterpret_cast<T*>( pAllocatedMemoryForInstance );
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template<typename T>
        [[nodiscard]] inline T* CreateTypeInstanceInPlace( TypeRegistry const& typeRegistry, void* pAllocatedMemoryForInstance ) const
        {
            TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( m_typeID );
            return CreateTypeInstanceInPlace<T>( typeRegistry, pTypeInfo, pAllocatedMemoryForInstance );
        }

        // This will create a new instance of the described type in the memory block provided
        // WARNING! Do not use this function on an existing type instance of type T since it will not call the destructor and so will leak, only use on uninitialized memory
        template<typename T>
        inline void CreateTypeInstanceInPlace( TypeRegistry const& typeRegistry, T* pTypeInstance ) const
        {
            pTypeInstance->~T();
            TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( m_typeID );
            T* pCreatedType = CreateTypeInstanceInPlace<T>( typeRegistry, pTypeInfo, pTypeInstance );
        }

        // Properties
        //-------------------------------------------------------------------------

        PropertyDescriptor* GetProperty( PropertyPath const& path );
        inline PropertyDescriptor const* GetProperty( PropertyPath const& path ) const { return const_cast<TypeDescriptor*>( this )->GetProperty( path ); }
        void RemovePropertyValue( PropertyPath const& path );

    private:

        void* SetPropertyValues( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, void* pTypeInstance ) const;

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

    struct EE_SYSTEM_API TypeDescriptorCollection
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
                outTypes.emplace_back( collection.m_descriptors[i].CreateTypeInstanceInPlace<T>( typeRegistry, collection.m_typeInfos[i], (IReflectedType*) pTypeMemory ) );
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
                outTypes.emplace_back( collection.m_descriptors[i].CreateTypeInstance<T>( typeRegistry, collection.m_typeInfos[i] ) );
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
        int32_t                                                       m_totalRequiredSize = -1;
        int32_t                                                       m_requiredAlignment = -1;
        TVector<TypeInfo const*>                                    m_typeInfos;
        TVector<uint32_t>                                             m_typeSizes;
        TVector<uint32_t>                                             m_typePaddings;
    };
}
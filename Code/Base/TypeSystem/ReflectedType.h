#pragma once
#include "TypeInfo.h"

//-------------------------------------------------------------------------
// Type Reflection
//-------------------------------------------------------------------------

namespace EE
{
    namespace TypeSystem
    {
        class TypeRegistry;
        class PropertyInfo;
    }

    // Base type for reflection
    //-------------------------------------------------------------------------
    // Interface to enforce virtual destructors and type-info overrides

    class EE_BASE_API IReflectedType
    {
    public:

        inline static EE::TypeSystem::TypeInfo const* s_pTypeInfo = nullptr;

    public:

        IReflectedType() = default;
        IReflectedType( IReflectedType const& ) = default;
        virtual ~IReflectedType() = default;

        IReflectedType& operator=( IReflectedType const& rhs ) = default;

        virtual EE::TypeSystem::TypeInfo const* GetTypeInfo() const = 0;
        virtual EE::TypeSystem::TypeID GetTypeID() const = 0;

        //-------------------------------------------------------------------------

        // Called whenever we finish deserializing a type
        virtual void PostDeserialize() {}

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Called before a property is changed via a property grid or other similar system
        virtual void PrePropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) {}

        // Called after a property is changed via a property grid or other similar system
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) {}
        #endif
    };

    // Default instance constructor
    //-------------------------------------------------------------------------
    // In some cases, you might need a custom constructor for the default instance
    // If you define a ctor with this argument in your reflected type it will use that ctor instead of the default one
    // e.g. Foo::Foo( DefaultInstanceCtor_t ) { ... }

    enum DefaultInstanceCtor_t { DefaultInstanceCtor };

    // Helper methods
    //-------------------------------------------------------------------------

    template<typename T>
    bool IsOfType( IReflectedType const* pType )
    {
        if ( pType == nullptr )
        {
            return false;
        }

        return pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() );
    }

    // This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
    template<typename T>
    T* Cast( IReflectedType* pType )
    {
        EE_ASSERT( pType != nullptr );
        EE_ASSERT( pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) );
        return reinterpret_cast<T*>( pType );
    }

    // This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
    template<typename T>
    T const* Cast( IReflectedType const* pType )
    {
        EE_ASSERT( pType != nullptr );
        EE_ASSERT( pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) );
        return reinterpret_cast<T const*>( pType );
    }

    // This will try to cast to the specified type but can fail. Also accepts null arguments
    template<typename T>
    T* TryCast( IReflectedType* pType )
    {
        if ( pType != nullptr && pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) )
        {
            return reinterpret_cast<T*>( pType );
        }

        return nullptr;
    }

    // This will try to cast to the specified type but can fail. Also accepts null arguments
    template<typename T>
    T const* TryCast( IReflectedType const* pType )
    {
        if ( pType != nullptr && pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) )
        {
            return reinterpret_cast<T const*>( pType );
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void ValidateStaticTypeInfoPtr( TypeSystem::TypeInfo const* pPtr )
    {
        #if EE_DEVELOPMENT_TOOLS
        if ( pPtr == nullptr )
        {
            EE_TRACE_HALT( "Invalid Typeinfo Ptr: either this type hasn't been reflected, or it is being used across DLL boundaries and hasn't been exported!" );
        }
        #endif
    }
}

//-------------------------------------------------------------------------
// Reflection Macros
//-------------------------------------------------------------------------

// Flag this enum for reflection and expose it to the tools/serializers
#define EE_REFLECT_ENUM

// Flag this type for reflection and expose it to the tools/serializers
#define EE_REFLECT_TYPE( TypeName ) \
        friend EE::TypeSystem::TypeInfo;\
        template<typename T> friend class EE::TypeSystem::TTypeInfo;\
        public: \
        inline static EE::TypeSystem::TypeInfo const* s_pTypeInfo = nullptr; \
        static EE::TypeSystem::TypeID GetStaticTypeID() { ValidateStaticTypeInfoPtr( s_pTypeInfo ); return TypeName::s_pTypeInfo->m_ID; }\
        virtual EE::TypeSystem::TypeInfo const* GetTypeInfo() const override { ValidateStaticTypeInfoPtr( TypeName::s_pTypeInfo ); return TypeName::s_pTypeInfo; }\
        virtual EE::TypeSystem::TypeID GetTypeID() const override { ValidateStaticTypeInfoPtr( TypeName::s_pTypeInfo ); return TypeName::s_pTypeInfo->m_ID; }\

// Flag this member variable (i.e. class property) for reflection and expose it to the tools/serializers
// Note: look at "PropertyMetadata.h" for all the supported metadata
// Example: EE_REFLECT( Category = "Foo", Description = "Bar", ReadOnly, ShowAsStaticArray )
#define EE_REFLECT( ... )

// Flag a class as the module class for that project
#define EE_REFLECT_MODULE
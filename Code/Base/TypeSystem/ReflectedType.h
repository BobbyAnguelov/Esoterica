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

        #if EE_DEVELOPMENT_TOOLS
        // Called before a property is changed via a property grid or other similar system
        virtual void PrePropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) {}

        // Called after a property is changed via a property grid or other similar system
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) {}
        #endif
    };

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
        static EE::TypeSystem::TypeID GetStaticTypeID() { EE_ASSERT( s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo->m_ID; }\
        virtual EE::TypeSystem::TypeInfo const* GetTypeInfo() const override { EE_ASSERT( TypeName::s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo; }\
        virtual EE::TypeSystem::TypeID GetTypeID() const override { EE_ASSERT( TypeName::s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo->m_ID; }\

// Flag this member variable (i.e. class property) for reflection and expose it to the tools/serializers
// Users can fill the contents with a JSON string to define per variable meta data
//
// Currently Supported Meta Data:
// * "Category" : "Lorem Ipsum" - Create a category for this property
// * "Description" : "Lorem Ipsum" - Provides tooltip help text for this property
// * "IsToolsReadOnly" : true/false - Allows the tools to edit this property, by default all properties are writable
// * "ShowAsStaticArray" : true/false - Removes the resizing controls from a dynamic array
// * "CustomEditor" : "MyEditorID" - Allows for a custom editor to be used in the property grid without creating a new type
#define EE_REFLECT( ... )

// Flag a class as the module class for that project
#define EE_REFLECT_MODULE \
        public: \
        static void RegisterTypes( TypeSystem::TypeRegistry& typeRegistry ); \
        static void UnregisterTypes( TypeSystem::TypeRegistry& typeRegistry );
#pragma once
#include "TypeInfo.h"

//-------------------------------------------------------------------------
// Prevent Reflection
//-------------------------------------------------------------------------

#define EE_REFLECTION_IGNORE_HEADER

//-------------------------------------------------------------------------
// Enum Registration
//-------------------------------------------------------------------------

#define EE_REGISTER_ENUM 

//-------------------------------------------------------------------------
// Type Registration
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
    class EE_SYSTEM_API IRegisteredType
    {
    public:

        static EE::TypeSystem::TypeInfo const* s_pTypeInfo;

    public:

        virtual ~IRegisteredType() = default;
        virtual EE::TypeSystem::TypeInfo const* GetTypeInfo() const = 0;
        virtual EE::TypeSystem::TypeID GetTypeID() const = 0;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) {}
        #endif
    };

    //-------------------------------------------------------------------------

    template<typename T>
    bool IsOfType( IRegisteredType const* pType )
    {
        EE_ASSERT( pType != nullptr );
        return pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() );
    }

    // This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
    template<typename T>
    T* Cast( IRegisteredType* pType )
    {
        EE_ASSERT( pType != nullptr );
        EE_ASSERT( pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) );
        return static_cast<T*>( pType );
    }

    // This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
    template<typename T>
    T const* Cast( IRegisteredType const* pType )
    {
        EE_ASSERT( pType != nullptr );
        EE_ASSERT( pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) );
        return static_cast<T const*>( pType );
    }

    // This will try to cast to the specified type but can fail. Also accepts null arguments
    template<typename T>
    T* TryCast( IRegisteredType* pType )
    {
        if ( pType != nullptr && pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) )
        {
            return static_cast<T*>( pType );
        }

        return nullptr;
    }

    // This will try to cast to the specified type but can fail. Also accepts null arguments
    template<typename T>
    T const* TryCast( IRegisteredType const* pType )
    {
        if ( pType != nullptr && pType->GetTypeInfo()->IsDerivedFrom( T::GetStaticTypeID() ) )
        {
            return static_cast<T const*>( pType );
        }

        return nullptr;
    }
}

#define EE_REGISTER_TYPE( TypeName ) \
        friend EE::TypeSystem::TypeInfo;\
        template<typename T> friend class EE::TypeSystem::TTypeInfo;\
        public: \
        static EE::TypeSystem::TypeInfo const* s_pTypeInfo; \
        static EE::TypeSystem::TypeID GetStaticTypeID() { EE_ASSERT( s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo->m_ID; }\
        virtual EE::TypeSystem::TypeInfo const* GetTypeInfo() const override { EE_ASSERT( TypeName::s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo; }\
        virtual EE::TypeSystem::TypeID GetTypeID() const override { EE_ASSERT( TypeName::s_pTypeInfo != nullptr ); return TypeName::s_pTypeInfo->m_ID; }\


#define EE_REGISTER_TYPE_RESOURCE( ResourceTypeFourCC, ResourceFriendlyName, TypeName ) \
        EE_REGISTER_RESOURCE( ResourceTypeFourCC, ResourceFriendlyName )\
        EE_REGISTER_TYPE( TypeName )

//-------------------------------------------------------------------------
// Property Registration
//-------------------------------------------------------------------------
// TODO: extend clang so that we can use attributes instead of this macro shit

// Register this property but dont expose it to the property grid/editors
#define EE_REGISTER

// Register this property and expose it to the property grid/editors
#define EE_EXPOSE

//-------------------------------------------------------------------------
// Module Registration
//-------------------------------------------------------------------------

#define EE_REGISTER_MODULE \
        public: \
        static void RegisterTypes( TypeSystem::TypeRegistry& typeRegistry ); \
        static void UnregisterTypes( TypeSystem::TypeRegistry& typeRegistry );
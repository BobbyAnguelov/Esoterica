#pragma once
#include "Base/_Module/API.h"
#include "ReflectedType.h"
#include "TypeID.h"
#include "TypeInfo.h"

//-------------------------------------------------------------------------

namespace EE
{
    class IReflectedType;
    namespace TypeSystem { class TypeInfo; }

    // Type Instance
    //-------------------------------------------------------------------------
    // Container to hold an dynamically instantiated reflected type instance
    // NOTE:    Since this holds a ptr to a type, copying this container around is complicated
    //          As such, all copies are semi-destructive as well as potentially expensive 
    //          as they will require a new allocation in the target object.
    //
    //          Equality and copy operators WILL ONLY work on reflected members, 
    //          any unreflected state will be invisible and ignored!

    class EE_BASE_API TypeInstance
    {
    public:

        // Empty TypeInstance
        TypeInstance() = default;

        // Create a TypeInstance wrapper from an already allocated instance - transfers ownership to the TypeInstance container
        TypeInstance( IReflectedType* pInstance );

        // Will deallocate the instance if set
        virtual ~TypeInstance();

        // Creates an instance of the same type and copies all reflected properties from the supplied type
        // Note: This operation ignores any unreflected state!
        TypeInstance( TypeInstance const& rhs );

        // Moves the instance ptr from one container to another
        TypeInstance( TypeInstance&& rhs );

        // Copy instance and all data
        TypeInstance& CopyFrom( TypeInstance const& rhs );

        // Copy instance and all data
        TypeInstance& CopyFrom( IReflectedType const* pInstance );

        // Creates an instance of the same type and copies all reflected properties from the supplied type
        // Note: This operation ignores any unreflected state!
        TypeInstance& operator=( TypeInstance const& rhs ) { return CopyFrom( rhs ); }

        // Moves the instance ptr from one container to another
        TypeInstance& operator=( TypeInstance&& rhs );

        // Do we have a created instance?
        inline bool IsSet() const { return m_pInstance != nullptr; }

        // Direct instance comparison
        inline bool operator==( IReflectedType const* pRHS ) const { return m_pInstance == pRHS; }

        // Direct instance comparison
        inline bool operator!=( IReflectedType const* pRHS ) const { return m_pInstance != pRHS; }

        // Check if all reflected properties are not equal
        bool AreTypesAndPropertyValuesEqual( TypeInstance const& rhs ) const;

        // What is the type ID for the created instance (returns an invalid type ID if no instance exists)
        TypeSystem::TypeID GetInstanceTypeID() const;

        // Get the typeinfo for the created instance (returns nullptr if no instance exists)
        inline TypeSystem::TypeInfo const* GetInstanceTypeInfo() const;

        // Get the created instance (if it exists)
        inline IReflectedType* Get() { return m_pInstance; }

        // Get the created instance (if it exists)
        inline IReflectedType const* Get() const { return m_pInstance; }

        // Get this node as a derived or parent type. This will assert if you try to cast to an invalid type
        template<typename U>
        inline U* GetAs() { return Cast<U>( m_pInstance ); }

        // Get this node as a derived or parent type. This will assert if you try to cast to an invalid type
        template<typename U>
        inline U const* GetAs() const { return Cast<U>( m_pInstance ); }

        // Get this node as a derived or parent type. This will return nullptr if you try to cast to an invalid type
        template<typename U>
        inline U* TryGetAs() { return TryCast<U>( m_pInstance ); }

        // Get this node as a derived or parent type. This will return nullptr if you try to cast to an invalid type
        template<typename U>
        inline U const* TryGetAs() const { return TryCast<U>( m_pInstance ); }

        // Get the base class typeinfo
        virtual TypeSystem::TypeInfo const* GetAllowedBaseTypeInfo() const;

        // Set the instance (will deallocate the current instance if already set)
        // This will take full ownership of the passed in instance!
        void SetInstance( IReflectedType* pNewInstance );

        // Create a new instance of a given type
        void CreateInstance( TypeSystem::TypeInfo const* pTypeInfo )
        {
            EE_ASSERT( pTypeInfo != nullptr );
            EE_ASSERT( !pTypeInfo->IsAbstractType() );
            EE_ASSERT( pTypeInfo->IsDerivedFrom( GetAllowedBaseTypeInfo()->m_ID ) );

            EE::Delete( m_pInstance );
            m_pInstance = pTypeInfo->CreateType();
        }

        // Explicitly create a class instance
        template< typename T, typename ... ConstructorParams >
        void CreateInstance( ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<IReflectedType, T>::value, "Instance types must be derived from IReflectedType" );
            EE_ASSERT( T::s_pTypeInfo != nullptr );
            EE_ASSERT( T::s_pTypeInfo->IsDerivedFrom( GetAllowedBaseTypeInfo()->m_ID ) );

            EE::Delete( m_pInstance );
            m_pInstance = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
        }

        // Destroy the currently set instance
        void DestroyInstance();

    protected:

        IReflectedType* m_pInstance = nullptr;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TTypeInstance final : public TypeInstance
    {
    public:

        using TypeInstance::TypeInstance;

        TTypeInstance( T* pInstance ) : TypeInstance() { m_pInstance = pInstance; }

        // Creates an instance of the same type and copies all reflected properties from the supplied type
        // Note: This operation ignores any unreflected state!
        TTypeInstance( TypeInstance const& rhs )
        {
            EE_ASSERT( rhs.m_pInstance == nullptr || rhs.m_pInstance->GetTypeInfo()->IsDerivedFrom( T::s_pTypeInfo->m_ID ) );
            m_pInstance = rhs.GetInstanceTypeInfo()->CreateType();
            m_pInstance->GetTypeInfo()->CopyProperties( m_pInstance, rhs.m_pInstance );
        }

        // Creates an instance of the same type and copies all reflected properties from the supplied type
        // Note: This operation ignores any unreflected state!
        TTypeInstance( TTypeInstance const& rhs )
        {
            m_pInstance = rhs.GetInstanceTypeInfo()->CreateType();
            m_pInstance->GetTypeInfo()->CopyProperties( m_pInstance, rhs.m_pInstance );
        }

        // Moves the instance ptr from one container to another
        TTypeInstance( TypeInstance&& rhs )
            : TypeInstance( rhs.m_pInstance )
        {
            EE_ASSERT( rhs.m_pInstance == nullptr || rhs.m_pInstance->GetTypeInfo()->IsDerivedFrom( T::s_pTypeInfo->m_ID ) );
            rhs.m_pInstance = nullptr;
        }

        // Moves the instance ptr from one container to another
        TTypeInstance( TTypeInstance&& rhs )
            : TypeInstance( rhs.m_pInstance )
        {
            rhs.m_pInstance = nullptr;
        }

        // Creates an instance of the same type and copies all reflected properties from the supplied type
        // Note: This operation ignores any unreflected state!
        TTypeInstance& operator=( TypeInstance const& rhs ) { return CopyFrom( rhs ); }

        // Creates an instance of the same type and copies all reflected properties from the supplied type
        // Note: This operation ignores any unreflected state!
        TTypeInstance& operator=( TTypeInstance const& rhs )
        {
            EE_ASSERT( rhs.m_pInstance == nullptr || rhs.m_pInstance->GetTypeInfo()->IsDerivedFrom( T::s_pTypeInfo->m_ID ) );
            CopyFrom( rhs );
            return *this;
        }

        // Moves the instance ptr from one container to another
        TTypeInstance& operator=( TypeInstance&& rhs )
        {
            EE_ASSERT( rhs.m_pInstance == nullptr || rhs.m_pInstance->GetTypeInfo()->IsDerivedFrom( T::s_pTypeInfo->m_ID ) );
            EE::Delete( m_pInstance );
            m_pInstance = rhs.m_pInstance;
            rhs.m_pInstance = nullptr;
            return *this;
        }

        // Moves the instance ptr from one container to another
        TTypeInstance& operator=( TTypeInstance&& rhs )
        {
            EE::Delete( m_pInstance );
            m_pInstance = rhs.m_pInstance;
            rhs.m_pInstance = nullptr;
            return *this;
        }

        //-------------------------------------------------------------------------

        virtual TypeSystem::TypeInfo const* GetAllowedBaseTypeInfo() const { return T::s_pTypeInfo; }

        // Accessors
        inline T* Get() { return static_cast<T*>( m_pInstance ); }
        inline T const* Get() const { return static_cast<T const*>( m_pInstance ); }
        inline T* operator->() { EE_ASSERT( IsSet() ); return (T*) m_pInstance; }
        inline T const* operator->() const { EE_ASSERT( IsSet() ); return (T const*) m_pInstance; }

        // Direct instance comparison
        template<typename U>
        bool operator==( U const* pRHS ) const { return m_pInstance == pRHS; }

        // Direct instance comparison
        template<typename U>
        bool operator!=( U const* pRHS ) const { return m_pInstance != pRHS; }

        // Direct instance comparison
        template<typename U>
        inline bool operator==( TTypeInstance<U> const& RHS ) const { return m_pInstance == RHS.m_pInstance; }

        // Direct instance comparison
        template<typename U>
        inline bool operator!=( TTypeInstance<U> const& RHS ) const { return m_pInstance != RHS.m_pInstance; }

        // Create an instance of the specified type
        // WARNING! Be very careful with this call for reflected component properties - DO NOT call this in any of the component entity system lifetime functions
        void CreateInstance() { TypeInstance::CreateInstance<T>(); }

        // Explicitly create a new instance
        // WARNING! Be very careful with this call for reflected component properties - DO NOT call this in any of the component entity system lifetime functions
        template<typename U, typename ... ConstructorParams>
        void CreateInstance( ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<T, U>::value, "Invalid instance type - not derived from the specified base class" );
            TypeInstance::CreateInstance<U>( eastl::forward<ConstructorParams>( params )... );
        }

        // Create derived class instance
        // WARNING! Be very careful with this call for reflected component properties - DO NOT call this in any of the component entity system lifetime functions
        void CreateInstance( TypeSystem::TypeInfo const* pTypeInfo )
        {
            EE_ASSERT( pTypeInfo->IsDerivedFrom( T::s_pTypeInfo->m_ID ) );
            TypeInstance::CreateInstance( pTypeInfo );
        }

    private:

        TTypeInstance( IReflectedType* pInstance ) = delete;
    };
}
#pragma once

#include "PropertyInfo.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/HashMap.h"
#include "Base/Types/LoadingStatus.h"

//-------------------------------------------------------------------------

namespace EE
{
    class IReflectedType;
    class ResourceTypeID;
    class ResourceID;
}

namespace EE::Resource
{
    class ResourceSystem;
    class ResourceRequesterID;
}

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EE_BASE_API TypeInfo
    {

    public:

        TypeInfo() = default;
        TypeInfo( TypeInfo const& ) = default;
        virtual ~TypeInfo() = default;

        TypeInfo& operator=( TypeInfo const& rhs ) = default;

        inline IReflectedType const* GetDefaultInstance() const { return m_pDefaultInstance; }

        // Basic Type Info
        //-------------------------------------------------------------------------

        inline char const* GetTypeName() const { return m_ID.ToStringID().c_str(); }

        #if EE_DEVELOPMENT_TOOLS
        inline char const* GetFriendlyTypeName() const { return m_friendlyName.c_str(); }
        inline char const* GetCategoryName() const { return m_category.c_str(); }
        inline bool HasCategoryName() const { return !m_category.empty(); }
        #endif

        bool IsAbstractType() const { return m_isAbstract; }

        bool IsDerivedFrom( TypeID const parentTypeID ) const;

        template<typename T>
        inline bool IsDerivedFrom() const { return IsDerivedFrom( T::GetStaticTypeID() ); }

        // Property Info
        //-------------------------------------------------------------------------

        PropertyInfo* GetPropertyInfo( StringID propertyID );

        PropertyInfo const* GetPropertyInfo( StringID propertyID ) const { return const_cast<TypeInfo*>( this )->GetPropertyInfo( propertyID ); }

        // Function declaration for generated property registration functions
        template<typename T>
        void RegisterProperties( IReflectedType const* pDefaultTypeInstance )
        {
            EE_HALT(); // Default implementation should never be called
        }

        // Triggers a reflected property copy for the actual type, kinda like static_cast<ActualType*>( pTypeInstance )->operator=( *static_cast<ActualType*>( pRHS ) )
        virtual void CopyProperties( IReflectedType* pTypeInstance, IReflectedType const* pRHS ) const = 0;

        // Type Factories
        //-------------------------------------------------------------------------

        // Create a new instance of this type
        virtual IReflectedType* CreateType() const = 0;

        // Create a new instance of this type in a pre-allocated location
        virtual void CreateTypeInPlace( IReflectedType* pAllocatedMemory ) const = 0;

        // Reset an already created instance of a type - calls the destructor of the current instance and creates a new instance in place;
        virtual void ResetType( IReflectedType* pTypeInstance ) const = 0;

        // Resource Helpers
        //-------------------------------------------------------------------------

        virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const = 0;
        virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const = 0;
        virtual LoadingStatus GetResourceLoadingStatus( IReflectedType* pType ) const = 0;
        virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType* pType ) const = 0;
        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType* pType, uint64_t propertyID ) const = 0;
        virtual void GetReferencedResources( IReflectedType const* pType, TVector<ResourceID>& outReferencedResources ) const = 0;

        // Array helpers
        //-------------------------------------------------------------------------

        virtual uint8_t* GetArrayElementDataPtr( IReflectedType* pTypeInstance, uint64_t arrayID, size_t arrayIdx ) const = 0;
        virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint64_t arrayID ) const = 0;
        virtual size_t GetArrayElementSize( uint64_t arrayID ) const = 0;
        virtual void SetArraySize( IReflectedType* pTypeInstance, uint64_t arrayID, size_t size ) const = 0;
        virtual void ClearArray( IReflectedType* pTypeInstance, uint64_t arrayID ) const = 0;
        virtual void AddArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID ) const = 0;
        virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID, size_t insertIdx ) const = 0;
        virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const = 0;
        virtual void RemoveArrayElement( IReflectedType* pTypeInstance, uint64_t arrayID, size_t elementIdx ) const = 0;

        // Default value helpers
        //-------------------------------------------------------------------------

        virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const = 0;
        virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint64_t propertyID, int32_t arrayIdx = InvalidIndex ) const = 0;
        virtual void ResetToDefault( IReflectedType* pTypeInstance, uint64_t propertyID ) const = 0;

        inline bool AreAllPropertiesSetToDefault( IReflectedType const* pTypeInstance ) const
        {
            return AreAllPropertyValuesEqual( pTypeInstance, m_pDefaultInstance );
        }

        inline bool IsPropertyValueSetToDefault( IReflectedType const* pTypeInstance, uint64_t propertyID, int32_t arrayIdx = InvalidIndex ) const
        {
            return IsPropertyValueEqual( pTypeInstance, m_pDefaultInstance, propertyID, arrayIdx );
        }

    public:

        TypeID                                  m_ID;
        IReflectedType const*                   m_pDefaultInstance;
        TypeInfo const*                         m_pParentTypeInfo = nullptr;
        TVector<PropertyInfo>                   m_properties;
        THashMap<StringID, int32_t>             m_propertyMap;
        int32_t                                 m_size = -1;
        int32_t                                 m_alignment = -1;
        bool                                    m_isAbstract = false;

        #if EE_DEVELOPMENT_TOOLS
        bool                                    m_isForDevelopmentUseOnly = false;      // Whether this type only exists in development builds
        String                                  m_friendlyName;
        String                                  m_namespace; // The internal namespace for this type ( i.e. no engine namespace at the start)
        String                                  m_category;
        #endif
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TTypeInfo : public TypeInfo
    {};
}
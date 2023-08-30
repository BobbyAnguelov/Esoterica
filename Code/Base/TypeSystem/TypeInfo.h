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

        PropertyInfo const* GetPropertyInfo( StringID propertyID ) const;

        // Function declaration for generated property registration functions
        template<typename T>
        void RegisterProperties( IReflectedType const* pDefaultTypeInstance )
        {
            EE_HALT(); // Default implementation should never be called
        }

        // Type Factories
        //-------------------------------------------------------------------------

        virtual IReflectedType* CreateType() const = 0;
        virtual void CreateTypeInPlace( IReflectedType* pAllocatedMemory ) const = 0;

        // Resource Helpers
        //-------------------------------------------------------------------------

        virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const = 0;
        virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const = 0;
        virtual LoadingStatus GetResourceLoadingStatus( IReflectedType* pType ) const = 0;
        virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType* pType ) const = 0;
        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType* pType, uint32_t propertyID ) const = 0;
        virtual void GetReferencedResources( IReflectedType* pType, TVector<ResourceID>& outReferencedResources ) const = 0;

        // Array helpers
        //-------------------------------------------------------------------------

        virtual uint8_t* GetArrayElementDataPtr( IReflectedType* pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const = 0;
        virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint32_t arrayID ) const = 0;
        virtual size_t GetArrayElementSize( uint32_t arrayID ) const = 0;
        virtual void ClearArray( IReflectedType* pTypeInstance, uint32_t arrayID ) const = 0;
        virtual void AddArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID ) const = 0;
        virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t insertIdx ) const = 0;
        virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const = 0;
        virtual void RemoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t elementIdx ) const = 0;

        // Default value helpers
        //-------------------------------------------------------------------------

        virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const = 0;
        virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const = 0;
        virtual void ResetToDefault( IReflectedType* pTypeInstance, uint32_t propertyID ) const = 0;

        inline bool AreAllPropertiesSetToDefault( IReflectedType const* pTypeInstance ) const
        {
            return AreAllPropertyValuesEqual( pTypeInstance, m_pDefaultInstance );
        }

        inline bool IsPropertyValueSetToDefault( IReflectedType const* pTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const
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
        bool                                    m_isForDevelopmentUseOnly = false;      // Whether this property only exists in development builds
        bool                                    m_isToolsReadOnly = false; // Whether this property can be modified by any tools or if it's just a serializable value
        String                                  m_friendlyName;
        String                                  m_category;
        #endif
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TTypeInfo : public TypeInfo
    {};
}
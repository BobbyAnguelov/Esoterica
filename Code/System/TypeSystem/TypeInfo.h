#pragma once

#include "PropertyInfo.h"
#include "System/Types/Arrays.h"
#include "System/Types/HashMap.h"
#include "System/Types/LoadingStatus.h"

//-------------------------------------------------------------------------

namespace EE
{
    class IRegisteredType;
    class ResourceTypeID;
}

namespace EE::Resource
{
    class ResourceSystem;
    class ResourceRequesterID;
}

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class ITypeDataManager;

    //-------------------------------------------------------------------------

    enum class TypeInfoMetaData
    {
        Abstract,
        Entity,
        EntitySystem,
        EntityComponent,
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API TypeInfo
    {

    public:

        TypeInfo() = default;

        inline IRegisteredType const* GetDefaultInstance() const { return m_pDefaultInstance; }

        // Basic Type Info
        //-------------------------------------------------------------------------

        inline char const* GetTypeName() const { return m_ID.ToStringID().c_str(); }

        #if EE_DEVELOPMENT_TOOLS
        inline char const* GetFriendlyTypeName() const { return m_friendlyName.c_str(); }
        inline char const* GetCategoryName() const { return m_category.c_str(); }
        inline bool HasCategoryName() const { return !m_category.empty(); }
        #endif

        bool IsAbstractType() const { return m_metadata.IsFlagSet( TypeInfoMetaData::Abstract ); }

        bool IsDerivedFrom( TypeID const parentTypeID ) const;

        template<typename T>
        inline bool IsDerivedFrom() const { return IsDerivedFrom( T::GetStaticTypeID() ); }

        // Property Info
        //-------------------------------------------------------------------------

        PropertyInfo const* GetPropertyInfo( StringID propertyID ) const;

        // Function declaration for generated property registration functions
        template<typename T>
        void RegisterProperties( IRegisteredType const* pDefaultTypeInstance )
        {
            EE_HALT(); // Default implementation should never be called
        }

        // Type Factories
        //-------------------------------------------------------------------------

        virtual IRegisteredType* CreateType() const = 0;
        virtual void CreateTypeInPlace( IRegisteredType* pAllocatedMemory ) const = 0;

        // Resource Helpers
        //-------------------------------------------------------------------------

        virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IRegisteredType* pType ) const = 0;
        virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IRegisteredType* pType ) const = 0;
        virtual LoadingStatus GetResourceLoadingStatus( IRegisteredType* pType ) const = 0;
        virtual LoadingStatus GetResourceUnloadingStatus( IRegisteredType* pType ) const = 0;
        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IRegisteredType* pType, uint32_t propertyID ) const = 0;

        // Array helpers
        //-------------------------------------------------------------------------

        virtual uint8_t* GetArrayElementDataPtr( IRegisteredType* pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const = 0;
        virtual size_t GetArraySize( IRegisteredType const* pTypeInstance, uint32_t arrayID ) const = 0;
        virtual size_t GetArrayElementSize( uint32_t arrayID ) const = 0;
        virtual void ClearArray( IRegisteredType* pTypeInstance, uint32_t arrayID ) const = 0;
        virtual void AddArrayElement( IRegisteredType* pTypeInstance, uint32_t arrayID ) const = 0;
        virtual void RemoveArrayElement( IRegisteredType* pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const = 0;

        // Default value helpers
        //-------------------------------------------------------------------------

        virtual bool AreAllPropertyValuesEqual( IRegisteredType const* pTypeInstance, IRegisteredType const* pOtherTypeInstance ) const = 0;
        virtual bool IsPropertyValueEqual( IRegisteredType const* pTypeInstance, IRegisteredType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const = 0;
        virtual void ResetToDefault( IRegisteredType* pTypeInstance, uint32_t propertyID ) const = 0;

        inline bool AreAllPropertiesSetToDefault( IRegisteredType const* pTypeInstance ) const
        {
            return AreAllPropertyValuesEqual( pTypeInstance, m_pDefaultInstance );
        }

        inline bool IsPropertyValueSetToDefault( IRegisteredType const* pTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const
        {
            return IsPropertyValueEqual( pTypeInstance, m_pDefaultInstance, propertyID, arrayIdx );
        }

    public:

        TypeID                                  m_ID;
        int32_t                                 m_size = -1;
        int32_t                                 m_alignment = -1;
        TBitFlags<TypeInfoMetaData>             m_metadata;
        ITypeDataManager*                       m_pTypeManager = nullptr;
        IRegisteredType const*                  m_pDefaultInstance;
        TVector<TypeInfo const*>                m_parentTypes;
        TVector<PropertyInfo>                   m_properties;
        THashMap<StringID, int32_t>             m_propertyMap;

        #if EE_DEVELOPMENT_TOOLS
        bool                                    m_isForDevelopmentUseOnly = false;      // Whether this property only exists in development builds
        String                                  m_friendlyName;
        String                                  m_category;
        #endif
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TTypeInfo : public TypeInfo
    {};
}
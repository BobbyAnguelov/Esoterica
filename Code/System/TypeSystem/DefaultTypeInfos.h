#pragma once

#include "RegisteredType.h"
#include "TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    template<>
    class TTypeInfo<IRegisteredType> final : public TypeInfo
    {
    public:

        static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )
        {
            IRegisteredType::s_pTypeInfo = EE::New<TTypeInfo<IRegisteredType>>();
            typeRegistry.RegisterType( IRegisteredType::s_pTypeInfo );
        }

        static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )
        {
            typeRegistry.UnregisterType( IRegisteredType::s_pTypeInfo );
            EE::Delete( IRegisteredType::s_pTypeInfo );
        }

    public:

        TTypeInfo()
        {
            m_ID = TypeSystem::TypeID( "EE::IRegisteredType" );
            m_size = sizeof( IRegisteredType );
            m_alignment = alignof( IRegisteredType );
        }

        virtual IRegisteredType* CreateType() const override
        {
            EE_HALT(); // Error! Trying to instantiate an abstract entity system!
            return nullptr;
        }

        virtual void CreateTypeInPlace( IRegisteredType* pAllocatedMemory ) const override
        {
            EE_HALT();
        }

        virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IRegisteredType* pType ) const override
        {}

        virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IRegisteredType* pType ) const override
        {}

        virtual void GetReferencedResources( IRegisteredType* pType, TVector<ResourceID>& outReferencedResources ) const override 
        {}

        virtual LoadingStatus GetResourceLoadingStatus( IRegisteredType* pType ) const override
        {
            return LoadingStatus::Loaded;
        }

        virtual LoadingStatus GetResourceUnloadingStatus( IRegisteredType* pType ) const override
        {
            return LoadingStatus::Unloaded;
        }

        virtual uint8_t* GetArrayElementDataPtr( IRegisteredType* pType, uint32_t arrayID, size_t arrayIdx ) const override
        {
            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        virtual size_t GetArraySize( IRegisteredType const* pTypeInstance, uint32_t arrayID ) const override
        {
            EE_UNREACHABLE_CODE();
            return 0;
        }

        virtual size_t GetArrayElementSize( uint32_t arrayID ) const override
        {
            EE_UNREACHABLE_CODE();
            return 0;
        }

        virtual void ClearArray( IRegisteredType* pTypeInstance, uint32_t arrayID ) const override { EE_UNREACHABLE_CODE(); }
        virtual void AddArrayElement( IRegisteredType* pTypeInstance, uint32_t arrayID ) const override { EE_UNREACHABLE_CODE(); }
        virtual void RemoveArrayElement( IRegisteredType* pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const override { EE_UNREACHABLE_CODE(); }

        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IRegisteredType* pType, uint32_t propertyID ) const override
        {
            EE_UNREACHABLE_CODE();
            return ResourceTypeID();
        }

        virtual bool AreAllPropertyValuesEqual( IRegisteredType const* pTypeInstance, IRegisteredType const* pOtherTypeInstance ) const override { return false; }
        virtual bool IsPropertyValueEqual( IRegisteredType const* pTypeInstance, IRegisteredType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const override { return false; }
        virtual void ResetToDefault( IRegisteredType* pTypeInstance, uint32_t propertyID ) const override {}
    };
}
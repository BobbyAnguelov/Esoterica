#pragma once

#include "ReflectedType.h"
#include "TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    template<>
    class TTypeInfo<IReflectedType> final : public TypeInfo
    {
    public:

        static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )
        {
            IReflectedType::s_pTypeInfo = EE::New<TTypeInfo<IReflectedType>>();
            typeRegistry.RegisterType( IReflectedType::s_pTypeInfo );
        }

        static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )
        {
            typeRegistry.UnregisterType( IReflectedType::s_pTypeInfo );
            EE::Delete( IReflectedType::s_pTypeInfo );
        }

    public:

        TTypeInfo()
        {
            m_ID = TypeSystem::TypeID( "EE::IReflectedType" );
            m_size = sizeof( IReflectedType );
            m_alignment = alignof( IReflectedType );
        }

        virtual IReflectedType* CreateType() const override
        {
            EE_HALT(); // Error! Trying to instantiate an abstract entity system!
            return nullptr;
        }

        virtual void CreateTypeInPlace( IReflectedType* pAllocatedMemory ) const override
        {
            EE_HALT();
        }

        virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override
        {}

        virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override
        {}

        virtual void GetReferencedResources( IReflectedType* pType, TVector<ResourceID>& outReferencedResources ) const override 
        {}

        virtual LoadingStatus GetResourceLoadingStatus( IReflectedType* pType ) const override
        {
            return LoadingStatus::Loaded;
        }

        virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType* pType ) const override
        {
            return LoadingStatus::Unloaded;
        }

        virtual uint8_t* GetArrayElementDataPtr( IReflectedType* pType, uint32_t arrayID, size_t arrayIdx ) const override
        {
            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint32_t arrayID ) const override
        {
            EE_UNREACHABLE_CODE();
            return 0;
        }

        virtual size_t GetArrayElementSize( uint32_t arrayID ) const override
        {
            EE_UNREACHABLE_CODE();
            return 0;
        }

        virtual void ClearArray( IReflectedType* pTypeInstance, uint32_t arrayID ) const override { EE_UNREACHABLE_CODE(); }
        virtual void AddArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID ) const override { EE_UNREACHABLE_CODE(); }
        virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t insertIdx ) const { EE_UNREACHABLE_CODE(); }
        virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const { EE_UNREACHABLE_CODE(); }
        virtual void RemoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const override { EE_UNREACHABLE_CODE(); }

        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType* pType, uint32_t propertyID ) const override
        {
            EE_UNREACHABLE_CODE();
            return ResourceTypeID();
        }

        virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const override { return false; }
        virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const override { return false; }
        virtual void ResetToDefault( IReflectedType* pTypeInstance, uint32_t propertyID ) const override {}
    };
}
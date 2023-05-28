#pragma once

#include "ResourceRecord.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Resource
    {
        //-------------------------------------------------------------------------
        // Generic Resource Ptr
        //-------------------------------------------------------------------------
        // There is no direct access to runtime resources through generic resource ptr
        // You should generally try to avoid using generic resource ptrs

        class ResourcePtr
        {
            friend ResourceRecord;
            friend class ResourceSystem;

            EE_SERIALIZE( m_resourceID );

        public:

            ResourcePtr() = default;
            ResourcePtr( nullptr_t ) {};
            ResourcePtr( ResourceID id ) : m_resourceID( id ) {}
            ResourcePtr( Resource::ResourcePtr const& rhs ) { operator=( rhs ); }
            ResourcePtr( Resource::ResourcePtr&& rhs ) { operator=( eastl::move( rhs ) ); }

            // Is the resource ID set for this ptr - doesnt signify if the resource is loaded
            inline bool IsSet() const { return m_resourceID.IsValid(); }

            inline ResourceID const& GetResourceID() const { return m_resourceID; }
            inline ResourcePath const& GetResourcePath() const { return m_resourceID.GetResourcePath(); }
            inline ResourceTypeID GetResourceTypeID() const { return m_resourceID.GetResourceTypeID(); }

            inline void Clear()
            {
                EE_ASSERT( m_pResourceRecord == nullptr ); // Only allowed to clear unloaded resource ptrs
                m_resourceID.Clear();
            }

            template<typename T>
            inline T* GetPtr() { return (T*) m_pResourceRecord->GetResourceData(); }

            inline bool operator==( nullptr_t ) { return m_pResourceRecord == nullptr; }
            inline bool operator!=( nullptr_t ) { return m_pResourceRecord != nullptr; }
            inline bool operator==( ResourcePtr const& rhs ) const { return m_resourceID == rhs.m_resourceID; }
            inline bool operator!=( ResourcePtr const& rhs ) const { return m_resourceID != rhs.m_resourceID; }

            inline ResourcePtr& operator=( ResourcePtr const& rhs ) 
            {
                // Can't change a loaded resource, unload it first
                EE_ASSERT( m_pResourceRecord == nullptr || m_pResourceRecord->IsUnloaded() );
                m_resourceID = rhs.m_resourceID;
                m_pResourceRecord = rhs.m_pResourceRecord;
                return *this;
            }

            inline ResourcePtr& operator=( ResourcePtr&& rhs )
            {
                m_resourceID = rhs.m_resourceID;
                m_pResourceRecord = rhs.m_pResourceRecord;
                rhs.m_resourceID = ResourceID();
                rhs.m_pResourceRecord = nullptr;
                return *this;
            }

            inline TInlineVector<ResourceID, 4> const& GetInstallDependencies() const
            {
                EE_ASSERT( m_pResourceRecord != nullptr && ( m_pResourceRecord->IsLoaded() || m_pResourceRecord->IsLoading() || m_pResourceRecord->HasLoadingFailed() ) );
                return m_pResourceRecord->GetInstallDependencies();
            }

            // Load status
            //-------------------------------------------------------------------------
            // Note: loading status is a frame delayed, so if you want to know if a resource has been requested (and still to be processed) used the "WasRequested" function

            inline bool WasRequested() const { return m_pResourceRecord != nullptr; }

            inline LoadingStatus GetLoadingStatus() const { return ( m_pResourceRecord != nullptr ) ? m_pResourceRecord->GetLoadingStatus() : LoadingStatus::Unloaded; }
            inline bool IsLoading() const { return GetLoadingStatus() == LoadingStatus::Loading; }
            inline bool IsLoaded() const { return GetLoadingStatus() == LoadingStatus::Loaded; }
            inline bool IsUnloading() const { return GetLoadingStatus() == LoadingStatus::Unloading; }
            inline bool IsUnloaded() const { return GetLoadingStatus() == LoadingStatus::Unloaded; }
            inline bool HasLoadingFailed() const { return GetLoadingStatus() == LoadingStatus::Failed; }

        protected:

            ResourceID                  m_resourceID;
            ResourceRecord const*       m_pResourceRecord = nullptr;
        };
    }

    //-------------------------------------------------------------------------
    // Templatized Resource Ptr
    //-------------------------------------------------------------------------

    template <typename T>
    class TResourcePtr : public Resource::ResourcePtr
    {
        static_assert( std::is_base_of<EE::Resource::IResource, T>::value, "Invalid specialization for TResourcePtr, only classes derived from IResource are allowed." );

        EE_SERIALIZE( m_resourceID );

    public:

        TResourcePtr() : ResourcePtr() {}
        TResourcePtr( nullptr_t ) : ResourcePtr( nullptr ) {}
        TResourcePtr( ResourceID ID ) : Resource::ResourcePtr( ID ) { EE_ASSERT( ID.GetResourceTypeID() == T::GetStaticResourceTypeID() ); }
        TResourcePtr( Resource::ResourcePtr const& otherResourcePtr ) { operator=( otherResourcePtr ); }

        // Move ctor
        TResourcePtr( Resource::ResourcePtr&& otherResourcePtr ) 
        { 
            operator=( otherResourcePtr );
            otherResourcePtr.m_resourceID = ResourceID();
            otherResourcePtr.m_pResourceRecord = nullptr;
        }

        inline bool operator==( nullptr_t ) const { return m_pResourceRecord == nullptr; }
        inline bool operator!=( nullptr_t ) const { return m_pResourceRecord != nullptr; }
        inline bool operator==( ResourcePtr const& rhs ) const { return m_resourceID == rhs.m_resourceID; }
        inline bool operator!=( ResourcePtr const& rhs ) const { return m_resourceID != rhs.m_resourceID; }
        inline bool operator==( TResourcePtr const& rhs ) const { return m_resourceID == rhs.m_resourceID; }
        inline bool operator!=( TResourcePtr const& rhs ) const { return m_resourceID != rhs.m_resourceID; }

        inline T const* operator->() const { EE_ASSERT( m_pResourceRecord != nullptr ); return reinterpret_cast<T const*>( m_pResourceRecord->GetResourceData() ); }
        inline T const* GetPtr() const { EE_ASSERT( m_pResourceRecord != nullptr ); return reinterpret_cast<T const*>( m_pResourceRecord->GetResourceData() ); }

        inline ResourceTypeID GetSpecializedResourceTypeID() const { return T::GetStaticResourceTypeID(); }

        inline TResourcePtr<T>& operator=( ResourcePtr const& rhs )
        {
            // Can't change a loaded resource, unload it first
            EE_ASSERT( m_pResourceRecord == nullptr || m_pResourceRecord->IsUnloaded() );

            if ( rhs.IsSet() )
            {
                if ( rhs.GetResourceTypeID() == T::GetStaticResourceTypeID() )
                {
                    ResourcePtr::operator=( rhs );
                }
                else // Invalid Assignment
                {
                    EE_HALT();
                }
            }
            else
            {
                m_resourceID.Clear();
            }

            return *this;
        }

        template <typename U>
        inline bool operator==( const TResourcePtr<U>& rhs ) { return m_resourceID == rhs.m_resourceID; }

        template <typename U>
        inline bool operator!=( const TResourcePtr<U>& rhs ) { return m_resourceID != rhs.m_resourceID; }
    };
}
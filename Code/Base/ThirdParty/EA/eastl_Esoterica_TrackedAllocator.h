#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"
#include "Base/Memory/Memory.h"
#include <EABase/eabase.h>
#include <EASTL/internal/config.h>

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    EE_BASE_API extern MemoryAllocator g_EASTL;
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <size_t Alignment>
    class TrackedAllocatorBase;

    using TrackedAllocator = TrackedAllocatorBase<EE_DEFAULT_ALIGNMENT>;
    using TrackedAlignedAllocator = TrackedAllocatorBase<32>;

    //-------------------------------------------------------------------------

    template <size_t Alignment>
    bool operator==( TrackedAllocatorBase<Alignment> const& a, TrackedAllocatorBase<Alignment> const& b );

    template <size_t Alignment>
    bool operator!=( TrackedAllocatorBase<Alignment> const& a, TrackedAllocatorBase<Alignment> const& b );

    //-------------------------------------------------------------------------

    template <size_t Alignment>
    class TrackedAllocatorBase
    {
        template <size_t Alignment0>
        friend bool eastl::operator==( TrackedAllocatorBase<Alignment0> const& a, TrackedAllocatorBase<Alignment0> const& b );

        template <size_t Alignment0>
        friend bool eastl::operator!=( TrackedAllocatorBase<Alignment0> const& a, TrackedAllocatorBase<Alignment0> const& b );

    public:

        inline TrackedAllocatorBase( EE::Memory::MemoryAllocator& memoryAllocator )
        {
            #if EE_DEVELOPMENT_TOOLS
            m_pMemoryAllocator = &memoryAllocator;
            #endif
        }

        inline explicit TrackedAllocatorBase( char const* pName = "EASTL allocator" )
        {
            set_name( pName );
        }

        inline TrackedAllocatorBase( TrackedAllocatorBase const& alloc )
            #if EE_DEVELOPMENT_TOOLS
            : m_pMemoryAllocator( alloc.m_pMemoryAllocator )
            #endif
        {
            set_name( alloc.get_name() );
        }

        inline TrackedAllocatorBase( TrackedAllocatorBase const& x, char const* pName )
            #if EE_DEVELOPMENT_TOOLS
            : m_pMemoryAllocator( x.m_pMemoryAllocator )
            #endif
        {
            set_name( pName );
        }

        inline TrackedAllocatorBase& operator=( TrackedAllocatorBase const& x )
        {
            #if EE_DEVELOPMENT_TOOLS
            EE_ASSERT( m_pMemoryAllocator == x.m_pMemoryAllocator ); // DO NOT MIX DIFFERENT ALLOCATORS! This will break the memory tracking system.
            #endif
            return *this;
        }

        inline void* allocate( size_t n, int flags = 0 )
        {
            #if EE_DEVELOPMENT_TOOLS
            return m_pMemoryAllocator->Alloc( n, Alignment );
            #else
            return EE::Memory::Allocators::g_EASTL.Alloc( n, Alignment );
            #endif
        }

        inline void* allocate( size_t n, size_t alignment, size_t offset, int flags = 0 )
        {
            if ( Alignment > alignment ) { alignment = Alignment; }
            EE_ASSERT( ( Alignment % alignment ) == 0 );

            #if EE_DEVELOPMENT_TOOLS
            return m_pMemoryAllocator->Alloc( n, alignment );
            #else
            return EE::Memory::Allocators::g_EASTL.Alloc( n, alignment );
            #endif
        }

        inline void deallocate( void* p, size_t n )
        {
            #if EE_DEVELOPMENT_TOOLS
            m_pMemoryAllocator->Free( p );
            #else
            EE::Memory::Allocators::g_EASTL.Free( p );
            #endif
        }

        inline char const* get_name() const
        {
            #if EE_DEVELOPMENT_TOOLS
            return m_pMemoryAllocator->GetName();
            #else
            return EASTL_ALLOCATOR_DEFAULT_NAME;
            #endif
        }

        inline void set_name( char const* pName )
        {
            // Not used right now
        }

        inline bool Compare( TrackedAllocatorBase const& other ) const
        {
            #if EE_DEVELOPMENT_TOOLS
            return m_pMemoryAllocator == other.m_pMemoryAllocator;
            #else
            return true;
            #endif
        }

    protected:

        #if EE_DEVELOPMENT_TOOLS
        EE::Memory::MemoryAllocator* m_pMemoryAllocator = &EE::Memory::Allocators::g_EASTL;
        #endif
    };

    //-------------------------------------------------------------------------

    template <size_t Alignment>
    inline bool operator==( TrackedAllocatorBase<Alignment> const& a, TrackedAllocatorBase<Alignment> const& b ) { return a.Compare( b ); }

    template <size_t Alignment>
    inline bool operator!=( TrackedAllocatorBase<Alignment> const& a, TrackedAllocatorBase<Alignment> const& b ) { return !a.Compare( b ); }
}
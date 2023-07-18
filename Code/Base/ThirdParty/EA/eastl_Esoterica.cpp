#include "eastl_Esoterica.h"
#include "Base/Memory/Memory.h"
#include <EASTL/internal/config.h>
#include <EASTL/allocator.h>

//-------------------------------------------------------------------------

namespace eastl
{
    allocator g_defaultAllocator;

    //-------------------------------------------------------------------------

    allocator* GetDefaultAllocator()
    {
        return &g_defaultAllocator;
    }

    allocator* SetDefaultAllocator( allocator* pAllocator )
    {
        return &g_defaultAllocator;
    }

    //-------------------------------------------------------------------------

    allocator::allocator( const char* EASTL_NAME( pName ) )
    {
    }

    allocator::allocator( const allocator& EASTL_NAME( alloc ) )
    {
    }

    allocator::allocator( const allocator&, const char* EASTL_NAME( pName ) )
    {
    }

    allocator& allocator::operator=( const allocator& EASTL_NAME( alloc ) )
    {
        return *this;
    }

    const char* allocator::get_name() const
    {
        return EASTL_ALLOCATOR_DEFAULT_NAME;
    }

    void allocator::set_name( const char* EASTL_NAME( pName ) )
    {
    }

    void* allocator::allocate( size_t n, int flags )
    {
        return EE::Alloc( n, EASTL_ALLOCATOR_MIN_ALIGNMENT );
    }

    void* allocator::allocate( size_t n, size_t alignment, size_t offset, int flags )
    {
        return EE::Alloc( n, alignment );
    }

    void allocator::deallocate( void* p, size_t )
    {
        EE::Free( p );
    }
}
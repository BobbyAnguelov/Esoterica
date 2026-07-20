#include "meshoptimizer_esoterica.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    static MemoryAllocator g_meshOptimizer( "MeshOptimizer" );
}

//-------------------------------------------------------------------------

namespace EE::Render::MeshOptimizer
{
    static void* Allocate( size_t size )
    {
        return Memory::Allocators::g_meshOptimizer.Alloc( size, EE_DEFAULT_ALIGNMENT );
    }

    static void Deallocate( void* pMemory )
    {
        Memory::Allocators::g_meshOptimizer.Free( pMemory );
    }

    //-------------------------------------------------------------------------

    void Initialize()
    {
        meshopt_setAllocator( &Allocate, &Deallocate );
    }

    void Shutdown()
    {
        meshopt_setAllocator( nullptr, nullptr );
    }
}

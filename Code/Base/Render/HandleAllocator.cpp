#include "HandleAllocator.h"

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    MemoryAllocator g_handleAllocator_L1( "Handle Allocator L1" );
    MemoryAllocator g_handleAllocator_L2( "Handle Allocator L2" );
    MemoryAllocator g_handleAllocator_Hint( "Handle Allocator Hint" );
}

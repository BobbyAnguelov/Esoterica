#if EE_ENABLE_NAVPOWER
#include "NavPower.h"

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    static MemoryAllocator g_navpower( "Navpower" );
}

//-------------------------------------------------------------------------

namespace EE::Navmesh::NavPower
{
    class Allocator final : public bfx::CustomAllocator
    {
        virtual void* CustomMalloc( size_t size ) override final { return Memory::Allocators::g_navpower.Alloc( size, EE_DEFAULT_ALIGNMENT ); }
        virtual void* CustomAlignedMalloc( uint32_t alignment, size_t size ) override final { return Memory::Allocators::g_navpower.Alloc( size, alignment ); }
        virtual void CustomFree( void* ptr ) override final { Memory::Allocators::g_navpower.Free( ptr ); }
        virtual bool IsThreadSafe() const override final { return true; }
        virtual const char* GetName() const override { return "NavpowerCustomAllocator"; }
    };

    Allocator* g_pAllocator = nullptr;

    //-------------------------------------------------------------------------

    void Initialize()
    {
        EE_ASSERT( g_pAllocator == nullptr );
        g_pAllocator = EE::New<Allocator>();
    }

    void Shutdown()
    {
        EE::Delete( g_pAllocator );
    }

    bfx::CustomAllocator* GetAllocator()
    {
        EE_ASSERT( g_pAllocator != nullptr );
        return g_pAllocator;
    }
}
#endif

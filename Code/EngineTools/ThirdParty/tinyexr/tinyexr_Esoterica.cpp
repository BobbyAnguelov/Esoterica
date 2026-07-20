#include "Base/Memory/Memory.h"
#include "Base/ThirdParty/stb/stb_image.h"
#include "Base/ThirdParty/stb/stb_image_write.h"

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    static MemoryAllocator g_tinyEXR( "Tiny EXR" );
}

//-------------------------------------------------------------------------

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION 1

#pragma warning( disable : 4242 )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 4706 )
#pragma warning( disable : 4800 )

static void* EE_TINYEXR_Calloc( size_t count, size_t size )
{
    void* pPtr = EE::Memory::Allocators::g_tinyEXR.Alloc( count * size, EE_DEFAULT_ALIGNMENT );
    EE::Memory::MemsetZero( pPtr, count * size );
    return pPtr;
}

static void EE_TINYEXR_Free( void* pPtr )
{
    EE::Memory::Allocators::g_tinyEXR.Free( pPtr );
}

#define malloc EE::Alloc
#define calloc EE_TINYEXR_Calloc
#define free EE_TINYEXR_Free
#include "tinyexr.h"
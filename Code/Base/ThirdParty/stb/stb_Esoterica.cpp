#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

#define STBI_MALLOC EE::Alloc
#define STBI_REALLOC EE::Realloc
#define STBI_FREE EE::Free

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    MemoryAllocator g_STB( "STB" );
}

//-------------------------------------------------------------------------

static void* EE_STBIR_Malloc( size_t size, void* )
{
    return EE::Memory::Allocators::g_STB.Alloc( size, EE_DEFAULT_ALIGNMENT );
}

static void EE_STBIR_Free( void* pPtr, void* )
{
    EE::Memory::Allocators::g_STB.Free( pPtr );
}

#define STBIR_MALLOC EE_STBIR_Malloc
#define STBIR_FREE EE_STBIR_Free

//-------------------------------------------------------------------------

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

//-------------------------------------------------------------------------

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//-------------------------------------------------------------------------

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stb_image_resize2.h"

//-------------------------------------------------------------------------

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
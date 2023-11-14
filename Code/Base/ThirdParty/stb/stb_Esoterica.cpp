#include "Base/Memory/Memory.h"

#define STBI_MALLOC EE::Alloc
#define STBI_REALLOC EE::Realloc
#define STBI_FREE EE::Free

//-------------------------------------------------------------------------

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

//-------------------------------------------------------------------------

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
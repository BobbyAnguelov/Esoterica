#include "ImportedImage.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    Image::~Image()
    {
        EE::Free( m_pImageData );
    }
}
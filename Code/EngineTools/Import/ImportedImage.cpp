#include "ImportedImage.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    ImportedImage::~ImportedImage()
    {
        EE::Free( m_pImageData );
    }
}
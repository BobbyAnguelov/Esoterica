#include "Bitmaps.h"
#include "EngineTools/Import/ImportedImage.h"
#include "EngineTools/Import/ImporterSource.h"
#include "Base/Memory/Memory.h"
#include "Base/ThirdParty/stb/stb_image.h"

//-------------------------------------------------------------------------

namespace EE::Import::BitmapImage
{
    class ImportedImage : public Image
    {
        friend class File;
    };

    //-------------------------------------------------------------------------

    File::File( Source const& source )
        : Import::ImageFile( source )
    {
        if ( !Import::ImageFile::IsValid() )
        {
            return;
        }

        m_pImageData = stbi_load_from_memory( m_fileData.data(), (int32_t) m_fileData.size(), &m_width, &m_height, &m_channels, STBI_rgb_alpha );

        if ( m_pImageData == nullptr )
        {
            m_error = stbi_failure_reason();
        }
    }

    File::~File()
    {
        EE::Free( m_pImageData );
    }

    TUniquePtr<Image> File::ReadImage() const
    {
        EE_ASSERT( IsValid() );

        TUniquePtr<Image> image( EE::New<ImportedImage>() );
        ImportedImage* pImportedImage = (ImportedImage*) image.get();

        pImportedImage->m_width = m_width;
        pImportedImage->m_height = m_height;
        pImportedImage->m_hdr = false;
        pImportedImage->m_stride = int32_t( m_width * STBI_rgb_alpha );
        pImportedImage->m_channels = 4;

        size_t const imageDataSize = size_t( m_width ) * m_height * STBI_rgb_alpha; // Always 4 channels (RGBA)
        pImportedImage->m_pImageData = (uint8_t*) EE::Alloc( imageDataSize );
        memcpy( pImportedImage->m_pImageData, m_pImageData, imageDataSize );

        return image;
    }
}
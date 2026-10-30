#include "EXR.h"
#include "EngineTools/ThirdParty/tinyexr/tinyexr.h"
#include "EngineTools/Import/ImportedImage.h"
#include "EngineTools/Import/ImporterSource.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE::Import::EXR
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

        //-------------------------------------------------------------------------
        // TODO: Load EXR directly in half-float format instead of float.

        char const* pErrorMessage = nullptr;
        if ( LoadEXRFromMemory( &m_pImageData, &m_width, &m_height, m_fileData.data(), m_fileData.size(), &pErrorMessage ) != TINYEXR_SUCCESS )
        {
            m_error = String( String::CtorSprintf(), "Failed to load EXR file %s : %s", source.m_path.c_str(), pErrorMessage );
            EE::Free( pErrorMessage );
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
        pImportedImage->m_hdr = true;
        pImportedImage->m_stride = int32_t( m_width * 4 * sizeof( float ) );
        pImportedImage->m_channels = 4;

        size_t const imageDataSize = size_t( m_width ) * m_height * 4 * sizeof( float ); // Always 4 channels (RGBA)
        pImportedImage->m_pImageData = (uint8_t*) EE::Alloc( imageDataSize );
        memcpy( pImportedImage->m_pImageData, m_pImageData, imageDataSize );

        return image;
    }
}
#pragma once

#include "FormatFile.h"

//-------------------------------------------------------------------------

namespace EE::Import::BitmapImage
{
    class EE_ENGINETOOLS_API File final : public Import::ImageFile
    {
    public:

        File( Source const& source );
        ~File();

        virtual bool IsValid() const override { return Import::ImageFile::IsValid() && m_pImageData != nullptr; }

        virtual TUniquePtr<Image> ReadImage() const override;

    private:

        uint8_t*                    m_pImageData = nullptr;
        int32_t                     m_width = 0;
        int32_t                     m_height = 0;
        int32_t                     m_channels = 0;
    };
}

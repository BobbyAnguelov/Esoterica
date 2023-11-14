#pragma once

#include "ImportedData.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API ImportedImage : public ImportedData
    {

    public:

        virtual ~ImportedImage();

        virtual bool IsValid() const override final { return m_pImageData != nullptr; }

        uint8_t const* GetImageData() const { return m_pImageData; }

        int32_t GetNumChannels() const { return m_channels; }

        int32_t GetWidth() const { return m_width; }

        int32_t GetHeight() const { return m_height; }

        Int2 GetDimensions() const { return Int2( m_width, m_height ); }

        int32_t GetStride() const { return m_stride; }

    protected:

        uint8_t*    m_pImageData = nullptr;
        int32_t     m_channels = 0;
        int32_t     m_width = 0;
        int32_t     m_height = 0;
        int32_t     m_stride = 0;
    };
}
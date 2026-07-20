#pragma once

#include "ImportedData.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API Image : public ImportedData
    {
        friend struct Importer;
        friend struct ImporterAccessor;

    public:

        virtual ~Image();

        virtual bool IsValid() const override final { return m_pImageData != nullptr; }

        uint8_t const* GetImageData() const { return m_pImageData; }

        int32_t GetNumChannels() const { return m_channels; }

        int32_t GetWidth() const { return m_width; }

        int32_t GetHeight() const { return m_height; }

        int32_t GetDepth() const { return m_depth; }

        Int2 GetDimensions() const { return Int2( m_width, m_height ); }

        int32_t GetStride() const { return m_stride; }

        inline bool IsHDR() const { return m_hdr; }

        inline bool IsSharedExponent() const { return m_sharedExponent; }

        inline void ReplaceImageData( uint8_t* pNewImageData ) { m_pImageData = pNewImageData; }

    protected:

        uint8_t* m_pImageData = nullptr;
        int32_t  m_channels = 0;
        int32_t  m_width = 1;
        int32_t  m_height = 1;
        int32_t  m_depth = 1;
        int32_t  m_stride = 0;
        bool     m_hdr = false;
        bool     m_sharedExponent = false;
    };
}

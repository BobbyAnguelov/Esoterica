#include "DDS.h"
#include "Base/FileSystem/FileStreams.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE::Import::DDS
{
    // "https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat"
    struct DirectDrawPixelFormat
    {
        uint32_t                m_size;
        uint32_t                m_flags;
        uint8_t                 m_fourcc[4];
        uint32_t                m_rgbBitCount;
        uint32_t                m_redBitMask;
        uint32_t                m_greenBitMask;
        uint32_t                m_blueBitMask;
        uint32_t                m_alphaBitMask;
    };

    // "https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10"
    struct DirectDrawHeader10
    {
        uint32_t                m_dxgiFormat;
        uint32_t                m_resourceDimension;
        uint32_t                m_miscFlag;
        uint32_t                m_arraySize;
        uint32_t                m_miscFlag2;
    };

    // "https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header"
    struct DirectDrawHeader
    {
        uint8_t                 m_magic[4];
        uint32_t                m_size;
        uint32_t                m_flags;
        uint32_t                m_height;
        uint32_t                m_width;
        uint32_t                m_pitchOrLinearSize;
        uint32_t                m_depth;
        uint32_t                m_mipmapCount;
        uint32_t                m_reserved[11];
        DirectDrawPixelFormat   m_pixelFormat;
        uint32_t                m_caps;
        uint32_t                m_caps2;
        uint32_t                m_caps3;
        uint32_t                m_caps4;
        uint32_t                m_reserved2;
        DirectDrawHeader10      m_dxt10;
    };

    enum DXGIFormat : uint32_t
    {
        DXGI_FORMAT_UNKNOWN = 0,
        DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
        DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
        DXGI_FORMAT_R32G32B32A32_UINT = 3,
        DXGI_FORMAT_R32G32B32A32_SINT = 4,
        DXGI_FORMAT_R32G32B32_TYPELESS = 5,
        DXGI_FORMAT_R32G32B32_FLOAT = 6,
        DXGI_FORMAT_R32G32B32_UINT = 7,
        DXGI_FORMAT_R32G32B32_SINT = 8,
        DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
        DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
        DXGI_FORMAT_R16G16B16A16_UNORM = 11,
        DXGI_FORMAT_R16G16B16A16_UINT = 12,
        DXGI_FORMAT_R16G16B16A16_SNORM = 13,
        DXGI_FORMAT_R16G16B16A16_SINT = 14,
        DXGI_FORMAT_R32G32_TYPELESS = 15,
        DXGI_FORMAT_R32G32_FLOAT = 16,
        DXGI_FORMAT_R32G32_UINT = 17,
        DXGI_FORMAT_R32G32_SINT = 18,
        DXGI_FORMAT_R32G8X24_TYPELESS = 19,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
        DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
        DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
        DXGI_FORMAT_R10G10B10A2_UNORM = 24,
        DXGI_FORMAT_R10G10B10A2_UINT = 25,
        DXGI_FORMAT_R11G11B10_FLOAT = 26,
        DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
        DXGI_FORMAT_R8G8B8A8_UNORM = 28,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
        DXGI_FORMAT_R8G8B8A8_UINT = 30,
        DXGI_FORMAT_R8G8B8A8_SNORM = 31,
        DXGI_FORMAT_R8G8B8A8_SINT = 32,
        DXGI_FORMAT_R16G16_TYPELESS = 33,
        DXGI_FORMAT_R16G16_FLOAT = 34,
        DXGI_FORMAT_R16G16_UNORM = 35,
        DXGI_FORMAT_R16G16_UINT = 36,
        DXGI_FORMAT_R16G16_SNORM = 37,
        DXGI_FORMAT_R16G16_SINT = 38,
        DXGI_FORMAT_R32_TYPELESS = 39,
        DXGI_FORMAT_D32_FLOAT = 40,
        DXGI_FORMAT_R32_FLOAT = 41,
        DXGI_FORMAT_R32_UINT = 42,
        DXGI_FORMAT_R32_SINT = 43,
        DXGI_FORMAT_R24G8_TYPELESS = 44,
        DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
        DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
        DXGI_FORMAT_R8G8_TYPELESS = 48,
        DXGI_FORMAT_R8G8_UNORM = 49,
        DXGI_FORMAT_R8G8_UINT = 50,
        DXGI_FORMAT_R8G8_SNORM = 51,
        DXGI_FORMAT_R8G8_SINT = 52,
        DXGI_FORMAT_R16_TYPELESS = 53,
        DXGI_FORMAT_R16_FLOAT = 54,
        DXGI_FORMAT_D16_UNORM = 55,
        DXGI_FORMAT_R16_UNORM = 56,
        DXGI_FORMAT_R16_UINT = 57,
        DXGI_FORMAT_R16_SNORM = 58,
        DXGI_FORMAT_R16_SINT = 59,
        DXGI_FORMAT_R8_TYPELESS = 60,
        DXGI_FORMAT_R8_UNORM = 61,
        DXGI_FORMAT_R8_UINT = 62,
        DXGI_FORMAT_R8_SNORM = 63,
        DXGI_FORMAT_R8_SINT = 64,
        DXGI_FORMAT_A8_UNORM = 65,
        DXGI_FORMAT_R1_UNORM = 66,
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
        DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
        DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
        DXGI_FORMAT_BC1_TYPELESS = 70,
        DXGI_FORMAT_BC1_UNORM = 71,
        DXGI_FORMAT_BC1_UNORM_SRGB = 72,
        DXGI_FORMAT_BC2_TYPELESS = 73,
        DXGI_FORMAT_BC2_UNORM = 74,
        DXGI_FORMAT_BC2_UNORM_SRGB = 75,
        DXGI_FORMAT_BC3_TYPELESS = 76,
        DXGI_FORMAT_BC3_UNORM = 77,
        DXGI_FORMAT_BC3_UNORM_SRGB = 78,
        DXGI_FORMAT_BC4_TYPELESS = 79,
        DXGI_FORMAT_BC4_UNORM = 80,
        DXGI_FORMAT_BC4_SNORM = 81,
        DXGI_FORMAT_BC5_TYPELESS = 82,
        DXGI_FORMAT_BC5_UNORM = 83,
        DXGI_FORMAT_BC5_SNORM = 84,
        DXGI_FORMAT_B5G6R5_UNORM = 85,
        DXGI_FORMAT_B5G5R5A1_UNORM = 86,
        DXGI_FORMAT_B8G8R8A8_UNORM = 87,
        DXGI_FORMAT_B8G8R8X8_UNORM = 88,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
        DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
        DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
        DXGI_FORMAT_BC6H_TYPELESS = 94,
        DXGI_FORMAT_BC6H_UF16 = 95,
        DXGI_FORMAT_BC6H_SF16 = 96,
        DXGI_FORMAT_BC7_TYPELESS = 97,
        DXGI_FORMAT_BC7_UNORM = 98,
        DXGI_FORMAT_BC7_UNORM_SRGB = 99,
        DXGI_FORMAT_AYUV = 100,
        DXGI_FORMAT_Y410 = 101,
        DXGI_FORMAT_Y416 = 102,
        DXGI_FORMAT_NV12 = 103,
        DXGI_FORMAT_P010 = 104,
        DXGI_FORMAT_P016 = 105,
        DXGI_FORMAT_420_OPAQUE = 106,
        DXGI_FORMAT_YUY2 = 107,
        DXGI_FORMAT_Y210 = 108,
        DXGI_FORMAT_Y216 = 109,
        DXGI_FORMAT_NV11 = 110,
        DXGI_FORMAT_AI44 = 111,
        DXGI_FORMAT_IA44 = 112,
        DXGI_FORMAT_P8 = 113,
        DXGI_FORMAT_A8P8 = 114,
        DXGI_FORMAT_B4G4R4A4_UNORM = 115,
        DXGI_FORMAT_P208 = 116,
        DXGI_FORMAT_V208 = 117,
        DXGI_FORMAT_V408 = 118,
    };

    //-------------------------------------------------------------------------

    enum DirectDrawHeaderFlags : uint32_t
    {
        DDSD_CAPS = 0x1,
        DDSD_HEIGHT = 0x2,
        DDSD_WIDTH = 0x4,
        DDSD_PITCH = 0x8,
        DDSD_PIXELFORMAT = 0x1000,
        DDSD_MIPMAPCOUNT = 0x20000,
        DDSD_LINEARSIZE = 0x80000,
        DDSD_DEPTH = 0x800000,
    };

    enum DirectDrawPixelFormatFlags : uint32_t
    {
        DDPF_ALPHAPIXELS = 0x1,
        DDPF_ALPHA = 0x2,
        DDPF_FOURCC = 0x4,
        DDPF_RGB = 0x40,
        DDPF_YUV = 0x200,
        DDPF_LUMINANCE = 0x20000,
    };

    enum DirectDrawCaps : uint32_t
    {
        DDSCAPS_COMPLEX = 0x8,
        DDSCAPS_MIPMAP = 0x400000,
        DDSCAPS_TEXTURE = 0x1000,
    };

    enum DirectDrawCaps2 : uint32_t
    {
        DDSCAPS2_CUBEMAP = 0x200,
        DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
        DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
        DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
        DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
        DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
        DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
        DDSCAPS2_VOLUME = 0x200000,
    };

    enum D3D10ResourceDimension : uint32_t
    {
        D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
        D3D10_RESOURCE_DIMENSION_BUFFER = 1,
        D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
        D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
        D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4,
    };

    enum DirectDrawMiscFlag : uint32_t
    {
        DDS_RESOURCE_MISC_TEXTURECUBE = 0x4,
    };

    enum DirectDrawMiscFlag2 : uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0x0,
        DDS_ALPHA_MODE_STRAIGHT = 0x1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 0x2,
        DDS_ALPHA_MODE_OPAQUE = 0x3,
        DDS_ALPHA_MODE_CUSTOM = 0x4,
    };

    //-------------------------------------------------------------------------

    static uint32_t BlockSize( DXGIFormat dxgiFormat )
    {
        if ( dxgiFormat == DXGI_FORMAT_BC1_TYPELESS || dxgiFormat == DXGI_FORMAT_BC1_UNORM || dxgiFormat == DXGI_FORMAT_BC1_UNORM_SRGB )
        {
            return 8;
        }

        if ( dxgiFormat == DXGI_FORMAT_BC4_TYPELESS || dxgiFormat == DXGI_FORMAT_BC4_UNORM || dxgiFormat == DXGI_FORMAT_BC4_SNORM )
        {
            return 8;
        }

        return 16;
    }

    static bool IsBlockCompressed( DXGIFormat dxgiFormat )
    {
        return dxgiFormat == DXGI_FORMAT_BC1_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC1_UNORM
            || dxgiFormat == DXGI_FORMAT_BC1_UNORM_SRGB
            || dxgiFormat == DXGI_FORMAT_BC2_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC2_UNORM
            || dxgiFormat == DXGI_FORMAT_BC2_UNORM_SRGB
            || dxgiFormat == DXGI_FORMAT_BC3_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC3_UNORM
            || dxgiFormat == DXGI_FORMAT_BC3_UNORM_SRGB
            || dxgiFormat == DXGI_FORMAT_BC4_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC4_UNORM
            || dxgiFormat == DXGI_FORMAT_BC4_SNORM
            || dxgiFormat == DXGI_FORMAT_BC5_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC5_UNORM
            || dxgiFormat == DXGI_FORMAT_BC5_SNORM
            || dxgiFormat == DXGI_FORMAT_BC6H_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC6H_UF16
            || dxgiFormat == DXGI_FORMAT_BC6H_SF16
            || dxgiFormat == DXGI_FORMAT_BC7_TYPELESS
            || dxgiFormat == DXGI_FORMAT_BC7_UNORM
            || dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB;
    }

    static bool IsLegacyYUV( DXGIFormat dxgiFormat )
    {
        return dxgiFormat == DXGI_FORMAT_AYUV
            || dxgiFormat == DXGI_FORMAT_Y410
            || dxgiFormat == DXGI_FORMAT_Y416
            || dxgiFormat == DXGI_FORMAT_NV12
            || dxgiFormat == DXGI_FORMAT_P010
            || dxgiFormat == DXGI_FORMAT_P016
            || dxgiFormat == DXGI_FORMAT_420_OPAQUE
            || dxgiFormat == DXGI_FORMAT_YUY2
            || dxgiFormat == DXGI_FORMAT_Y210
            || dxgiFormat == DXGI_FORMAT_Y216
            || dxgiFormat == DXGI_FORMAT_NV11
            || dxgiFormat == DXGI_FORMAT_AI44
            || dxgiFormat == DXGI_FORMAT_IA44
            || dxgiFormat == DXGI_FORMAT_P8
            || dxgiFormat == DXGI_FORMAT_A8P8
            || dxgiFormat == DXGI_FORMAT_B4G4R4A4_UNORM
            || dxgiFormat == DXGI_FORMAT_P208
            || dxgiFormat == DXGI_FORMAT_V208
            || dxgiFormat == DXGI_FORMAT_V408;
    }

    static bool IsFloatingPoint32Bit( DXGIFormat dxgiFormat )
    {
        return dxgiFormat == DXGI_FORMAT_R32_FLOAT ||
            dxgiFormat == DXGI_FORMAT_R32G32_FLOAT ||
            dxgiFormat == DXGI_FORMAT_R32G32B32_FLOAT ||
            dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT;
    }

    // "https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide"
    static uint32_t BitsPerPixel( DXGIFormat dxgiFormat )
    {
        switch ( dxgiFormat )
        {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS: return 128;
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return 128;
            case DXGI_FORMAT_R32G32B32A32_UINT: return 128;
            case DXGI_FORMAT_R32G32B32A32_SINT: return 128;

            case DXGI_FORMAT_R32G32B32_TYPELESS: return 96;
            case DXGI_FORMAT_R32G32B32_FLOAT: return 96;
            case DXGI_FORMAT_R32G32B32_UINT: return 96;
            case DXGI_FORMAT_R32G32B32_SINT: return 96;

            case DXGI_FORMAT_R16G16B16A16_TYPELESS: return 64;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return 64;
            case DXGI_FORMAT_R16G16B16A16_UNORM: return 64;
            case DXGI_FORMAT_R16G16B16A16_UINT: return 64;
            case DXGI_FORMAT_R16G16B16A16_SNORM: return 64;
            case DXGI_FORMAT_R16G16B16A16_SINT: return 64;
            case DXGI_FORMAT_R32G32_TYPELESS: return 64;
            case DXGI_FORMAT_R32G32_FLOAT: return 64;
            case DXGI_FORMAT_R32G32_UINT: return 64;
            case DXGI_FORMAT_R32G32_SINT: return 64;
            case DXGI_FORMAT_R32G8X24_TYPELESS: return 64;
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return 64;
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return 64;
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return 64;
            case DXGI_FORMAT_Y416: return 64;
            case DXGI_FORMAT_Y210: return 64;
            case DXGI_FORMAT_Y216: return 64;

            case DXGI_FORMAT_R10G10B10A2_TYPELESS: return 32;
            case DXGI_FORMAT_R10G10B10A2_UNORM: return 32;
            case DXGI_FORMAT_R10G10B10A2_UINT: return 32;
            case DXGI_FORMAT_R11G11B10_FLOAT: return 32;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS: return 32;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return 32;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return 32;
            case DXGI_FORMAT_R8G8B8A8_UINT: return 32;
            case DXGI_FORMAT_R8G8B8A8_SNORM: return 32;
            case DXGI_FORMAT_R8G8B8A8_SINT: return 32;
            case DXGI_FORMAT_R16G16_TYPELESS: return 32;
            case DXGI_FORMAT_R16G16_FLOAT: return 32;
            case DXGI_FORMAT_R16G16_UNORM: return 32;
            case DXGI_FORMAT_R16G16_UINT: return 32;
            case DXGI_FORMAT_R16G16_SNORM: return 32;
            case DXGI_FORMAT_R16G16_SINT: return 32;
            case DXGI_FORMAT_R32_TYPELESS: return 32;
            case DXGI_FORMAT_D32_FLOAT: return 32;
            case DXGI_FORMAT_R32_FLOAT: return 32;
            case DXGI_FORMAT_R32_UINT: return 32;
            case DXGI_FORMAT_R32_SINT: return 32;
            case DXGI_FORMAT_R24G8_TYPELESS: return 32;
            case DXGI_FORMAT_D24_UNORM_S8_UINT: return 32;
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return 32;
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT: return 32;
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return 32;
            case DXGI_FORMAT_R8G8_B8G8_UNORM: return 32;
            case DXGI_FORMAT_G8R8_G8B8_UNORM: return 32;
            case DXGI_FORMAT_B8G8R8A8_UNORM: return 32;
            case DXGI_FORMAT_B8G8R8X8_UNORM: return 32;
            case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return 32;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS: return 32;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return 32;
            case DXGI_FORMAT_B8G8R8X8_TYPELESS: return 32;
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return 32;
            case DXGI_FORMAT_AYUV: return 32;
            case DXGI_FORMAT_Y410: return 32;
            case DXGI_FORMAT_YUY2: return 32;
            case DXGI_FORMAT_P010: return 24;
            case DXGI_FORMAT_P016: return 24;
            case DXGI_FORMAT_R8G8_TYPELESS: return 16;
            case DXGI_FORMAT_R8G8_UNORM: return 16;
            case DXGI_FORMAT_R8G8_UINT: return 16;
            case DXGI_FORMAT_R8G8_SNORM: return 16;
            case DXGI_FORMAT_R8G8_SINT: return 16;
            case DXGI_FORMAT_R16_TYPELESS: return 16;
            case DXGI_FORMAT_R16_FLOAT: return 16;
            case DXGI_FORMAT_D16_UNORM: return 16;
            case DXGI_FORMAT_R16_UNORM: return 16;
            case DXGI_FORMAT_R16_UINT: return 16;
            case DXGI_FORMAT_R16_SNORM: return 16;
            case DXGI_FORMAT_R16_SINT: return 16;
            case DXGI_FORMAT_B5G6R5_UNORM: return 16;
            case DXGI_FORMAT_B5G5R5A1_UNORM: return 16;
            case DXGI_FORMAT_A8P8: return 16;
            case DXGI_FORMAT_B4G4R4A4_UNORM: return 16;
            case DXGI_FORMAT_NV12: return 12;
            case DXGI_FORMAT_420_OPAQUE: return 12;
            case DXGI_FORMAT_NV11: return 12;

            case DXGI_FORMAT_R8_TYPELESS: return 8;
            case DXGI_FORMAT_R8_UNORM: return 8;
            case DXGI_FORMAT_R8_UINT: return 8;
            case DXGI_FORMAT_R8_SNORM: return 8;
            case DXGI_FORMAT_R8_SINT: return 8;
            case DXGI_FORMAT_A8_UNORM: return 8;
            case DXGI_FORMAT_AI44: return 8;
            case DXGI_FORMAT_IA44: return 8;
            case DXGI_FORMAT_P8: return 8;
            case DXGI_FORMAT_R1_UNORM: return 1;

            case DXGI_FORMAT_BC1_TYPELESS: return 4;
            case DXGI_FORMAT_BC1_UNORM: return 4;
            case DXGI_FORMAT_BC1_UNORM_SRGB: return 4;
            case DXGI_FORMAT_BC4_TYPELESS: return 4;
            case DXGI_FORMAT_BC4_UNORM: return 4;
            case DXGI_FORMAT_BC4_SNORM: return 4;

            case DXGI_FORMAT_BC2_TYPELESS: return 8;
            case DXGI_FORMAT_BC2_UNORM: return 8;
            case DXGI_FORMAT_BC2_UNORM_SRGB: return 8;
            case DXGI_FORMAT_BC3_TYPELESS: return 8;
            case DXGI_FORMAT_BC3_UNORM: return 8;
            case DXGI_FORMAT_BC3_UNORM_SRGB: return 8;
            case DXGI_FORMAT_BC5_TYPELESS: return 8;
            case DXGI_FORMAT_BC5_UNORM: return 8;
            case DXGI_FORMAT_BC5_SNORM: return 8;
            case DXGI_FORMAT_BC6H_TYPELESS: return 8;
            case DXGI_FORMAT_BC6H_UF16: return 8;
            case DXGI_FORMAT_BC6H_SF16: return 8;
            case DXGI_FORMAT_BC7_TYPELESS: return 8;
            case DXGI_FORMAT_BC7_UNORM: return 8;
            case DXGI_FORMAT_BC7_UNORM_SRGB: return 8;

            default:
            {
                EE_ASSERT( false );
                return 0;
            }
        };
    }

    // "https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide"
    static Int2 PitchAndLinearSize( uint32_t width, uint32_t height, DXGIFormat dxgiFormat )
    {
        if ( IsBlockCompressed( dxgiFormat ) )
        {
            uint32_t row_pitch = Math::Max( 1U, ( width + 3 ) / 4 ) * BlockSize( dxgiFormat );
            uint32_t linear_size = row_pitch * Math::Max( 1U, ( height + 3 ) / 4 );

            return Int2( row_pitch, linear_size );
        }

        if ( IsLegacyYUV( dxgiFormat ) || dxgiFormat == DXGI_FORMAT_R8G8_B8G8_UNORM || dxgiFormat == DXGI_FORMAT_G8R8_G8B8_UNORM )
        {
            uint32_t row_pitch = ( ( width + 1 ) >> 1 ) * 4;
            uint32_t linear_size = row_pitch * height; // TODO: is this correct?

            return Int2( row_pitch, linear_size );
        }

        uint32_t row_pitch = ( width * BitsPerPixel( dxgiFormat ) + 7 ) / 8;
        uint32_t linear_size = row_pitch * height;

        return Int2( row_pitch, linear_size );
    }

    //-------------------------------------------------------------------------

    class DDSImportedImage : public Image
    {
        friend struct DDSReader;
    };

    //-------------------------------------------------------------------------

    struct DDSReader
    {
        static TUniquePtr<Image> ReadImage( FileSystem::Path const& path )
        {
            TUniquePtr<Image> image( EE::New<DDSImportedImage>() );
            DDSImportedImage* pImportedImage = (DDSImportedImage*) image.get();

            FileSystem::InputFileStream inputStream( path );
            if ( !inputStream.IsValid() )
            {
                pImportedImage->LogError( "Failed to load file: %s", path.c_str() );
                return image;
            }

            // Read Header
            //-------------------------------------------------------------------------

            DirectDrawHeader header = {};
            inputStream.Read( &header, sizeof( header ) );

            EE_ASSERT( header.m_magic[0] == 'D' && header.m_magic[1] == 'D' && header.m_magic[2] == 'S' && header.m_magic[3] == ' ' );
            EE_ASSERT( header.m_size == 124 );
            EE_ASSERT( header.m_pixelFormat.m_size == 32 );
            EE_ASSERT( header.m_pixelFormat.m_fourcc[0] == 'D' && header.m_pixelFormat.m_fourcc[1] == 'X' && header.m_pixelFormat.m_fourcc[2] == '1' && header.m_pixelFormat.m_fourcc[3] == '0' );

            bool isCompressed = IsBlockCompressed( DXGIFormat( header.m_dxt10.m_dxgiFormat ) );
            Int2 rowPitchAndLinearSize = PitchAndLinearSize( header.m_width, header.m_height, DXGIFormat( header.m_dxt10.m_dxgiFormat ) );

            if ( isCompressed )
            {
                EE_ASSERT( uint32_t( rowPitchAndLinearSize.m_y ) == header.m_pitchOrLinearSize );
            }
            else
            {
                EE_ASSERT( uint32_t( rowPitchAndLinearSize.m_x ) == header.m_pitchOrLinearSize );
            }

            size_t imageDataSize = rowPitchAndLinearSize.m_y;
            for ( uint32_t mip = 1; mip < header.m_mipmapCount; ++mip )
            {
                Int2 mipPitchOrLinearSize = PitchAndLinearSize( header.m_width >> mip, header.m_height >> mip, DXGIFormat( header.m_dxt10.m_dxgiFormat ) );
                imageDataSize += mipPitchOrLinearSize.m_y;
            }

            imageDataSize *= header.m_depth;
            imageDataSize *= header.m_dxt10.m_arraySize;

            if ( ( header.m_dxt10.m_miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE ) == DDS_RESOURCE_MISC_TEXTURECUBE )
            {
                imageDataSize *= 6;
            }

            // Read Pixel Data
            //-------------------------------------------------------------------------

            std::streampos dataStart = inputStream.GetStream().tellg();
            EE_ASSERT( size_t( dataStart ) == sizeof( DirectDrawHeader ) );

            inputStream.GetStream().seekg( 0, std::ios::end );
            std::streampos dataEnd = inputStream.GetStream().tellg();
            inputStream.GetStream().seekg( dataStart );

            size_t const pixelDataSize = dataEnd - dataStart;
            EE_ASSERT( imageDataSize == pixelDataSize );
            uint8_t* pPixelData = (uint8_t*) EE::Alloc( pixelDataSize );
            inputStream.Read( pPixelData, pixelDataSize );

            // Set image parameters
            //-------------------------------------------------------------------------

            uint32_t const bitsPerPixel = DDS::BitsPerPixel( DDS::DXGIFormat( header.m_dxt10.m_dxgiFormat ) );

            pImportedImage->m_pImageData = pPixelData;
            pImportedImage->m_width = header.m_width;
            pImportedImage->m_height = header.m_height;
            pImportedImage->m_depth = header.m_depth;
            pImportedImage->m_hdr = DDS::IsFloatingPoint32Bit( DDS::DXGIFormat( header.m_dxt10.m_dxgiFormat ) );
            pImportedImage->m_stride = ( pImportedImage->m_width * bitsPerPixel + 7 ) / 8;

            if ( header.m_dxt10.m_dxgiFormat == DDS::DXGI_FORMAT_R9G9B9E5_SHAREDEXP )
            {
                pImportedImage->m_sharedExponent = true;
                pImportedImage->m_channels = 0;
            }
            else
            {
                pImportedImage->m_channels = bitsPerPixel / 8;
            }

            return image;
        }

        static TUniquePtr<Image> ReadImage( Blob const& blob )
        {
            TUniquePtr<Image> image( EE::New<DDSImportedImage>() );
            DDSImportedImage* pImportedImage = (DDSImportedImage*) image.get();
            uint8_t const* pBlobData = blob.data();

            // Check Header
            //-------------------------------------------------------------------------

            DirectDrawHeader const& header = *reinterpret_cast<DirectDrawHeader const*>( pBlobData );

            EE_ASSERT( header.m_magic[0] == 'D' && header.m_magic[1] == 'D' && header.m_magic[2] == 'S' && header.m_magic[3] == ' ' );
            EE_ASSERT( header.m_size == 124 );
            EE_ASSERT( header.m_pixelFormat.m_size == 32 );
            EE_ASSERT( header.m_pixelFormat.m_fourcc[0] == 'D' && header.m_pixelFormat.m_fourcc[1] == 'X' && header.m_pixelFormat.m_fourcc[2] == '1' && header.m_pixelFormat.m_fourcc[3] == '0' );

            bool const isCompressed = IsBlockCompressed( DXGIFormat( header.m_dxt10.m_dxgiFormat ) );
            Int2 const rowPitchAndLinearSize = PitchAndLinearSize( header.m_width, header.m_height, DXGIFormat( header.m_dxt10.m_dxgiFormat ) );

            if ( isCompressed )
            {
                EE_ASSERT( uint32_t( rowPitchAndLinearSize.m_y ) == header.m_pitchOrLinearSize );
            }
            else
            {
                EE_ASSERT( uint32_t( rowPitchAndLinearSize.m_x ) == header.m_pitchOrLinearSize );
            }

            size_t imageDataSize = rowPitchAndLinearSize.m_y;
            for ( uint32_t mip = 1; mip < header.m_mipmapCount; ++mip )
            {
                Int2 mipPitchOrLinearSize = PitchAndLinearSize( header.m_width >> mip, header.m_height >> mip, DXGIFormat( header.m_dxt10.m_dxgiFormat ) );
                imageDataSize += mipPitchOrLinearSize.m_y;
            }

            imageDataSize *= header.m_depth;
            imageDataSize *= header.m_dxt10.m_arraySize;

            if ( ( header.m_dxt10.m_miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE ) == DDS_RESOURCE_MISC_TEXTURECUBE )
            {
                imageDataSize *= 6;
            }

            // Copy Pixel Data
            //-------------------------------------------------------------------------

            size_t const pixelDataSize = blob.size() - sizeof( DirectDrawHeader );
            EE_ASSERT( imageDataSize == pixelDataSize );
            uint8_t* pPixelData = (uint8_t*) EE::Alloc( pixelDataSize );
            memcpy( pPixelData, pBlobData + sizeof( DirectDrawHeader ), pixelDataSize );

            // Set image parameters
            //-------------------------------------------------------------------------

            uint32_t const bitsPerPixel = DDS::BitsPerPixel( DDS::DXGIFormat( header.m_dxt10.m_dxgiFormat ) );

            pImportedImage->m_pImageData = pPixelData;
            pImportedImage->m_width = header.m_width;
            pImportedImage->m_height = header.m_height;
            pImportedImage->m_depth = header.m_depth;
            pImportedImage->m_hdr = DDS::IsFloatingPoint32Bit( DDS::DXGIFormat( header.m_dxt10.m_dxgiFormat ) );
            pImportedImage->m_stride = ( pImportedImage->m_width * bitsPerPixel + 7 ) / 8;

            if ( header.m_dxt10.m_dxgiFormat == DDS::DXGI_FORMAT_R9G9B9E5_SHAREDEXP )
            {
                pImportedImage->m_sharedExponent = true;
                pImportedImage->m_channels = 0;
            }
            else
            {
                pImportedImage->m_channels = bitsPerPixel / 8;
            }

            return image;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<EE::Import::Image> ReadImage( FileSystem::Path const& path )
    {
        return DDSReader::ReadImage( path );
    }

    TUniquePtr<EE::Import::Image> ReadImage( Blob const& blob )
    {
        return DDSReader::ReadImage( blob );
    }
}
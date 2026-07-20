
#include "Base/Esoterica.h"

#include "RHI.h"

#include "Base/Math/Math.h"
#include "Base/Types/HashMap.h"
#include "Base/Memory/UniquePtr.h"
#include "Base/Encoding/Embed.h"
#include "Base/Render/HandleAllocator.h"

//#define ENABLE_VERBOSE_BUFFER_CREATE 1

// Uncommend to enable verbose memory leak debugging
//#define ENABLE_VERBOSE_MEMORY_LEAK_DEBUG 1

#ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
#include "PageAllocator.h"
#endif

#include "EASTL/algorithm.h"
#include "EASTL/fixed_allocator.h"

#include <Windows.h>
#include <VersionHelpers.h>
#include <dwmapi.h>

#include <d3d12.h>

#include <dxgi.h>
#include <dxgidebug.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>

#pragma warning( push )
#pragma warning( disable : 5204 ) // Warnings in DXC headers
#include "d3d12shader.h"
#include "dxcapi.h"
#pragma warning( pop )

#include <wrl/client.h>

#include <shlobj.h>
#include <strsafe.h>

#ifndef EE_SHIPPING
#define USE_PIX 1 // This is required by the WinPixEventRuntime/pix3.h header file
#define USE_AGS 1
#define USE_RENDERDOC 1
#endif

#include "renderdoc_app.h"
#include "WinPixEventRuntime/pix3.h"

#ifdef USE_AGS
#include "amd_ags.h"
#endif

// This header needs to be included AFTER all other includes and be the last in the source file.
// There are a couple conflicts in the header file.
#include "Base/ThirdParty/D3D12MemoryAllocator/D3D12MemAlloc.h"

//-------------------------------------------------------------------------

// Undef some of the conflicts
#undef CreateSemaphore

#ifdef USE_RENDERDOC
// https://renderdoc.org/docs/behind_scenes/d3d12_support.html
MIDL_INTERFACE( "52528c37-bfd9-4bbb-99ff-fdb7188619ce" )
IRenderDocDescriptorNamer : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SetName( UINT DescriptorIndex, LPCSTR Name ) = 0;
};
#endif

//-------------------------------------------------------------------------

namespace D3D12MA
{
    class Allocator;
    class Allocation;
}

namespace eastl
{
    template <>
    struct hash<EE::TBitFlags<EE::Render::RHI::DescriptorTypeFlags>>
    {
        size_t operator()( EE::TBitFlags<EE::Render::RHI::DescriptorTypeFlags> const& s ) const { return hash<uint32_t>{}( s.Get() ); }
    };
}

namespace EE::Memory::Allocators
{
    static MemoryAllocator g_D3D12( "D3D12" );
    MemoryAllocator g_RHI( "RHI" );
}

//-------------------------------------------------------------------------

namespace EE::Render::RHI
{
    using Microsoft::WRL::ComPtr;

    static_assert( sizeof( IndirectDrawArguments ) == sizeof( D3D12_DRAW_ARGUMENTS ),
                   "IndirectDrawArguments is a hardcoded GPU type and needs to exactly match D3D12_DRAW_ARGUMENTS" );

    static_assert( sizeof( IndirectDrawIndexedArguments ) == ( sizeof( D3D12_DRAW_INDEXED_ARGUMENTS ) ),
                   "IndirectDrawIndexedArguments is a hardcoded GPU type and needs to exactly match D3D12_DRAW_INDEXED_ARGUMENTS" );

    static_assert( sizeof( IndirectDispatchArguments ) == ( sizeof( D3D12_DISPATCH_ARGUMENTS ) ),
                   "IndirectDispatchArguments is a hardcoded GPU type and needs to exactly match D3D12_DISPATCH_ARGUMENTS" );

    static_assert( sizeof( AccelerationStructureInstance ) == sizeof( D3D12_RAYTRACING_INSTANCE_DESC ),
                   "AccelerationStructureInstance is a hardcoded GPU type and needs to exactly match D3D12_RAYTRACING_INSTANCE_DESC" );

    static bool ValidateWindowsVersion()
    {
        if ( !IsWindows10OrGreater() )
        {
            EE_LOG_FATAL_ERROR( "RHI", "D3D12", "This application requires Windows 10 or newer, please update your Windows installation to continue." );
            return false;
        }

        OSVERSIONINFOEX osVersionInfo = {};
        osVersionInfo.dwOSVersionInfoSize = sizeof( osVersionInfo );

        // We are using D3D_ROOT_SIGNATURE_VERSION_1_1 which was introduced with Windows 10 Anniversary Update (14393)
        // This is a hard requirement and we issue an error message early instead of a cryptic crash when serializing root signatures.
        osVersionInfo.dwBuildNumber = 14393;

        DWORDLONG conditionMask = 0;
        VER_SET_CONDITION( conditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL );

        if ( !VerifyVersionInfo( &osVersionInfo, VER_BUILDNUMBER, conditionMask ) )
        {
            EE_LOG_FATAL_ERROR( "RHI", "D3D12", "This application requires Windows 10 Anniversary Update (build 14393) or newer, please update your Windows installation to continue." );
            return false;
        }

        // All good?
        return true;
    }

    #ifdef USE_PIX
    static void LoadPixGpuCapturerDLL()
    {
        LPWSTR programFilesPath = nullptr;
        HRESULT result = SHGetKnownFolderPath( FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath );
        EE_ASSERT( SUCCEEDED( result ) );

        wchar_t pixSearchPath[MAX_PATH] = {};
        result = StringCchCopyW( pixSearchPath, MAX_PATH, programFilesPath );
        EE_ASSERT( SUCCEEDED( result ) );
        result = StringCchCatW( pixSearchPath, MAX_PATH, L"\\Microsoft PIX\\*" );
        EE_ASSERT( SUCCEEDED( result ) );

        WIN32_FIND_DATAW findData;
        bool             foundPixInstallation = false;
        wchar_t          newestVersionFound[MAX_PATH] = {};

        HANDLE hFind = FindFirstFileW( pixSearchPath, &findData );
        if ( hFind != INVALID_HANDLE_VALUE )
        {
            do
            {
                if ( ( ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY ) && ( findData.cFileName[0] != '.' ) )
                {
                    if ( !foundPixInstallation || wcscmp( newestVersionFound, findData.cFileName ) <= 0 )
                    {
                        foundPixInstallation = true;
                        result = StringCchCopyW( newestVersionFound, MAX_PATH, findData.cFileName );
                        EE_ASSERT( SUCCEEDED( result ) );
                    }
                }
            } while ( FindNextFileW( hFind, &findData ) != 0 );
        }

        FindClose( hFind );

        if ( !foundPixInstallation )
        {
            EE_LOG_ERROR( LogCategory::Render, "RHI/Pix", "Pix installation was not found!" );
            return;
        }

        wchar_t dllPath[MAX_PATH];
        result = StringCchCopyW( dllPath, MAX_PATH, programFilesPath );
        EE_ASSERT( SUCCEEDED( result ) );
        result = StringCchCatW( dllPath, MAX_PATH, L"\\Microsoft PIX\\" );
        EE_ASSERT( SUCCEEDED( result ) );
        result = StringCchCatW( dllPath, MAX_PATH, newestVersionFound );
        EE_ASSERT( SUCCEEDED( result ) );
        result = StringCchCatW( dllPath, MAX_PATH, L"\\WinPixGpuCapturer.dll" );
        EE_ASSERT( SUCCEEDED( result ) );

        if ( GetModuleHandleW( L"WinPixGpuCapturer.dll" ) == nullptr )
        {
            if ( LoadLibraryW( dllPath ) == nullptr )
            {
                wchar_t errorString[256];
                FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                errorString, _countof( errorString ), NULL );

                EE_LOG_ERROR( LogCategory::Render, "RHI/Pix", "Failed to load WinPixGpuCapturer.dll: %S", errorString );
            }
            else
            {
                EE_LOG_MESSAGE( LogCategory::Render, "RHI/Pix", "Loaded %S", dllPath );
            }
        }
    }
    #endif

    static D3D12_QUERY_TYPE D3D12QueryType( QueryType const queryType )
    {
        switch ( queryType )
        {
            case QueryType::Timestamp: return D3D12_QUERY_TYPE_TIMESTAMP;
            case QueryType::PipelineStatistics: return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
        }
        EE_ASSERT( false );
        return D3D12_QUERY_TYPE_TIMESTAMP;
    }

    static D3D12_QUERY_HEAP_TYPE D3D12QueryHeapType( QueryType const queryType )
    {
        switch ( queryType )
        {
            case QueryType::Timestamp: return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            case QueryType::PipelineStatistics: return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
        }
        EE_ASSERT( false );
        return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    }

    static D3D12_COMMAND_LIST_TYPE D3D12CommandBufferType( QueueType const queueType )
    {
        switch ( queueType )
        {
            case QueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case QueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
            case QueueType::Transfer: return D3D12_COMMAND_LIST_TYPE_COPY;
        }

        EE_ASSERT( false );
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }

    static D3D12_COMMAND_QUEUE_FLAGS D3D12QueueFlags( TBitFlags<QueueFlags> const& queueFlags )
    {
        D3D12_COMMAND_QUEUE_FLAGS result = D3D12_COMMAND_QUEUE_FLAG_NONE;
        if ( queueFlags.IsFlagSet( QueueFlags::DisableTimeout ) )
        {
            result |= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        }
        return result;
    }

    static D3D12_COMMAND_QUEUE_PRIORITY D3D12QueuePriority( QueuePriority const queuePriority )
    {
        switch ( queuePriority )
        {
            case QueuePriority::Normal: return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            case QueuePriority::High: return D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
            case QueuePriority::GlobalRealtime: return D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME;
        }

        EE_ASSERT( false );
        return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    }

    static DXGI_FORMAT DXGIFormat( DataFormat const format )
    {
        switch ( format )
        {
            case DataFormat::Undefined: return DXGI_FORMAT_UNKNOWN;

                // Uncompressed formats
            case DataFormat::R1_UNorm: return DXGI_FORMAT_R1_UNORM;
            case DataFormat::RGB565_UNorm: return DXGI_FORMAT_B5G6R5_UNORM;
            case DataFormat::BGR565_UNorm: return DXGI_FORMAT_B5G6R5_UNORM;
            case DataFormat::BGR555_A1_UNorm: return DXGI_FORMAT_B5G5R5A1_UNORM;
            case DataFormat::R8_UNorm: return DXGI_FORMAT_R8_UNORM;
            case DataFormat::R8_SNorm: return DXGI_FORMAT_R8_SNORM;
            case DataFormat::R8_UInt: return DXGI_FORMAT_R8_UINT;
            case DataFormat::R8_SInt: return DXGI_FORMAT_R8_SINT;
            case DataFormat::RG8_UNorm: return DXGI_FORMAT_R8G8_UNORM;
            case DataFormat::RG8_SNorm: return DXGI_FORMAT_R8G8_SNORM;
            case DataFormat::RG8_UInt: return DXGI_FORMAT_R8G8_UINT;
            case DataFormat::RG8_SInt: return DXGI_FORMAT_R8G8_SINT;
            case DataFormat::BGRA4_UNorm: return DXGI_FORMAT_B4G4R4A4_UNORM;
            case DataFormat::RGBA8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case DataFormat::RGBA8_SNorm: return DXGI_FORMAT_R8G8B8A8_SNORM;
            case DataFormat::RGBA8_UInt: return DXGI_FORMAT_R8G8B8A8_UINT;
            case DataFormat::RGBA8_SInt: return DXGI_FORMAT_R8G8B8A8_SINT;
            case DataFormat::RGBA8_sRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case DataFormat::BGRA8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case DataFormat::BGRA8_sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case DataFormat::RGB10_A2_UNorm: return DXGI_FORMAT_R10G10B10A2_UNORM;
            case DataFormat::RGB10_A2_UInt: return DXGI_FORMAT_R10G10B10A2_UINT;
            case DataFormat::R16_UNorm: return DXGI_FORMAT_R16_UNORM;
            case DataFormat::R16_SNorm: return DXGI_FORMAT_R16_SNORM;
            case DataFormat::R16_UInt: return DXGI_FORMAT_R16_UINT;
            case DataFormat::R16_SInt: return DXGI_FORMAT_R16_SINT;
            case DataFormat::R16_SFloat: return DXGI_FORMAT_R16_FLOAT;
            case DataFormat::RG16_UNorm: return DXGI_FORMAT_R16G16_UNORM;
            case DataFormat::RG16_SNorm: return DXGI_FORMAT_R16G16_SNORM;
            case DataFormat::RG16_UInt: return DXGI_FORMAT_R16G16_UINT;
            case DataFormat::RG16_SInt: return DXGI_FORMAT_R16G16_SINT;
            case DataFormat::RG16_SFloat: return DXGI_FORMAT_R16G16_FLOAT;
            case DataFormat::RGBA16_UNorm: return DXGI_FORMAT_R16G16B16A16_UNORM;
            case DataFormat::RGBA16_SNorm: return DXGI_FORMAT_R16G16B16A16_SNORM;
            case DataFormat::RGBA16_UInt: return DXGI_FORMAT_R16G16B16A16_UINT;
            case DataFormat::RGBA16_SInt: return DXGI_FORMAT_R16G16B16A16_SINT;
            case DataFormat::RGBA16_SFloat: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case DataFormat::R32_UInt: return DXGI_FORMAT_R32_UINT;
            case DataFormat::R32_SInt: return DXGI_FORMAT_R32_SINT;
            case DataFormat::R32_SFloat: return DXGI_FORMAT_R32_FLOAT;
            case DataFormat::RG32_UInt: return DXGI_FORMAT_R32G32_UINT;
            case DataFormat::RG32_SInt: return DXGI_FORMAT_R32G32_SINT;
            case DataFormat::RG32_SFloat: return DXGI_FORMAT_R32G32_FLOAT;
            case DataFormat::RGB32_UInt: return DXGI_FORMAT_R32G32B32_UINT;
            case DataFormat::RGB32_SInt: return DXGI_FORMAT_R32G32B32_SINT;
            case DataFormat::RGB32_SFloat: return DXGI_FORMAT_R32G32B32_FLOAT;
            case DataFormat::RGBA32_UInt: return DXGI_FORMAT_R32G32B32A32_UINT;
            case DataFormat::RGBA32_SInt: return DXGI_FORMAT_R32G32B32A32_SINT;
            case DataFormat::RGBA32_SFloat: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case DataFormat::RG11_B10_UFloat: return DXGI_FORMAT_R11G11B10_FLOAT;
            case DataFormat::RGB9_E5_UFloat: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
            case DataFormat::D32_SFloat: return DXGI_FORMAT_D32_FLOAT;
            case DataFormat::D32_SFloat_S8_UInt: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            case DataFormat::S8_Uint: return DXGI_FORMAT_D24_UNORM_S8_UINT;

                // Compressed DXBC formats
            case DataFormat::DXBC1_RGB_UNorm: return DXGI_FORMAT_BC1_UNORM;
            case DataFormat::DXBC1_RGB_sRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
            case DataFormat::DXBC1_RGBA_UNorm: return DXGI_FORMAT_BC1_UNORM;
            case DataFormat::DXBC1_RGBA_sRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
            case DataFormat::DXBC2_UNorm: return DXGI_FORMAT_BC2_UNORM;
            case DataFormat::DXBC2_sRGB: return DXGI_FORMAT_BC2_UNORM_SRGB;
            case DataFormat::DXBC3_UNorm: return DXGI_FORMAT_BC3_UNORM;
            case DataFormat::DXBC3_sRGB: return DXGI_FORMAT_BC3_UNORM_SRGB;
            case DataFormat::DXBC4_UNorm: return DXGI_FORMAT_BC4_UNORM;
            case DataFormat::DXBC4_SNorm: return DXGI_FORMAT_BC4_SNORM;
            case DataFormat::DXBC5_UNorm: return DXGI_FORMAT_BC5_UNORM;
            case DataFormat::DXBC5_SNorm: return DXGI_FORMAT_BC5_SNORM;
            case DataFormat::DXBC6H_UFloat: return DXGI_FORMAT_BC6H_UF16;
            case DataFormat::DXBC6H_SFloat: return DXGI_FORMAT_BC6H_SF16;
            case DataFormat::DXBC7_UNorm: return DXGI_FORMAT_BC7_UNORM;
            case DataFormat::DXBC7_sRGB: return DXGI_FORMAT_BC7_UNORM_SRGB;

                // Compressed ASTC formats, not valid on D3D so return DXGI_FORMAT_UNKNOWN but don't assert
            case DataFormat::ASTC_4x4_UNorm:
            case DataFormat::ASTC_4x4_sRGB:
            case DataFormat::ASTC_5x4_UNorm:
            case DataFormat::ASTC_5x4_sRGB:
            case DataFormat::ASTC_5x5_UNorm:
            case DataFormat::ASTC_5x5_sRGB:
            case DataFormat::ASTC_6x5_UNorm:
            case DataFormat::ASTC_6x5_sRGB:
            case DataFormat::ASTC_6x6_UNorm:
            case DataFormat::ASTC_6x6_sRGB:
            case DataFormat::ASTC_8x5_UNorm:
            case DataFormat::ASTC_8x5_sRGB:
            case DataFormat::ASTC_8x6_UNorm:
            case DataFormat::ASTC_8x6_sRGB:
            case DataFormat::ASTC_8x8_UNorm:
            case DataFormat::ASTC_8x8_sRGB:
            case DataFormat::ASTC_10x5_UNorm:
            case DataFormat::ASTC_10x5_sRGB:
            case DataFormat::ASTC_10x6_UNorm:
            case DataFormat::ASTC_10x6_sRGB:
            case DataFormat::ASTC_10x8_UNorm:
            case DataFormat::ASTC_10x8_sRGB:
            case DataFormat::ASTC_10x10_UNorm:
            case DataFormat::ASTC_10x10_sRGB:
            case DataFormat::ASTC_12x10_UNorm:
            case DataFormat::ASTC_12x10_sRGB:
            case DataFormat::ASTC_12x12_UNorm:
            case DataFormat::ASTC_12x12_sRGB:
            {
                return DXGI_FORMAT_UNKNOWN;
            }

            // Special case - completely invalid format on all platforms
            default:
            {
                EE_ASSERT( false );
                return DXGI_FORMAT_UNKNOWN;
            }
        };
    }

    static D3D12_SRV_DIMENSION D3D12ShaderResourceViewDimension( ViewDimension const viewDimension )
    {
        switch ( viewDimension )
        {
            case ViewDimension::Undefined: return D3D12_SRV_DIMENSION_UNKNOWN;
            case ViewDimension::Buffer: return D3D12_SRV_DIMENSION_BUFFER;
            case ViewDimension::Texture1D: return D3D12_SRV_DIMENSION_TEXTURE1D;
            case ViewDimension::Texture1DArray: return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            case ViewDimension::Texture2D: return D3D12_SRV_DIMENSION_TEXTURE2D;
            case ViewDimension::Texture2DArray: return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            case ViewDimension::Texture2DMultisample: return D3D12_SRV_DIMENSION_TEXTURE2DMS;
            case ViewDimension::Texture2DMultisampleArray: return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            case ViewDimension::Texture3D: return D3D12_SRV_DIMENSION_TEXTURE3D;
            case ViewDimension::TextureCube: return D3D12_SRV_DIMENSION_TEXTURECUBE;
            case ViewDimension::TextureCubeArray: return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            case ViewDimension::AccelerationStructure: return D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;

                // Special case - invalid format
            default:
            {
                EE_ASSERT( false );
                return D3D12_SRV_DIMENSION_UNKNOWN;
            }
        }
    }

    static D3D12_DSV_DIMENSION D3D12DepthStencilViewDimension( ViewDimension const viewDimension )
    {
        switch ( viewDimension )
        {
            case ViewDimension::Undefined: return D3D12_DSV_DIMENSION_UNKNOWN;
            case ViewDimension::Texture1D: return D3D12_DSV_DIMENSION_TEXTURE1D;
            case ViewDimension::Texture1DArray: return D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            case ViewDimension::Texture2D: return D3D12_DSV_DIMENSION_TEXTURE2D;
            case ViewDimension::Texture2DArray: return D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            case ViewDimension::Texture2DMultisample: return D3D12_DSV_DIMENSION_TEXTURE2DMS;
            case ViewDimension::Texture2DMultisampleArray: return D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;

                // Special case - invalid format
            default:
            {
                EE_ASSERT( false );
                return D3D12_DSV_DIMENSION_UNKNOWN;
            }
        }
    }

    static D3D12_RTV_DIMENSION D3D12RenderTargetViewDimension( ViewDimension const viewDimension )
    {
        switch ( viewDimension )
        {
            case ViewDimension::Undefined: return D3D12_RTV_DIMENSION_UNKNOWN;
            case ViewDimension::Buffer: return D3D12_RTV_DIMENSION_BUFFER;
            case ViewDimension::Texture1D: return D3D12_RTV_DIMENSION_TEXTURE1D;
            case ViewDimension::Texture1DArray: return D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            case ViewDimension::Texture2D: return D3D12_RTV_DIMENSION_TEXTURE2D;
            case ViewDimension::Texture2DArray: return D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            case ViewDimension::Texture2DMultisample: return D3D12_RTV_DIMENSION_TEXTURE2DMS;
            case ViewDimension::Texture2DMultisampleArray: return D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            case ViewDimension::Texture3D: return D3D12_RTV_DIMENSION_TEXTURE3D;
            case ViewDimension::TextureCube: return D3D12_RTV_DIMENSION_TEXTURE2DARRAY;

                // Special case - invalid format
            default:
            {
                EE_ASSERT( false );
                return D3D12_RTV_DIMENSION_UNKNOWN;
            }
        }
    }

    static D3D12_UAV_DIMENSION D3D12UnorderedAccessViewDimension( ViewDimension const viewDimension )
    {
        switch ( viewDimension )
        {
            case ViewDimension::Undefined: return D3D12_UAV_DIMENSION_UNKNOWN;
            case ViewDimension::Buffer: return D3D12_UAV_DIMENSION_BUFFER;
            case ViewDimension::Texture1D: return D3D12_UAV_DIMENSION_TEXTURE1D;
            case ViewDimension::Texture1DArray: return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            case ViewDimension::Texture2D: return D3D12_UAV_DIMENSION_TEXTURE2D;
            case ViewDimension::Texture2DArray: return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            case ViewDimension::Texture3D: return D3D12_UAV_DIMENSION_TEXTURE3D;

                // Special case - invalid format
            case ViewDimension::Texture2DMultisample:
            case ViewDimension::Texture2DMultisampleArray:
            case ViewDimension::TextureCube:
            case ViewDimension::TextureCubeArray:
            case ViewDimension::AccelerationStructure:
            default:
            {
                EE_ASSERT( false );
                return D3D12_UAV_DIMENSION_UNKNOWN;
            }
        }
    }

    static D3D12_FILTER D3D12Filter( FilterType const minFilter, FilterType const magFilter, MipMapMode const mipmapMode, CompareMode const compareMode, FilterMode filterMode, uint32_t const maxAnisotropy )
    {
        if ( compareMode != CompareMode::Never )
        {
            EE_ASSERT( filterMode == FilterMode::Compare );
        }

        if ( maxAnisotropy > 1 )
        {
            if ( compareMode != CompareMode::Never )
            {
                return D3D12_FILTER_COMPARISON_ANISOTROPIC;
            }
            return D3D12_FILTER_ANISOTROPIC;
        }

        int const filterType = ( int( minFilter ) << 4 ) | ( int( magFilter ) << 2 ) | int( mipmapMode );

        switch ( filterMode )
        {
            case FilterMode::Default: return D3D12_FILTER( D3D12_FILTER_MIN_MAG_MIP_POINT + filterType );
            case FilterMode::Compare: return D3D12_FILTER( D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT + filterType );
            case FilterMode::Min: return D3D12_FILTER( D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT + filterType );
            case FilterMode::Max: return D3D12_FILTER( D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT + filterType );
        };

        return D3D12_FILTER_MIN_MAG_MIP_POINT;
    }

    static D3D12_TEXTURE_ADDRESS_MODE D3D12TextureAddressMode( AddressMode const addressMode )
    {
        switch ( addressMode )
        {
            case AddressMode::Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case AddressMode::ClampToEdge: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case AddressMode::ClampToBorder: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            case AddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

            default:
            {
                EE_ASSERT( false );
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            };
        }
    }

    static D3D12_SHADING_RATE_COMBINER D3D12ShadingRateCombiner( ShadingRateCombiner const combiner )
    {
        switch ( combiner )
        {
            case ShadingRateCombiner::Passthrough: return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
            case ShadingRateCombiner::Override: return D3D12_SHADING_RATE_COMBINER_OVERRIDE;
            case ShadingRateCombiner::Min: return D3D12_SHADING_RATE_COMBINER_MIN;
            case ShadingRateCombiner::Max: return D3D12_SHADING_RATE_COMBINER_MAX;
            case ShadingRateCombiner::Sum: return D3D12_SHADING_RATE_COMBINER_SUM;
            default:
            {
                EE_ASSERT( false );
                return D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
            }
        }
    }

    static D3D12_COMPARISON_FUNC D3D12ComparisonFunc( CompareMode const compareMode )
    {
        switch ( compareMode )
        {
            case CompareMode::Never: return D3D12_COMPARISON_FUNC_NEVER;
            case CompareMode::Less: return D3D12_COMPARISON_FUNC_LESS;
            case CompareMode::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
            case CompareMode::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case CompareMode::Greater: return D3D12_COMPARISON_FUNC_GREATER;
            case CompareMode::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case CompareMode::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case CompareMode::Always: return D3D12_COMPARISON_FUNC_ALWAYS;

            default:
            {
                EE_ASSERT( false );
                return D3D12_COMPARISON_FUNC_NEVER;
            };
        }
    }

    static D3D12_SHADING_RATE D3D12ShadingRate( ShadingRate const shadingRate )
    {
        switch ( shadingRate )
        {
            case ShadingRate::Full: return D3D12_SHADING_RATE_1X1;
            case ShadingRate::Rate1x2: return D3D12_SHADING_RATE_1X2;
            case ShadingRate::Rate2x1: return D3D12_SHADING_RATE_2X1;
            case ShadingRate::Half: return D3D12_SHADING_RATE_2X2;
            case ShadingRate::Rate2x4: return D3D12_SHADING_RATE_2X4;
            case ShadingRate::Rate4x2: return D3D12_SHADING_RATE_4X2;
            case ShadingRate::Quarter: return D3D12_SHADING_RATE_4X4;
            default:
            {
                EE_ASSERT( false );
                return D3D12_SHADING_RATE_1X1;
            }
        }
    }

    static DXGI_FORMAT D3D12IndexType( IndexType indexType )
    {
        switch ( indexType )
        {
            case IndexType::Uint16: return DXGI_FORMAT_R16_UINT;
            case IndexType::Uint32: return DXGI_FORMAT_R32_UINT;

            default:
            {
                EE_ASSERT( false );
                return DXGI_FORMAT_R16_UINT;
            }
        }
    }

    static D3D12_BARRIER_SYNC D3D12BarrierSync( TBitFlags<PipelineStage> pipelineStages )
    {
        D3D12_BARRIER_SYNC d3d12BarrierSync = {};

        //if ( pipelineStages.IsFlagSet( PipelineStage::None ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_NONE; }
        if ( pipelineStages.IsFlagSet( PipelineStage::All ) ) { return D3D12_BARRIER_SYNC_ALL; }
        if ( pipelineStages.IsFlagSet( PipelineStage::Draw ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_DRAW; }
        if ( pipelineStages.IsFlagSet( PipelineStage::PixelShader ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_PIXEL_SHADING; }
        if ( pipelineStages.IsFlagSet( PipelineStage::NonPixelShader ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_NON_PIXEL_SHADING; }
        if ( pipelineStages.IsFlagSet( PipelineStage::ComputeShader ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_COMPUTE_SHADING; }
        if ( pipelineStages.IsFlagSet( PipelineStage::Raytracing ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_RAYTRACING; }
        if ( pipelineStages.IsFlagSet( PipelineStage::AllShader ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_ALL_SHADING; }
        if ( pipelineStages.IsFlagSet( PipelineStage::Copy ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_COPY; }
        if ( pipelineStages.IsFlagSet( PipelineStage::ExecuteIndirect ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT; }
        if ( pipelineStages.IsFlagSet( PipelineStage::VideoDecode ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_VIDEO_DECODE; }
        if ( pipelineStages.IsFlagSet( PipelineStage::VideoProcess ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_VIDEO_PROCESS; }
        if ( pipelineStages.IsFlagSet( PipelineStage::VideoEncode ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_VIDEO_ENCODE; }
        if ( pipelineStages.IsFlagSet( PipelineStage::BuildAccelerationStructure ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE; }
        if ( pipelineStages.IsFlagSet( PipelineStage::CopyAccelerationStructure ) ) { d3d12BarrierSync |= D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE; }

        return d3d12BarrierSync;
    }

    static D3D12_BARRIER_ACCESS D3D12BarrierAccess( TBitFlags<ResourceAccess> barrierAccess )
    {
        D3D12_BARRIER_ACCESS d3d12BarrierAccess = {};

        if ( barrierAccess.IsFlagSet( ResourceAccess::NoAccess ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_NO_ACCESS; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::Common ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_COMMON; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::Present ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_COMMON; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::ConstantBuffer ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::IndexBuffer ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_INDEX_BUFFER; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::RenderTarget ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_RENDER_TARGET; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::UnorderedAccess ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::DepthWrite ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::DepthRead ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::ShaderResource ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::IndirectArgument ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::CopyDestination ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_COPY_DEST; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::CopySource ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_COPY_SOURCE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::AccelerationStructureRead ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::AccelerationStructureWrite ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::ShadingRateSource ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::VideoDecodeRead ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::VideoDecodeWrite ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::VideoProcessRead ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::VideoProcessWrite ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::VideoEncodeRead ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ; }
        if ( barrierAccess.IsFlagSet( ResourceAccess::VideoEncodeWrite ) ) { d3d12BarrierAccess |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE; }

        return d3d12BarrierAccess;
    }

    static D3D12_BARRIER_LAYOUT D3D12BarrierLayout( TextureState textureState )
    {
        switch ( textureState )
        {
            case TextureState::Undefined: return D3D12_BARRIER_LAYOUT_UNDEFINED; break;
            case TextureState::Common: return D3D12_BARRIER_LAYOUT_COMMON; break;
            case TextureState::ShaderResource: return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE; break;
            case TextureState::UnorderedAccess: return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS; break;
            case TextureState::Present: return D3D12_BARRIER_LAYOUT_PRESENT; break;
            case TextureState::RenderTarget: return D3D12_BARRIER_LAYOUT_RENDER_TARGET; break;
            case TextureState::DepthWrite: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE; break;
            case TextureState::DepthRead: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ; break;
            case TextureState::ShadingRateSource: return D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE; break;
            case TextureState::VideoDecodeRead: return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ; break;
            case TextureState::VideoDecodeWrite: return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE; break;
            case TextureState::VideoProcessRead: return D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ; break;
            case TextureState::VideoProcessWrite: return D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE; break;
            case TextureState::VideoEncodeRead: return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ; break;
            case TextureState::VideoEncodeWrite: return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE; break;
        };
        return {};
    }

    static D3D12_TEXTURE_BARRIER_FLAGS D3D12TextureBarrierFlags( TBitFlags<TextureBarrierFlags> flags )
    {
        D3D12_TEXTURE_BARRIER_FLAGS d3d12TextureBarrierFlags = {};

        if ( flags.IsFlagSet( TextureBarrierFlags::Discard ) ) { d3d12TextureBarrierFlags |= D3D12_TEXTURE_BARRIER_FLAG_DISCARD; }

        return d3d12TextureBarrierFlags;
    }

    static D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS D3D12AccelerationStructureBuildFlags( TBitFlags<AccelerationStructureBuildFlags> const& buildFlags )
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3d12BuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

        if ( buildFlags.IsFlagSet( AccelerationStructureBuildFlags::AllowUpdate ) ) { d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE; }
        if ( buildFlags.IsFlagSet( AccelerationStructureBuildFlags::AllowCompaction ) ) { d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION; }
        if ( buildFlags.IsFlagSet( AccelerationStructureBuildFlags::PreferFastTrace ) ) { d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; }
        if ( buildFlags.IsFlagSet( AccelerationStructureBuildFlags::PreferFastBuild ) ) { d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD; }
        if ( buildFlags.IsFlagSet( AccelerationStructureBuildFlags::MinimizeMemory ) ) { d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY; }
        if ( buildFlags.IsFlagSet( AccelerationStructureBuildFlags::PerformUpdate ) ) { d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE; }

        return d3d12BuildFlags;
    }

    static D3D12_RAYTRACING_GEOMETRY_FLAGS D3D12GeometryFlags( TBitFlags<AccelerationStructureGeometryFlags> const& geometryFlags )
    {
        D3D12_RAYTRACING_GEOMETRY_FLAGS d3d12GeometryFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

        if ( geometryFlags.IsFlagSet( AccelerationStructureGeometryFlags::Opaque ) ) { d3d12GeometryFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; }
        if ( geometryFlags.IsFlagSet( AccelerationStructureGeometryFlags::NoDuplicateAnyhitInvocation ) ) { d3d12GeometryFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION; }

        return d3d12GeometryFlags;
    }

    static D3D12_SHADER_VISIBILITY D3D12ShaderVisibility( TBitFlags<ShaderStage> const& shaderStages )
    {
        D3D12_SHADER_VISIBILITY result = D3D12_SHADER_VISIBILITY_ALL;
        uint32_t                numStages = 0;

        if ( shaderStages.IsFlagSet( ShaderStage::Vertex ) )
        {
            result = D3D12_SHADER_VISIBILITY_VERTEX;
            numStages++;
        }

        if ( shaderStages.IsFlagSet( ShaderStage::Pixel ) )
        {
            result = D3D12_SHADER_VISIBILITY_PIXEL;
            numStages++;
        }

        if ( shaderStages.IsFlagSet( ShaderStage::Task ) )
        {
            result = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
            numStages++;
        }

        if ( shaderStages.IsFlagSet( ShaderStage::Mesh ) )
        {
            result = D3D12_SHADER_VISIBILITY_MESH;
            numStages++;
        }

        if ( shaderStages.AreAnyFlagsSet( ShaderStage::RayTracing, ShaderStage::Compute ) )
        {
            result = D3D12_SHADER_VISIBILITY_ALL;
            numStages++;
        }

        if ( numStages > 1 )
        {
            return D3D12_SHADER_VISIBILITY_ALL;
        }
        else
        {
            return result;
        }
    }

    static D3D12_BLEND D3D12Blend( BlendConstant const constant )
    {
        switch ( constant )
        {
            case BlendConstant::Zero: return D3D12_BLEND_ZERO;
            case BlendConstant::One: return D3D12_BLEND_ONE;
            case BlendConstant::SrcColor: return D3D12_BLEND_SRC_COLOR;
            case BlendConstant::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
            case BlendConstant::DstColor: return D3D12_BLEND_DEST_COLOR;
            case BlendConstant::OneMinusDstColor: return D3D12_BLEND_INV_DEST_COLOR;
            case BlendConstant::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
            case BlendConstant::SrcAlphaSaturate: return D3D12_BLEND_SRC_ALPHA_SAT;
            case BlendConstant::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
            case BlendConstant::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
            case BlendConstant::OneMinusDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
            case BlendConstant::BlendFactor: return D3D12_BLEND_BLEND_FACTOR;
            case BlendConstant::OneMinusBlendFactor: return D3D12_BLEND_INV_BLEND_FACTOR;
        }
        EE_ASSERT( false );
        return D3D12_BLEND_ZERO;
    }

    static D3D12_BLEND_OP D3D12BlendOp( BlendMode const mode )
    {
        switch ( mode )
        {
            case BlendMode::Add: return D3D12_BLEND_OP_ADD;
            case BlendMode::Subtract: return D3D12_BLEND_OP_SUBTRACT;
            case BlendMode::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
            case BlendMode::Min: return D3D12_BLEND_OP_MIN;
            case BlendMode::Max: return D3D12_BLEND_OP_MAX;
        }
        EE_ASSERT( false );
        return D3D12_BLEND_OP_ADD;
    }

    static D3D12_STENCIL_OP D3D12StencilOp( StencilOp op )
    {
        switch ( op )
        {
            case StencilOp::Keep: return D3D12_STENCIL_OP_KEEP;
            case StencilOp::SetZero: return D3D12_STENCIL_OP_ZERO;
            case StencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
            case StencilOp::Invert: return D3D12_STENCIL_OP_INVERT;
            case StencilOp::Increment: return D3D12_STENCIL_OP_INCR;
            case StencilOp::Decrement: return D3D12_STENCIL_OP_DECR;
            case StencilOp::IncrementSaturate: return D3D12_STENCIL_OP_INCR_SAT;
            case StencilOp::DecrementSaturate: return D3D12_STENCIL_OP_DECR_SAT;
        }
        EE_ASSERT( false );
        return D3D12_STENCIL_OP_KEEP;
    }

    static D3D12_CULL_MODE D3D12CullMode( CullMode mode )
    {
        switch ( mode )
        {
            case CullMode::None: return D3D12_CULL_MODE_NONE;
            case CullMode::Back: return D3D12_CULL_MODE_BACK;
            case CullMode::Front: return D3D12_CULL_MODE_FRONT;
        }
        EE_ASSERT( false );
        return D3D12_CULL_MODE_NONE;
    }

    static D3D12_FILL_MODE D3D12FillMode( FillMode mode )
    {
        switch ( mode )
        {
            case FillMode::Solid: return D3D12_FILL_MODE_SOLID;
            case FillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
        }
        EE_ASSERT( false );
        return D3D12_FILL_MODE_SOLID;
    }

    static D3D12_PRIMITIVE_TOPOLOGY D3D12PrimitiveTopology( PrimitiveTopology topology )
    {
        switch ( topology )
        {
            case PrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        }
        EE_ASSERT( false );
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }

    static ViewDimension TextureViewDimension( uint32_t const width, uint32_t const height, uint32_t const depth, uint32_t const arrayLayers, uint32_t const numSamples, TBitFlags<DescriptorTypeFlags> const& descriptorType )
    {
        bool const isCubemap = descriptorType.AreAnyFlagsSet( DescriptorTypeFlags::TextureCube );
        if ( numSamples > 1 )
        {
            EE_ASSERT( height > 1 && depth == 1 && !isCubemap );
            if ( arrayLayers > 1 )
            {
                return ViewDimension::Texture2DMultisampleArray;
            }
            return ViewDimension::Texture2DMultisample;
        }

        if ( isCubemap )
        {
            EE_ASSERT( ( arrayLayers % 6 ) == 0 );
            if ( arrayLayers > 6 )
            {
                return ViewDimension::TextureCubeArray;
            }
            else
            {
                return ViewDimension::TextureCube;
            }
        }

        if ( arrayLayers > 1 )
        {
            EE_ASSERT( depth == 1 ); // No 3D texture array in D3D12

            if ( height > 1 )
            {
                return ViewDimension::Texture2DArray;
            }
            return ViewDimension::Texture1DArray;
        }

        if ( depth > 1 )
        {
            return ViewDimension::Texture3D;
        }

        if ( height > 1 )
        {
            return ViewDimension::Texture2D;
        }

        return ViewDimension::Texture1D;
    }

    static ViewDimension TextureViewDimension( Texture const* pTexture )
    {
        return TextureViewDimension( pTexture->m_width, pTexture->m_height, pTexture->m_depth, pTexture->m_arrayLayers, pTexture->m_numSamples, pTexture->m_descriptorTypes );
    }

    static bool IsShaderResourceViewTexture( D3D_SRV_DIMENSION dimension )
    {
        return dimension == D3D_SRV_DIMENSION_TEXTURE1D ||
            dimension == D3D_SRV_DIMENSION_TEXTURE1DARRAY ||
            dimension == D3D_SRV_DIMENSION_TEXTURE2D ||
            dimension == D3D_SRV_DIMENSION_TEXTURE2DARRAY ||
            dimension == D3D_SRV_DIMENSION_TEXTURE2DMS ||
            dimension == D3D_SRV_DIMENSION_TEXTURE3D ||
            dimension == D3D_SRV_DIMENSION_TEXTURECUBE ||
            dimension == D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
    }

    static DescriptorTypeFlags DescriptorTypeFromShaderInputType( D3D_SHADER_INPUT_TYPE shaderInputType, D3D_SRV_DIMENSION shaderViewDimension )
    {
        switch ( shaderInputType )
        {
            case D3D_SIT_CBUFFER: return DescriptorTypeFlags::ConstantBuffer;
            case D3D_SIT_TBUFFER: return DescriptorTypeFlags::Buffer;
            case D3D_SIT_TEXTURE:
            {
                if ( shaderViewDimension == D3D_SRV_DIMENSION_BUFFER )
                {
                    return DescriptorTypeFlags::Buffer;
                }

                EE_ASSERT( IsShaderResourceViewTexture( shaderViewDimension ) );
                return DescriptorTypeFlags::Texture;
            }
            case D3D_SIT_SAMPLER: return DescriptorTypeFlags::Sampler;
            case D3D_SIT_UAV_RWTYPED:
            {
                if ( shaderViewDimension == D3D_SRV_DIMENSION_BUFFER )
                {
                    return DescriptorTypeFlags::RWBuffer;
                }

                EE_ASSERT( IsShaderResourceViewTexture( shaderViewDimension ) );
                return DescriptorTypeFlags::RWTexture;
            }
            case D3D_SIT_STRUCTURED: return DescriptorTypeFlags::Buffer;
            case D3D_SIT_UAV_RWSTRUCTURED: return DescriptorTypeFlags::RWBuffer;
            case D3D_SIT_BYTEADDRESS: return DescriptorTypeFlags::Buffer;
            case D3D_SIT_UAV_RWBYTEADDRESS: return DescriptorTypeFlags::RWBuffer;
            case D3D_SIT_UAV_APPEND_STRUCTURED: return DescriptorTypeFlags::RWBuffer;
            case D3D_SIT_UAV_CONSUME_STRUCTURED: return DescriptorTypeFlags::RWBuffer;
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: return DescriptorTypeFlags::RWBuffer;
            case D3D_SIT_RTACCELERATIONSTRUCTURE: return DescriptorTypeFlags::AccelerationStructure;

            default:
            {
                EE_ASSERT( false );
                return {};
            }
        }
    }

    static ViewDimension ViewDimensionFromShaderResourceViewDimension( D3D_SRV_DIMENSION shaderResourceViewDimension )
    {
        switch ( shaderResourceViewDimension )
        {
            case D3D_SRV_DIMENSION_UNKNOWN: return ViewDimension::Undefined;
            case D3D_SRV_DIMENSION_BUFFER: return ViewDimension::Buffer;
            case D3D_SRV_DIMENSION_TEXTURE1D: return ViewDimension::Texture1D;
            case D3D_SRV_DIMENSION_TEXTURE1DARRAY: return ViewDimension::Texture1DArray;
            case D3D_SRV_DIMENSION_TEXTURE2D: return ViewDimension::Texture2D;
            case D3D_SRV_DIMENSION_TEXTURE2DARRAY: return ViewDimension::Texture2DArray;
            case D3D_SRV_DIMENSION_TEXTURE2DMS: return ViewDimension::Texture2DMultisample;
            case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY: return ViewDimension::Texture2DMultisampleArray;
            case D3D_SRV_DIMENSION_TEXTURE3D: return ViewDimension::Texture3D;
            case D3D_SRV_DIMENSION_TEXTURECUBE: return ViewDimension::TextureCube;
            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY: return ViewDimension::TextureCubeArray;
            case D3D_SRV_DIMENSION_BUFFEREX: return ViewDimension::Undefined;

            default:
            {
                EE_ASSERT( false );
                return ViewDimension::Undefined;
            }
        }
    }

    static bool IsRootConstant( StringView name )
    {
        StringView   rootConstantName = "rootconstants";
        InlineString lowercaseName( rootConstantName.size(), '\0' );

        for ( size_t charIndex = 0; charIndex < rootConstantName.size(); ++charIndex )
        {
            lowercaseName[charIndex] = char( tolower( name[charIndex] ) );
        }

        return lowercaseName == rootConstantName;
    }

    static ShaderReflection ExtractReflection( IDxcBlobEncoding* pD3D12BlobEncoding, ShaderStage shaderType )
    {
        HRESULT result;

        ComPtr<IDxcContainerReflection> pD3D12ContainerReflection = {};
        result = DxcCreateInstance( CLSID_DxcContainerReflection, IID_PPV_ARGS( pD3D12ContainerReflection.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );
        result = pD3D12ContainerReflection->Load( pD3D12BlobEncoding );
        EE_ASSERT( SUCCEEDED( result ) );

        UINT32 shaderPart = 0;
        result = pD3D12ContainerReflection->FindFirstPartKind( DXC_FOURCC( 'D', 'X', 'I', 'L' ), &shaderPart );
        EE_ASSERT( SUCCEEDED( result ) );

        ComPtr<ID3D12ShaderReflection> pD3D12ShaderReflection = {};
        result = pD3D12ContainerReflection->GetPartReflection( shaderPart, IID_PPV_ARGS( pD3D12ShaderReflection.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        D3D12_SHADER_DESC d3d12ShaderDesc = {};
        result = pD3D12ShaderReflection->GetDesc( &d3d12ShaderDesc );
        EE_ASSERT( SUCCEEDED( result ) );

        ShaderReflection reflection = {};
        reflection.m_shaderStages.AppendFlags( shaderType );

        if ( shaderType == ShaderStage::Compute || shaderType == ShaderStage::Task || shaderType == ShaderStage::Mesh )
        {
            result = pD3D12ShaderReflection->GetThreadGroupSize( &reflection.m_threadsPerGroup[0], &reflection.m_threadsPerGroup[1], &reflection.m_threadsPerGroup[2] );
            EE_ASSERT( SUCCEEDED( result ) );
        }

        reflection.m_shaderResources.reserve( d3d12ShaderDesc.BoundResources );
        reflection.m_shaderVariables.reserve( d3d12ShaderDesc.ConstantBuffers );

        for ( UINT shaderResourceIndex = 0; shaderResourceIndex < d3d12ShaderDesc.BoundResources; ++shaderResourceIndex )
        {
            D3D12_SHADER_INPUT_BIND_DESC d3d12InputBindDesc = {};
            result = pD3D12ShaderReflection->GetResourceBindingDesc( shaderResourceIndex, &d3d12InputBindDesc );
            EE_ASSERT( SUCCEEDED( result ) );

            ShaderResource shaderResource = {};
            shaderResource.m_setIndex = d3d12InputBindDesc.Space;
            shaderResource.m_registerIndex = d3d12InputBindDesc.BindPoint;
            shaderResource.m_numConstants = d3d12InputBindDesc.BindCount;
            shaderResource.m_usedStages = shaderType;
            shaderResource.m_name = d3d12InputBindDesc.Name;
            shaderResource.m_viewDimension = ViewDimensionFromShaderResourceViewDimension( d3d12InputBindDesc.Dimension );

            if ( IsRootConstant( d3d12InputBindDesc.Name ) )
            {
                shaderResource.m_descriptorTypeFlags = DescriptorTypeFlags::RootConstant;
            }
            else
            {
                shaderResource.m_descriptorTypeFlags = DescriptorTypeFromShaderInputType( d3d12InputBindDesc.Type, d3d12InputBindDesc.Dimension );
            }

            reflection.m_shaderResources.push_back( shaderResource );
        }

        for ( UINT constantBufferIndex = 0; constantBufferIndex < d3d12ShaderDesc.ConstantBuffers; ++constantBufferIndex )
        {
            ID3D12ShaderReflectionConstantBuffer* pD3D12ConstantBuffer = pD3D12ShaderReflection->GetConstantBufferByIndex( constantBufferIndex );

            D3D12_SHADER_BUFFER_DESC d3d12ConstantBufferDesc = {};
            result = pD3D12ConstantBuffer->GetDesc( &d3d12ConstantBufferDesc );
            EE_ASSERT( SUCCEEDED( result ) );

            if ( d3d12ConstantBufferDesc.Type != D3D_CT_CBUFFER )
            {
                continue;
            }

            uint32_t foundResourceIndex = ~0U;
            for ( uint32_t resourceIndex = 0; resourceIndex < d3d12ShaderDesc.BoundResources; ++resourceIndex )
            {
                D3D12_SHADER_INPUT_BIND_DESC d3d12InputBindDesc = {};
                result = pD3D12ShaderReflection->GetResourceBindingDesc( resourceIndex, &d3d12InputBindDesc );
                EE_ASSERT( SUCCEEDED( result ) );

                if ( d3d12InputBindDesc.Type == D3D_SIT_CBUFFER && ( strcmp( d3d12InputBindDesc.Name, d3d12ConstantBufferDesc.Name ) == 0 ) )
                {
                    foundResourceIndex = resourceIndex;
                    break;
                }
            }

            EE_ASSERT( foundResourceIndex != ~0U );

            for ( uint32_t variableIndex = 0; variableIndex < d3d12ConstantBufferDesc.Variables; ++variableIndex )
            {
                ID3D12ShaderReflectionVariable* pD3D12Variable = pD3D12ConstantBuffer->GetVariableByIndex( variableIndex );

                D3D12_SHADER_VARIABLE_DESC d3d12VariableDesc = {};
                result = pD3D12Variable->GetDesc( &d3d12VariableDesc );
                EE_ASSERT( SUCCEEDED( result ) );

                if ( ( d3d12VariableDesc.uFlags & D3D_SVF_USED ) != 0 )
                {
                    ShaderVariable variable = {};
                    variable.m_name = d3d12VariableDesc.Name;
                    variable.m_parentResourceIndex = foundResourceIndex;
                    variable.m_offset = d3d12VariableDesc.StartOffset;
                    variable.m_size = d3d12VariableDesc.Size;

                    reflection.m_shaderVariables.push_back( variable );
                }
            }
        }

        return reflection;
    }

    template<size_t N>
    static TInlineString<N> FormatVendorID( UINT id )
    {
        TInlineString<N> buffer;
        buffer.sprintf( "%#x", id );
        return buffer;
    }

    template<size_t N>
    static TInlineString<N> FormatVendorName( WCHAR const* pVendorName )
    {
        char          buffer[N] = {};
        size_t        numConverted = 0;
        errno_t const error = wcstombs_s( &numConverted, buffer, pVendorName, N );

        EE_ASSERT( error == 0 );

        return TInlineString<N>( buffer );
    }

    static void FormatEntryPointName( StringView name, TArrayView<wchar_t> dest )
    {
        size_t        numConverted = 0;
        errno_t const error = mbstowcs_s( &numConverted, dest.data(), dest.size(), name.data(), name.size() );

        EE_ASSERT( error == 0 );
    }

    template<typename T>
    static void InitializeFixedCapacityVector( TUniquePtr<T[]>& memory, eastl::fixed_allocator& allocator, eastl::vector<T, eastl::fixed_allocator>& vector, size_t capacity )
    {
        memory.reset( EE::NewArray<T>( capacity ) );
        allocator.init( memory.get(), sizeof( T ) * capacity, sizeof( T ), alignof( T ) );
        vector.set_allocator( allocator );
    }

    static uint32_t SubresourceIndex( uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arrayLayers )
    {
        return mipSlice + arraySlice * mipLevels + planeSlice * mipLevels * arrayLayers;
    }

    template<typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
    struct alignas( void* ) D3D12PipelineStreamSubobject
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_subobjectType = Type;
        T                                   m_subobject = {};

        EE_FORCE_INLINE D3D12PipelineStreamSubobject& operator=( T const& i ) noexcept
        {
            m_subobject = i;
            return *this;
        }

        EE_FORCE_INLINE          operator T const&( ) const noexcept { return m_subobject; }
        EE_FORCE_INLINE          operator T&( ) noexcept { return m_subobject; }
        EE_FORCE_INLINE T*       operator&() noexcept { return &m_subobject; }
        EE_FORCE_INLINE T const* operator&() const noexcept { return &m_subobject; }
    };

    #pragma warning( push )
    #pragma warning( disable : 4324 ) // "C4324: Structure was padded due to alignment specifier", padding is expected by D3D

    struct D3D12MeshShaderPipelineStream
    {
        D3D12PipelineStreamSubobject<D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS>                 m_Flags;
        D3D12PipelineStreamSubobject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK>                                   m_NodeMask;
        D3D12PipelineStreamSubobject<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>              m_pRootSignature;
        D3D12PipelineStreamSubobject<D3D12_PRIMITIVE_TOPOLOGY_TYPE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY> m_PrimitiveTopologyType;
        D3D12PipelineStreamSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>                         m_PS;
        D3D12PipelineStreamSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>                         m_AS;
        D3D12PipelineStreamSubobject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>                         m_MS;
        D3D12PipelineStreamSubobject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>                           m_BlendState;
        D3D12PipelineStreamSubobject<D3D12_DEPTH_STENCIL_DESC1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1>         m_DepthStencilState;
        D3D12PipelineStreamSubobject<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>                 m_DSVFormat;
        D3D12PipelineStreamSubobject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>                 m_RasterizerState;
        D3D12PipelineStreamSubobject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS>      m_RTVFormats;
        D3D12PipelineStreamSubobject<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC>                     m_SampleDesc;
        D3D12PipelineStreamSubobject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK>                                 m_SampleMask;
    };

    #pragma warning( pop )

    using DescriptorHandle = HandleAllocator<GenericResourceHandle>::Handle;

    struct DescriptorAllocator final
    {
        ComPtr<ID3D12DescriptorHeap>                            m_heap = {};
        D3D12_DESCRIPTOR_HEAP_TYPE                              m_heapType = {};

        D3D12_CPU_DESCRIPTOR_HANDLE                             m_startHostHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE                             m_startDeviceHandle = {};

        HandleAllocator<GenericResourceHandle>                  m_descriptorHandleAllocator = {};

        #ifdef USE_RENDERDOC
        ComPtr<IRenderDocDescriptorNamer>                       m_renderDocDescriptorNamer;
        #endif

        uint32_t                                                m_numDescriptors = 0;
        uint32_t                                                m_descriptorSize = 0;

        void Initialize( ID3D12Device* pD3D12Device, D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc );
        void Shutdown();

        DescriptorHandle AllocateDescriptors( uint32_t numDescriptors, char const* pResourceName );
        void FreeDescriptors( DescriptorHandle&& handle );

        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHostHandle( uint64_t handle ) const;
        D3D12_GPU_DESCRIPTOR_HANDLE DescriptorDeviceHandle( uint64_t handle ) const;
    };

    void DescriptorAllocator::Initialize( ID3D12Device* pD3D12Device, D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc )
    {
        HRESULT const result = pD3D12Device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( m_heap.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );
        EE_ASSERT( ( heapDesc.NumDescriptors % 64 ) == 0 );

        #ifdef USE_RENDERDOC
        m_heap->QueryInterface( IID_PPV_ARGS( m_renderDocDescriptorNamer.ReleaseAndGetAddressOf() ) );
        #endif

        m_heapType = heapDesc.Type;
        m_startHostHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
        if ( heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE )
        {
            m_startDeviceHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
        }

        m_numDescriptors = heapDesc.NumDescriptors;
        m_descriptorSize = pD3D12Device->GetDescriptorHandleIncrementSize( m_heapType );

        EE_ASSERT( ( heapDesc.NumDescriptors % 64 ) == 0 );

        m_descriptorHandleAllocator.Initialize( ( heapDesc.NumDescriptors + 63 ) / 64 );
        m_descriptorHandleAllocator.SetIsGrowable( false );
    }

    void DescriptorAllocator::Shutdown()
    {
        m_heap.Reset();
        m_descriptorHandleAllocator.Shutdown();
    }

    DescriptorHandle DescriptorAllocator::AllocateDescriptors( uint32_t numDescriptors, char const* pResourceName )
    {
        D3D12MA::VIRTUAL_ALLOCATION_DESC d3d12VirtualAllocationDesc = {};
        d3d12VirtualAllocationDesc.Size = numDescriptors;

        DescriptorHandle result = m_descriptorHandleAllocator.Allocate( uint16_t( numDescriptors ) );

        #ifdef USE_RENDERDOC
        if ( m_renderDocDescriptorNamer )
        {
            m_renderDocDescriptorNamer->SetName( UINT( result.m_offset ), pResourceName );
        }
        #endif

        return result;
    }

    void DescriptorAllocator::FreeDescriptors( DescriptorHandle&& handle )
    {
        m_descriptorHandleAllocator.Deallocate( eastl::move( handle ) );
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::DescriptorHostHandle( uint64_t const handle ) const
    {
        return { m_startHostHandle.ptr + handle * m_descriptorSize };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::DescriptorDeviceHandle( uint64_t const handle ) const
    {
        return { m_startDeviceHandle.ptr + handle * m_descriptorSize };
    }

    EE_BASE_API GenericResource::~GenericResource() = default;

    struct Direct3D12Queue final : Queue
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Queue>::Handle                  m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12CommandQueue>                              m_queue = {};

        ComPtr<ID3D12Fence>                                     m_fence = {};
        HANDLE                                                  m_fenceEvent = {};
        uint64_t                                                m_fenceValue = 0;

        TVector<ID3D12CommandList*>                             m_submitCommandLists{ Memory::Allocators::g_RHI };

        virtual ~Direct3D12Queue();
    };

    Direct3D12Queue::~Direct3D12Queue()
    {
        CloseHandle( m_fenceEvent );
    }

    struct Direct3D12Buffer final : Buffer
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Buffer>::Handle                 m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12Resource>                                  m_resource = {};
        ComPtr<D3D12MA::Allocation>                             m_allocation = {};
        uint32_t                                                m_nodeMask = 0;
        DescriptorAllocator const*                              m_pHostResourceDescriptorAllocator = nullptr;
        DescriptorAllocator const*                              m_pDeviceResourceDescriptorAllocator = nullptr;
        DescriptorHandle                                        m_hostDescriptorHandles = {};
        DescriptorHandle                                        m_deviceDescriptorHandles = {};
        int8_t                                                  m_srvDescriptorOffset = -1;
        int8_t                                                  m_uavDescriptorOffset = -1;
        D3D12_SUBRESOURCE_FOOTPRINT                             m_footprint = {};
        D3D12_RANGE                                             m_mappedRange = {};
        ComPtr<D3D12MA::VirtualBlock>                           m_virtualBlock = {};
    };

    struct Direct3D12Texture final : Texture
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Texture>::Handle                m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12Resource>                                  m_resource = {};
        ComPtr<D3D12MA::Allocation>                             m_allocation = {};
        DescriptorAllocator const*                              m_pHostResourceDescriptorAllocator = nullptr;
        DescriptorAllocator const*                              m_pDeviceResourceDescriptorAllocator = nullptr;
        DescriptorAllocator const*                              m_pRenderTargetDescriptorAllocator = nullptr;
        DescriptorHandle                                        m_hostResourceDescriptorHandles = {};
        DescriptorHandle                                        m_deviceResourceDescriptorHandles = {};
        DescriptorHandle                                        m_renderTargetDescriptorHandles = {};
        int8_t                                                  m_uavDescriptorOffset = -1;
        TVector<D3D12_SUBRESOURCE_FOOTPRINT>                    m_footprints{ Memory::Allocators::g_RHI };

        GenericResourceHandle ResourceDescriptorHandle( DescriptorTypeFlags descriptorType, uint32_t uavMipLevel ) const;
        D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDescriptorHandle( uint32_t arrayLayer, uint32_t mipLevel ) const;
    };

    GenericResourceHandle Direct3D12Texture::ResourceDescriptorHandle( DescriptorTypeFlags descriptorType, uint32_t uavMipLevel ) const
    {
        EE_ASSERT( m_descriptorTypes.IsFlagSet( descriptorType ) );
        EE_ASSERT( m_deviceResourceDescriptorHandles.IsValid() );

        switch ( descriptorType )
        {
            case DescriptorTypeFlags::Texture:
            case DescriptorTypeFlags::TextureCube:
            {
                return uint16_t( m_deviceResourceDescriptorHandles.m_offset );
            }

            case DescriptorTypeFlags::RWTexture:
            {
                EE_ASSERT( uavMipLevel < m_mipLevels );
                EE_ASSERT( m_uavDescriptorOffset != -1 );

                return uint16_t( m_deviceResourceDescriptorHandles.m_offset ) + uint8_t( m_uavDescriptorOffset ) + uint16_t( uavMipLevel );
            }

            default:
            {
                EE_ASSERT( false );
                return InvalidResourceHandle;
            }
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Direct3D12Texture::RenderTargetDescriptorHandle( uint32_t arrayLayer, uint32_t mipLevel ) const
    {
        EE_ASSERT( m_renderTargetDescriptorHandles.IsValid() );
        return m_pRenderTargetDescriptorAllocator->DescriptorHostHandle( m_renderTargetDescriptorHandles.m_offset + m_mipLevels * arrayLayer + mipLevel );
    }

    struct Direct3D12Sampler final : Sampler
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Sampler>::Handle                m_allocatorHandle = {};
        #endif

        D3D12_SAMPLER_DESC                                      m_samplerDesc = {};

        DescriptorHandle                                        m_hostDescriptorHandle = {};
        DescriptorHandle                                        m_deviceDescriptorHandle = {};
    };

    struct Direct3D12QueryPool : QueryPool
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12QueryPool>::Handle              m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12QueryHeap>                                 m_queryHeap = {};
        D3D12_QUERY_TYPE                                        m_queryType = {};
    };

    struct Direct3D12Shader : Shader
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Shader>::Handle                 m_allocatorHandle = {};
        #endif

        TVector<ComPtr<IDxcBlobEncoding>>                       m_shaderBlobs{ Memory::Allocators::g_RHI };
    };

    struct Direct3D12RootSignature : RootSignature
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12RootSignature>::Handle          m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12RootSignature>                             m_rootSignature = {};
    };

    struct Direct3D12Pipeline : Pipeline
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Pipeline>::Handle               m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12PipelineState>                             m_pipelineState = {};
        ComPtr<ID3D12StateObject>                               m_raytracingState = {};
        D3D_PRIMITIVE_TOPOLOGY                                  m_primitiveTopology = {};
    };

    struct Direct3D12PipelineCache : PipelineCache
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12PipelineCache>::Handle          m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12PipelineLibrary>                           m_pipelineLibrary = {};
        TVector<uint8_t>                                        m_pipelineLibraryData{ Memory::Allocators::g_RHI };
    };

    struct Direct3D12AccelerationStructure : AccelerationStructure
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12AccelerationStructure>::Handle  m_allocatorHandle = {};
        #endif

        struct BottomLevel
        {
            Buffer*                                             m_structureBuffer = {};
            TVector<D3D12_RAYTRACING_GEOMETRY_DESC>             m_geometries{ Memory::Allocators::g_RHI };
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_flags = {};
        };

        BottomLevel                                             m_bottomLevel = {};
        uint64_t                                                m_numInstances;
        Buffer*                                                 m_instanceBuffer = {};
        Buffer*                                                 m_structureBuffer = {};
        Buffer*                                                 m_scratchBuffer = {};
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS     m_flags = {};
    };

    struct Direct3D12RaytracingShaderTable : RaytracingShaderTable
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12RaytracingShaderTable>::Handle  m_allocatorHandle = {};
        #endif

        Buffer*                                                 m_buffer = {};
        uint64_t                                                m_maxEntrySize = 0;
        uint64_t                                                m_missRecordSize = 0;
        uint64_t                                                m_hitGroupRecordSize = 0;
    };

    struct Direct3D12CommandPool : CommandPool
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12CommandPool>::Handle            m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12CommandAllocator>                          m_commandAllocator = {};
    };

    struct Direct3D12CommandBuffer : CommandBuffer
    {
        enum struct Stage
        {
            Invalid,
            Created,
            Recording,
            Closed
        };

        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12CommandBuffer>::Handle          m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12GraphicsCommandList7>                      m_commandList = {};
        D3D12_COMMAND_LIST_TYPE                                 m_commandListType = {};

        DescriptorAllocator const*                              m_pDeviceResourceDescriptorAllocator = nullptr;
        DescriptorAllocator const*                              m_pDeviceSamplerDescriptorAllocator = nullptr;

        TBitFlags<ShadingRateCaps>                              m_shadingRateCaps = {};

        TVector<D3D12_GLOBAL_BARRIER>                           m_globalBarriers{ Memory::Allocators::g_RHI };
        TVector<D3D12_BUFFER_BARRIER>                           m_bufferBarriers{ Memory::Allocators::g_RHI };
        TVector<D3D12_TEXTURE_BARRIER>                          m_textureBarriers{ Memory::Allocators::g_RHI };

        #ifdef USE_AGS
        AGSContext*                                             m_pAmdAgsContext = nullptr;
        #endif

        Stage                                                   m_stage = Stage::Invalid;

        float                                                   m_currentDebugMarkerColorValue = 0.5F;
        int32_t                                                 m_debugMarkerScopeCounter = 0;

        void ResetRootSignature( PipelineType pipelineType, Direct3D12RootSignature* pD3D12RootSignature );
        void ResetPipeline( Direct3D12Pipeline* pD3D12Pipeline );

        void FlushBarriers();
    };

    void Direct3D12CommandBuffer::ResetRootSignature( PipelineType pipelineType, Direct3D12RootSignature* pD3D12RootSignature )
    {
        if ( m_pBoundRootSignature != pD3D12RootSignature )
        {
            m_pBoundRootSignature = pD3D12RootSignature;
            if ( pipelineType == PipelineType::Graphics )
            {
                m_commandList->SetGraphicsRootSignature( pD3D12RootSignature->m_rootSignature.Get() );
            }
            else
            {
                m_commandList->SetComputeRootSignature( pD3D12RootSignature->m_rootSignature.Get() );
            }
        }
    }

    void Direct3D12CommandBuffer::ResetPipeline( Direct3D12Pipeline* pD3D12Pipeline )
    {
        if ( m_pBoundPipeline != pD3D12Pipeline )
        {
            m_pBoundPipeline = pD3D12Pipeline;
            switch ( pD3D12Pipeline->m_pipelineType )
            {
                case PipelineType::Graphics:
                {
                    m_commandList->IASetPrimitiveTopology( pD3D12Pipeline->m_primitiveTopology );
                    m_commandList->SetPipelineState( pD3D12Pipeline->m_pipelineState.Get() );
                }
                break;

                case PipelineType::Compute:
                {
                    m_commandList->SetPipelineState( pD3D12Pipeline->m_pipelineState.Get() );
                }
                break;

                case PipelineType::RayTracing:
                {
                    m_commandList->SetPipelineState1( pD3D12Pipeline->m_raytracingState.Get() );
                }
                break;

                case PipelineType::Undefined:
                {
                    EE_ASSERT( false );
                }
                break;
            }
        }
    }

    void Direct3D12CommandBuffer::FlushBarriers()
    {
        TArray<D3D12_BARRIER_GROUP, 3> barrierGroups = {};
        UINT32 numBarrierGroups = 0;

        if ( !m_globalBarriers.empty() )
        {
            D3D12_BARRIER_GROUP& d3d12BarrierGroup = barrierGroups[numBarrierGroups];

            d3d12BarrierGroup.Type = D3D12_BARRIER_TYPE_GLOBAL;
            d3d12BarrierGroup.NumBarriers = UINT32( m_globalBarriers.size() );
            d3d12BarrierGroup.pGlobalBarriers = m_globalBarriers.data();

            ++numBarrierGroups;
        }

        if ( !m_bufferBarriers.empty() )
        {
            D3D12_BARRIER_GROUP& d3d12BarrierGroup = barrierGroups[numBarrierGroups];

            d3d12BarrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
            d3d12BarrierGroup.NumBarriers = UINT32( m_bufferBarriers.size() );
            d3d12BarrierGroup.pBufferBarriers = m_bufferBarriers.data();

            ++numBarrierGroups;
        }

        if ( !m_textureBarriers.empty() )
        {
            D3D12_BARRIER_GROUP& d3d12BarrierGroup = barrierGroups[numBarrierGroups];

            d3d12BarrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            d3d12BarrierGroup.NumBarriers = UINT32( m_textureBarriers.size() );
            d3d12BarrierGroup.pTextureBarriers = m_textureBarriers.data();

            ++numBarrierGroups;
        }

        if ( numBarrierGroups )
        {
            m_commandList->Barrier( numBarrierGroups, barrierGroups.data() );

            m_globalBarriers.clear();
            m_bufferBarriers.clear();
            m_textureBarriers.clear();
        }
    }

    struct Direct3D12CommandSignature : CommandSignature
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12CommandSignature>::Handle       m_allocatorHandle = {};
        #endif

        ComPtr<ID3D12CommandSignature>                          m_pCommandSignature = {};
    };

    struct Direct3D12Swapchain : Swapchain
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Swapchain>::Handle              m_allocatorHandle = {};
        #endif

        ComPtr<IDXGISwapChain3>                                 m_swapchain = {};
        uint32_t                                                m_presentFlags = 0;
        uint32_t                                                m_syncInterval = 0;
    };

    struct Direct3D12Context : Context
    {
        struct ResourceAllocStats
        {
            uint64_t m_numAllocations = 0;
            uint64_t m_numBytes = 0;
        };


        DescriptorAllocator                                     m_hostSamplerDescriptorAllocator = {};
        DescriptorAllocator                                     m_hostResourceDescriptorAllocator = {};
        DescriptorAllocator                                     m_hostRenderTargetDescriptorAllocator = {};
        DescriptorAllocator                                     m_hostDepthStencilDescriptorAllocator = {};

        TUniquePtr<DescriptorAllocator[]>                       m_deviceDescriptorAllocatorMemory = {};    // All descriptor allocators for all GPUs
        TArrayView<DescriptorAllocator>                         m_deviceSamplerDescriptorAllocators = {};  // One allocator per GPU
        TArrayView<DescriptorAllocator>                         m_deviceResourceDescriptorAllocators = {}; // One allocator per GPU

        ComPtr<D3D12MA::Allocator>                              m_resourceAllocator = {};
        ComPtr<IDxcUtils>                                       m_dxcUtils = {};

        ComPtr<ID3D12Debug>                                     m_debugInterface = {};
        ComPtr<ID3D12InfoQueue>                                 m_debugValidation = {};
        ComPtr<IDXGIFactory6>                                   m_factory = {};
        ComPtr<IDXGIAdapter4>                                   m_adapter = {};
        ComPtr<ID3D12DeviceRemovedExtendedDataSettings>         m_dredSettings = {};

        ComPtr<ID3D12Device10>                                  m_device = {};

        RENDERDOC_API_1_0_0*                                    m_pRenderDocAPI = nullptr;

        #ifdef USE_AGS
        AGSContext*                                             m_pAmdAgsContext = nullptr;
        #endif

        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        PageAllocator<Direct3D12Queue>                          m_queueAllocator;
        PageAllocator<Direct3D12Swapchain>                      m_swapchainAllocator;
        PageAllocator<Direct3D12CommandPool>                    m_commandPoolAllocator;
        PageAllocator<Direct3D12CommandBuffer>                  m_commandBufferAllocator;
        PageAllocator<Direct3D12CommandSignature>               m_commandSignatureAllocator;
        PageAllocator<Direct3D12AccelerationStructure>          m_accelerationStructureAllocator;
        PageAllocator<Direct3D12Buffer>                         m_bufferAllocator;
        PageAllocator<Direct3D12Texture>                        m_textureAllocator;
        PageAllocator<Direct3D12Sampler>                        m_samplerAllocator;
        PageAllocator<Direct3D12Shader>                         m_shaderAllocator;
        PageAllocator<Direct3D12RootSignature>                  m_rootSignatureAllocator;
        PageAllocator<Direct3D12PipelineCache>                  m_pipelineCacheAllocator;
        PageAllocator<Direct3D12Pipeline>                       m_pipelineAllocator;
        PageAllocator<Direct3D12QueryPool>                      m_queryPoolAllocator;
        #endif

        // Per-descriptor-type allocation statistics
        THashMap<TBitFlags<DescriptorTypeFlags>, ResourceAllocStats>    m_bufferStats{ Memory::Allocators::g_RHI };
        THashMap<TBitFlags<DescriptorTypeFlags>, ResourceAllocStats>    m_textureStats{ Memory::Allocators::g_RHI };

        UINT NodeMask( uint32_t nodeIndex ) const;
        UINT SharedNodeMask() const;

        template <typename T>
        T* CreateObject();

        template <typename T>
        void DestroyObject( T*&& pObject );
    };

    UINT Direct3D12Context::NodeMask( uint32_t nodeIndex ) const
    {
        if ( m_deviceMode == DeviceMode::Linked )
        {
            return 1 << nodeIndex;
        }
        else
        {
            return 0;
        }
    }

    UINT Direct3D12Context::SharedNodeMask() const
    {
        if ( m_deviceMode == DeviceMode::Linked )
        {
            return ( 1 << m_numLinkedNodes ) - 1;
        }
        else
        {
            return 0;
        }
    }

    template <typename T>
    T* Direct3D12Context::CreateObject()
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        typename PageAllocator<T>::Handle handle = {};

        if constexpr ( std::is_same_v<T, Direct3D12Queue> ) { handle = m_queueAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12Swapchain> ) { handle = m_swapchainAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12CommandPool> ) { handle = m_commandPoolAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12CommandBuffer> ) { handle = m_commandBufferAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12CommandSignature> ) { handle = m_commandSignatureAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12AccelerationStructure> ) { handle = m_accelerationStructureAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12Buffer> ) { handle = m_bufferAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12Texture> ) { handle = m_textureAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12Sampler> ) { handle = m_samplerAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12Shader> ) { handle = m_shaderAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12RootSignature> ) { handle = m_rootSignatureAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12PipelineCache> ) { handle = m_pipelineCacheAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12Pipeline> ) { handle = m_pipelineAllocator.Allocate( 1 ); }
        if constexpr ( std::is_same_v<T, Direct3D12QueryPool> ) { handle = m_queryPoolAllocator.Allocate( 1 ); }

        EE_ASSERT( handle.m_handle.IsValid() );

        handle.m_data.data()->m_allocatorHandle = handle;
        return handle.m_data.data();
        #else
        void* pObjectMemory = Memory::Allocators::g_RHI.Alloc( sizeof( T ), alignof( T ) );
        return new( pObjectMemory ) T();
        #endif
    }

    template <typename T>
    void Direct3D12Context::DestroyObject( T*&& pObject )
    {
        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        if ( pObject )
        {
            typename PageAllocator<T>::Handle handle = pObject->m_allocatorHandle;

            if constexpr ( std::is_same_v<T, Direct3D12Queue> ) { m_queueAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12Swapchain> ) { m_swapchainAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12CommandPool> ) { m_commandPoolAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12CommandBuffer> ) { m_commandBufferAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12CommandSignature> ) { m_commandSignatureAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12AccelerationStructure> ) { m_accelerationStructureAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12Buffer> ) { m_bufferAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12Texture> ) { m_textureAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12Sampler> ) { m_samplerAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12Shader> ) { m_shaderAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12RootSignature> ) { m_rootSignatureAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12PipelineCache> ) { m_pipelineCacheAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12Pipeline> ) { m_pipelineAllocator.Deallocate( eastl::move( handle ) ); }
            if constexpr ( std::is_same_v<T, Direct3D12QueryPool> ) { m_queryPoolAllocator.Deallocate( eastl::move( handle ) ); }

            EE_ASSERT( !handle.m_handle.IsValid() );
        }
        #else
        pObject->~T();
        Memory::Allocators::g_RHI.Free( (void*&) pObject );
        #endif
    }

    static void TrackResourceAllocation( Direct3D12Context* pCtx, TBitFlags<DescriptorTypeFlags> descriptorTypes, bool isTexture, bool isAllocation, D3D12MA::Allocation* pAllocation )
    {
        if ( !pAllocation ) { return; }

        uint64_t numBytes = pAllocation->GetSize();

        auto& stats = isTexture ? pCtx->m_textureStats : pCtx->m_bufferStats;
        auto it = stats.find( descriptorTypes );
        if ( it == stats.end() )
        {
            Direct3D12Context::ResourceAllocStats entry;
            entry.m_numAllocations = isAllocation ? 1 : 0;
            entry.m_numBytes = isAllocation ? numBytes : 0;
            stats.insert( { descriptorTypes, entry } );
        }
        else
        {
            if ( isAllocation )
            {
                it->second.m_numAllocations++;
                it->second.m_numBytes += numBytes;
            }
            else
            {
                it->second.m_numAllocations--;
                it->second.m_numBytes -= numBytes;
            }
        }
    }

    EE_BASE_API Context* CreateContext( ContextParameters const& parameters )
    {
        bool isWindowsVersionSupported = ValidateWindowsVersion();
        EE_ASSERT( isWindowsVersionSupported );

        if ( !isWindowsVersionSupported )
        {
            return nullptr;
        }

        void* pContextMemory = Memory::Allocators::g_RHI.Alloc( sizeof( Direct3D12Context ), alignof( Direct3D12Context ) );

        Direct3D12Context* pD3D12Context = new ( pContextMemory ) Direct3D12Context();

        #ifdef USE_PIX
        if ( parameters.m_enablePix )
        {
            LoadPixGpuCapturerDLL();
        }
        #endif

        bool renderDocConnected = false;
        #ifdef USE_RENDERDOC
        if ( parameters.m_enableRenderDoc )
        {
            if ( HMODULE renderDocModule = GetModuleHandleA( "renderdoc.dll" ) )
            {
                pRENDERDOC_GetAPI renderdoc_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>( GetProcAddress( renderDocModule, "RENDERDOC_GetAPI" ) ); // NOLINT(clang-diagnostic-cast-function-type-strict)
                int               result = renderdoc_GetAPI( eRENDERDOC_API_Version_1_0_0, reinterpret_cast<void**>( &pD3D12Context->m_pRenderDocAPI ) );
                EE_ASSERT( result == 1 );

                EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "RenderDoc connected" );
                renderDocConnected = true;
            }
        }
        #endif

        #ifdef USE_AGS
        if ( parameters.m_enableAmdAgs )
        {
            AGSGPUInfo gpuInfo = {};
            AGSConfiguration config = {};
            AGSDX12ReturnedParams agsParams = {};

            if ( AGSReturnCode returnCode = agsInitialize( AGS_CURRENT_VERSION, &config, &pD3D12Context->m_pAmdAgsContext, &gpuInfo ); returnCode == AGS_SUCCESS )
            {
                EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "AMD AGS Initialized, Radeon Software Version: %s, Driver Version: %s", gpuInfo.radeonSoftwareVersion, gpuInfo.driverVersion );
                pD3D12Context->m_amdAgsEnabled = true;
            }
            else
            {
                // Do not spam into log on non AMD GPUs
                if ( returnCode != AGS_NO_AMD_DRIVER_INSTALLED )
                {
                    EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "AMD AGS was requested but failed to initialize" );
                }
            }
        }
        #endif

        UINT factoryFlags = 0;
        if ( parameters.m_enableHostValidation || parameters.m_enableDeviceValidation )
        {
            if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( pD3D12Context->m_debugInterface.ReleaseAndGetAddressOf() ) ) ) )
            {
                factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
            else
            {
                EE_LOG_WARNING( LogCategory::Render, "RHI/CreateContext", "Failed to initialize D3D12 validation" );
            }
        }

        if ( parameters.m_enableHostValidation && pD3D12Context->m_debugInterface )
        {
            pD3D12Context->m_debugInterface->EnableDebugLayer();
        }

        if ( parameters.m_enableDeviceValidation && pD3D12Context->m_debugInterface )
        {
            // RenderDoc does not support device validation, disable it when detected
            if ( !renderDocConnected )
            {
                ComPtr<ID3D12Debug1> d3d12Debug1 = {};
                if ( SUCCEEDED( pD3D12Context->m_debugInterface->QueryInterface( IID_PPV_ARGS( d3d12Debug1.ReleaseAndGetAddressOf() ) ) ) )
                {
                    d3d12Debug1->SetEnableGPUBasedValidation( TRUE );
                }
            }

            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }

        if ( parameters.m_enableDeviceBreadcrumbs )
        {
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

            // Not all GPUs support DRED, need to check device capabilities first.
            // Actual DRED initialization is done after device is created and capabilities validated.
        }

        HRESULT result = CreateDXGIFactory2( factoryFlags, IID_PPV_ARGS( pD3D12Context->m_factory.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        D3D_FEATURE_LEVEL preferredFeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
        };

        DXGI_ADAPTER_DESC3      dxgiAdapterDesc = {};
        ComPtr<IDXGIAdapter4>   dxgiAdapter = {};
        ComPtr<ID3D12Device10>  d3d12Device = {};

        DXGI_GPU_PREFERENCE dxgiGPUPreference = {};
        switch ( parameters.m_deviceSelection )
        {
            case DeviceSelectionPreference::PreferPerformance:
            {
                dxgiGPUPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            }
            break;

            case DeviceSelectionPreference::PreferPowerEfficiency:
            {
                dxgiGPUPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }
            break;

            case DeviceSelectionPreference::UseProvidedIndex:
            {
                dxgiGPUPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED;
            }
            break;
        }

        if ( parameters.m_deviceSelection == DeviceSelectionPreference::UseProvidedIndex )
        {
            result = pD3D12Context->m_factory->EnumAdapterByGpuPreference( parameters.m_deviceIndex, dxgiGPUPreference, IID_PPV_ARGS( dxgiAdapter.ReleaseAndGetAddressOf() ) );
            EE_ASSERT( result != DXGI_ERROR_NOT_FOUND );
            result = dxgiAdapter->GetDesc3( &dxgiAdapterDesc );
            EE_ASSERT( SUCCEEDED( result ) );

            for ( auto const& preferredFeatureLevel : preferredFeatureLevels )
            {
                result = D3D12CreateDevice( dxgiAdapter.Get(), preferredFeatureLevel, IID_PPV_ARGS( d3d12Device.ReleaseAndGetAddressOf() ) );
                if ( SUCCEEDED( result ) )
                {
                    break;
                }
            }
        }
        else
        {
            for ( UINT adapterIndex = 0; pD3D12Context->m_factory->EnumAdapterByGpuPreference( adapterIndex, dxgiGPUPreference, IID_PPV_ARGS( dxgiAdapter.ReleaseAndGetAddressOf() ) ) != DXGI_ERROR_NOT_FOUND; ++adapterIndex )
            {
                result = dxgiAdapter->GetDesc3( &dxgiAdapterDesc );
                EE_ASSERT( SUCCEEDED( result ) );

                if ( dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE )
                {
                    continue;
                }

                for ( auto const& preferredFeatureLevel : preferredFeatureLevels )
                {
                    #ifdef USE_AGS
                    if ( pD3D12Context->m_amdAgsEnabled )
                    {
                        AGSDX12DeviceCreationParams creationParams = {};
                        creationParams.pAdapter = dxgiAdapter.Get();
                        creationParams.iid = IID_PPV_ARGS( d3d12Device.ReleaseAndGetAddressOf() );
                        creationParams.FeatureLevel = preferredFeatureLevel;

                        AGSDX12ExtensionParams extensionParams = {};
                        AGSDX12ReturnedParams returnedParams = {};

                        AGSReturnCode rc = agsDriverExtensionsDX12_CreateDevice( pD3D12Context->m_pAmdAgsContext, &creationParams, &extensionParams, &returnedParams );

                        if ( rc == AGSReturnCode::AGS_SUCCESS )
                        {
                            result = returnedParams.pDevice->QueryInterface( IID_PPV_ARGS( d3d12Device.ReleaseAndGetAddressOf() ) );
                            EE_ASSERT( SUCCEEDED( result ) );
                            break;
                        }
                    }
                    else
                        #endif
                    {
                        if ( SUCCEEDED( D3D12CreateDevice( dxgiAdapter.Get(), preferredFeatureLevel, IID_PPV_ARGS( d3d12Device.ReleaseAndGetAddressOf() ) ) ) )
                        {
                            break;
                        }
                    }
                }

                if ( d3d12Device.Get() )
                {
                    break;
                }
            }
        }

        EE_ASSERT( dxgiAdapter.Get() );
        EE_ASSERT( d3d12Device.Get() );

        EE_LOG_MESSAGE
        (
            LogCategory::Render, "RHI/CreateContext", "GPU %s %s %s",
            FormatVendorID<MaxVendorNameLength>( dxgiAdapterDesc.VendorId ).c_str(),
            FormatVendorID<MaxVendorNameLength>( dxgiAdapterDesc.DeviceId ).c_str(),
            FormatVendorName<MaxVendorNameLength>( dxgiAdapterDesc.Description ).c_str()
        );

        pD3D12Context->m_adapter = dxgiAdapter;
        pD3D12Context->m_device = d3d12Device;

        uint32_t numNodes = d3d12Device->GetNodeCount();
        pD3D12Context->m_numLinkedNodes = numNodes;
        if ( numNodes > 1 )
        {
            pD3D12Context->m_deviceMode = DeviceMode::Linked;
        }
        else
        {
            pD3D12Context->m_deviceMode = DeviceMode::Single;
        }

        if ( parameters.m_enableHostValidation )
        {
            if ( FAILED( d3d12Device->QueryInterface( IID_PPV_ARGS( pD3D12Context->m_debugValidation.ReleaseAndGetAddressOf() ) ) ) )
            {
                EE_LOG_WARNING( LogCategory::Render, "RHI/CreateContext", "Failed to initialize host validation" );
            }

            if ( pD3D12Context->m_debugValidation )
            {
                if ( parameters.m_enablePix && PIXIsAttachedForGpuCapture() )
                {
                    // Don't ignore corruptions, this is fatal
                    pD3D12Context->m_debugValidation->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, true );

                    // Ignore all validation warnings caused by PIX but log them just in case.
                    pD3D12Context->m_debugValidation->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, false );
                    pD3D12Context->m_debugValidation->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, false );
                }
                else
                {
                    pD3D12Context->m_debugValidation->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, true );
                    pD3D12Context->m_debugValidation->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, true );
                    pD3D12Context->m_debugValidation->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, false );
                }

                ComPtr<ID3D12InfoQueue1> infoQueue1 = {};
                if ( SUCCEEDED( pD3D12Context->m_debugValidation->QueryInterface( IID_PPV_ARGS( infoQueue1.ReleaseAndGetAddressOf() ) ) ) )
                {
                    auto MessageCallback = [] ( D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR pDescription, void* pContext )
                    {
                        // Check that we have a valid memory heap for this thread, since this call can occur via the GPU validation code we might not have initialized it for this thread
                        if ( !Memory::HasInitializedThreadHeap() )
                        {
                            Memory::InitializeThreadHeap();
                        }

                        // Filter out some warnings that aren't useful
                        if ( id == D3D12_MESSAGE_ID_NON_OPTIMAL_BARRIER_ONLY_EXECUTE_COMMAND_LISTS )
                        {
                            return;
                        }

                        if ( severity == D3D12_MESSAGE_SEVERITY_INFO )
                        {
                            // Ignore, not interesting
                        }
                        else if ( severity == D3D12_MESSAGE_SEVERITY_MESSAGE )
                        {
                            EE_LOG_MESSAGE( LogCategory::Render, "RHI/D3D12", pDescription );
                        }
                        else if ( severity == D3D12_MESSAGE_SEVERITY_WARNING )
                        {
                            EE_LOG_WARNING( LogCategory::Render, "RHI/D3D12", pDescription );
                        }
                        else
                        {
                            if ( PIXIsAttachedForGpuCapture() )
                            {
                                // Ignore all validation warnings caused by PIX but log them just in case.
                                EE_LOG_WARNING( LogCategory::Render, "RHI/D3D12", pDescription );
                            }
                            else
                            {
                                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/D3D12", pDescription );
                            }
                        }
                    };

                    DWORD callbackCookie = 0;
                    HRESULT hr = infoQueue1->RegisterMessageCallback( MessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie );

                    if ( callbackCookie == 0 )
                    {
                        EE_LOG_ERROR( LogCategory::Render, "RHI/D3D12", "Failed to register debug message callback" );
                    }
                }
            }
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS  featureData = {};
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 featureData1 = {};

        result = d3d12Device->CheckFeatureSupport( D3D12_FEATURE( D3D12_FEATURE_D3D12_OPTIONS ), &featureData, sizeof( featureData ) );
        EE_ASSERT( SUCCEEDED( result ) );
        result = d3d12Device->CheckFeatureSupport( D3D12_FEATURE( D3D12_FEATURE_D3D12_OPTIONS1 ), &featureData1, sizeof( featureData1 ) );
        EE_ASSERT( SUCCEEDED( result ) );

        DeviceVendorInfo vendorInfo = {};
        vendorInfo.m_vendorID = FormatVendorID<MaxVendorNameLength>( dxgiAdapterDesc.VendorId );
        vendorInfo.m_deviceID = FormatVendorID<MaxVendorNameLength>( dxgiAdapterDesc.DeviceId );
        vendorInfo.m_revisionID = FormatVendorID<MaxVendorNameLength>( dxgiAdapterDesc.Revision );
        vendorInfo.m_deviceName = FormatVendorName<MaxVendorNameLength>( dxgiAdapterDesc.Description );

        pD3D12Context->m_deviceCapabilities.m_dedicatedVideoMemory = dxgiAdapterDesc.DedicatedVideoMemory;
        pD3D12Context->m_deviceCapabilities.m_constantBufferAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        pD3D12Context->m_deviceCapabilities.m_uploadBufferTextureAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
        pD3D12Context->m_deviceCapabilities.m_uploadBufferTextureRowAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

        // 13 is AMD recommendation that also works great on NV.
        // AMD HW works with packets, each packet is 16 DWORDs.
        // Packets are used, among other things, to encode draw and dispatch calls.
        // 3 DWORDs are needed to encode draw call or dispatch parameters, thus leaving 13 DWORDs free.
        // So if the root signature is <= 13 DWORDs then the driver can store it in the packet itself.
        // When stored in the packet, the command processor can preload it into SGPR before the shader starts executing.
        // This saves one memory indirection per draw or dispatch call when accessing root constants.
        // Savings should be pretty miniscule so feel free to ignore it.
        pD3D12Context->m_deviceCapabilities.m_optimalRootSignatureSizeInDWORDs = 13;

        pD3D12Context->m_deviceCapabilities.m_numWaveLanes = featureData1.WaveLaneCountMin;
        pD3D12Context->m_deviceCapabilities.m_waveOpsSupportFlags.SetAllFlags();

        // TODO: Shading rate not implemented yet
        pD3D12Context->m_deviceCapabilities.m_shadingRate = ShadingRate::NotSupported;
        pD3D12Context->m_deviceCapabilities.m_shadingRateCaps = ShadingRateCaps::NotSupported;
        pD3D12Context->m_deviceCapabilities.m_shadingRateTexelWidth = 0;
        pD3D12Context->m_deviceCapabilities.m_shadingRateTexelHeight = 0;

        pD3D12Context->m_deviceCapabilities.m_multiDrawIndirect = true;
        pD3D12Context->m_deviceCapabilities.m_indirectRootConstant = true;
        pD3D12Context->m_deviceCapabilities.m_rasterizerOrderViews = featureData.ROVsSupported;

        if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( pD3D12Context->m_dredSettings.ReleaseAndGetAddressOf() ) ) ) )
        {
            pD3D12Context->m_deviceCapabilities.m_breadcrumbs = true;
        }

        pD3D12Context->m_deviceCapabilities.m_hdr = true;

        if ( parameters.m_enableDeviceBreadcrumbs )
        {
            // RenderDoc does not support DRED, disable it when detected
            if ( !renderDocConnected )
            {
                result = pD3D12Context->m_deviceCapabilities.m_breadcrumbs;

                pD3D12Context->m_dredSettings->SetAutoBreadcrumbsEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
                pD3D12Context->m_dredSettings->SetPageFaultEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
            }
        }

        for ( uint32_t formatIndex = 0; formatIndex < NumDataFormats; ++formatIndex )
        {
            DXGI_FORMAT dxgiFormat = DXGIFormat( DataFormat( formatIndex ) );
            if ( dxgiFormat == DXGI_FORMAT_UNKNOWN )
            {
                continue;
            }

            D3D12_FEATURE_DATA_FORMAT_SUPPORT dxgiFormatSupport = {};
            dxgiFormatSupport.Format = dxgiFormat;

            if ( SUCCEEDED( d3d12Device->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &dxgiFormatSupport, sizeof( dxgiFormatSupport ) ) ) )
            {
                pD3D12Context->m_deviceCapabilities.m_canShaderReadFrom[formatIndex] = ( dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE ) != 0;
                pD3D12Context->m_deviceCapabilities.m_canShaderWriteTo[formatIndex] = ( dxgiFormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE ) != 0;
                pD3D12Context->m_deviceCapabilities.m_canRenderTargetWriteTo[formatIndex] = ( dxgiFormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET ) != 0;
            }
        }

        pD3D12Context->m_deviceDescriptorAllocatorMemory.reset( EE::NewArray<DescriptorAllocator>( numNodes * 2 ) );
        pD3D12Context->m_deviceResourceDescriptorAllocators = TArrayView<DescriptorAllocator>( pD3D12Context->m_deviceDescriptorAllocatorMemory.get(), numNodes );
        pD3D12Context->m_deviceSamplerDescriptorAllocators = TArrayView<DescriptorAllocator>( pD3D12Context->m_deviceDescriptorAllocatorMemory.get() + numNodes, numNodes );

        D3D12_DESCRIPTOR_HEAP_DESC d3d12HostSamplerDescriptorHeapDesc = {};
        d3d12HostSamplerDescriptorHeapDesc.NumDescriptors = 2048;
        d3d12HostSamplerDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        pD3D12Context->m_hostSamplerDescriptorAllocator.Initialize( d3d12Device.Get(), d3d12HostSamplerDescriptorHeapDesc );

        EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "Sampler descriptor pool size: %i", d3d12HostSamplerDescriptorHeapDesc.NumDescriptors );

        D3D12_DESCRIPTOR_HEAP_DESC d3d12HostResourceDescriptorHeapDesc = {};
        d3d12HostResourceDescriptorHeapDesc.NumDescriptors = 64 * 1023;
        d3d12HostResourceDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        pD3D12Context->m_hostResourceDescriptorAllocator.Initialize( d3d12Device.Get(), d3d12HostResourceDescriptorHeapDesc );

        EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "ShaderResource descriptor pool size: %i", d3d12HostResourceDescriptorHeapDesc.NumDescriptors );

        D3D12_DESCRIPTOR_HEAP_DESC d3d12HostRenderTargetDescriptorHeapDesc = {};
        d3d12HostRenderTargetDescriptorHeapDesc.NumDescriptors = 64 * 1023;
        d3d12HostRenderTargetDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        pD3D12Context->m_hostRenderTargetDescriptorAllocator.Initialize( d3d12Device.Get(), d3d12HostRenderTargetDescriptorHeapDesc );

        EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "RenderTarget descriptor pool size: %i", d3d12HostRenderTargetDescriptorHeapDesc.NumDescriptors );

        D3D12_DESCRIPTOR_HEAP_DESC d3d12HostDepthStencilDescriptorHeapDesc = {};
        d3d12HostDepthStencilDescriptorHeapDesc.NumDescriptors = 64 * 1023;
        d3d12HostDepthStencilDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        pD3D12Context->m_hostDepthStencilDescriptorAllocator.Initialize( d3d12Device.Get(), d3d12HostDepthStencilDescriptorHeapDesc );

        EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateContext", "DepthStencil descriptor pool size: %i", d3d12HostDepthStencilDescriptorHeapDesc.NumDescriptors );

        for ( uint32_t nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex )
        {
            D3D12_DESCRIPTOR_HEAP_DESC d3d12DeviceSamplerDescriptorHeapDesc = {};
            d3d12DeviceSamplerDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            d3d12DeviceSamplerDescriptorHeapDesc.NodeMask = pD3D12Context->NodeMask( nodeIndex );
            d3d12DeviceSamplerDescriptorHeapDesc.NumDescriptors = d3d12HostSamplerDescriptorHeapDesc.NumDescriptors;
            d3d12DeviceSamplerDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

            pD3D12Context->m_deviceSamplerDescriptorAllocators[nodeIndex].Initialize( d3d12Device.Get(), d3d12DeviceSamplerDescriptorHeapDesc );

            D3D12_DESCRIPTOR_HEAP_DESC d3d12DeviceResourceDescriptorHeapDesc = {};
            d3d12DeviceResourceDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            d3d12DeviceResourceDescriptorHeapDesc.NodeMask = pD3D12Context->NodeMask( nodeIndex );
            d3d12DeviceResourceDescriptorHeapDesc.NumDescriptors = d3d12HostResourceDescriptorHeapDesc.NumDescriptors;
            d3d12DeviceResourceDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

            pD3D12Context->m_deviceResourceDescriptorAllocators[nodeIndex].Initialize( d3d12Device.Get(), d3d12DeviceResourceDescriptorHeapDesc );
        }

        D3D12MA::ALLOCATION_CALLBACKS d3d12AllocationCallbacks = {};
        d3d12AllocationCallbacks.pAllocate = [] ( size_t size, size_t alignment, void* )
        {
            return Memory::Allocators::g_D3D12.Alloc( size, alignment );
        };
        d3d12AllocationCallbacks.pFree = [] ( void* pPtr, void* )
        {
            if ( pPtr )
            {
                Memory::Allocators::g_D3D12.Free( pPtr );
            }
        };

        D3D12MA::ALLOCATOR_DESC d3d12AllocatorDesc = {};
        d3d12AllocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        d3d12AllocatorDesc.pDevice = d3d12Device.Get();
        d3d12AllocatorDesc.pAdapter = dxgiAdapter.Get();
        d3d12AllocatorDesc.pAllocationCallbacks = &d3d12AllocationCallbacks;

        result = D3D12MA::CreateAllocator( &d3d12AllocatorDesc, pD3D12Context->m_resourceAllocator.ReleaseAndGetAddressOf() );
        EE_ASSERT( SUCCEEDED( result ) );

        result = DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( pD3D12Context->m_dxcUtils.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        pD3D12Context->m_hostValidation = parameters.m_enableHostValidation;
        pD3D12Context->m_deviceValidation = parameters.m_enableDeviceValidation;
        pD3D12Context->m_deviceBreadcrumbs = parameters.m_enableDeviceBreadcrumbs;
        pD3D12Context->m_renderDoc = parameters.m_enableRenderDoc;
        pD3D12Context->m_amdAgsEnabled = parameters.m_enableAmdAgs;

        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        uint32_t const pageAllocatorUpperBound = UINT16_MAX;

        pD3D12Context->m_queueAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_swapchainAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_commandPoolAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_commandBufferAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_commandSignatureAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_accelerationStructureAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_bufferAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_textureAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_samplerAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_shaderAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_rootSignatureAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_pipelineCacheAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_pipelineAllocator.Initialize( 1, pageAllocatorUpperBound );
        pD3D12Context->m_queryPoolAllocator.Initialize( 1, pageAllocatorUpperBound );
        #endif

        return pD3D12Context;
    }

    EE_BASE_API void DestroyContext( Context*&& pContext )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );

        #ifdef ENABLE_VERBOSE_MEMORY_LEAK_DEBUG
        if ( pD3D12Context )
        {
            pD3D12Context->m_queueAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Queue 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_swapchainAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Swapchain 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_commandPoolAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::CommandPool 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_commandBufferAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::CommandBuffer 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_commandSignatureAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::CommandSignature 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_accelerationStructureAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::AccelerationStructure 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_bufferAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Buffer 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_textureAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Texture 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_samplerAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Sampler 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_shaderAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Shader 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_rootSignatureAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::RootSignature 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_pipelineCacheAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::PipelineCache 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_pipelineAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::Pipeline 0x%p", pD3D12Object );
            } );
            pD3D12Context->m_queryPoolAllocator.ForEachAllocatedItem( [] ( auto const* pD3D12Object )
            {
                EE_LOG_FATAL_ERROR( LogCategory::Render, "RHI/Context", "Memory leak detected: RHI::QueryPool 0x%p", pD3D12Object );
            } );

            pD3D12Context->m_queueAllocator.Shutdown();
            pD3D12Context->m_swapchainAllocator.Shutdown();
            pD3D12Context->m_commandPoolAllocator.Shutdown();
            pD3D12Context->m_commandBufferAllocator.Shutdown();
            pD3D12Context->m_commandSignatureAllocator.Shutdown();
            pD3D12Context->m_accelerationStructureAllocator.Shutdown();
            pD3D12Context->m_bufferAllocator.Shutdown();
            pD3D12Context->m_textureAllocator.Shutdown();
            pD3D12Context->m_samplerAllocator.Shutdown();
            pD3D12Context->m_shaderAllocator.Shutdown();
            pD3D12Context->m_rootSignatureAllocator.Shutdown();
            pD3D12Context->m_pipelineCacheAllocator.Shutdown();
            pD3D12Context->m_pipelineAllocator.Shutdown();
            pD3D12Context->m_queryPoolAllocator.Shutdown();
        }
        #endif

        //---------------------------------------------------------------------------------------------------

        pD3D12Context->m_hostSamplerDescriptorAllocator.Shutdown();
        pD3D12Context->m_hostResourceDescriptorAllocator.Shutdown();
        pD3D12Context->m_hostRenderTargetDescriptorAllocator.Shutdown();
        pD3D12Context->m_hostDepthStencilDescriptorAllocator.Shutdown();

        for ( DescriptorAllocator& deviceSamplerDescriptorAllocator : pD3D12Context->m_deviceSamplerDescriptorAllocators )
        {
            deviceSamplerDescriptorAllocator.Shutdown();
        }

        for ( DescriptorAllocator& deviceResourceDescriptorAllocator : pD3D12Context->m_deviceResourceDescriptorAllocators )
        {
            deviceResourceDescriptorAllocator.Shutdown();
        }

        pD3D12Context->m_deviceDescriptorAllocatorMemory.reset();

        pD3D12Context->m_resourceAllocator.Reset();
        pD3D12Context->m_dxcUtils.Reset();

        pD3D12Context->m_debugInterface.Reset();
        pD3D12Context->m_debugValidation.Reset();
        pD3D12Context->m_factory.Reset();
        pD3D12Context->m_adapter.Reset();
        pD3D12Context->m_dredSettings.Reset();

        #ifdef USE_AGS
        if ( pD3D12Context->m_amdAgsEnabled )
        {
            agsDriverExtensionsDX12_DestroyDevice( pD3D12Context->m_pAmdAgsContext, pD3D12Context->m_device.Get(), nullptr );
            agsDeInitialize( pD3D12Context->m_pAmdAgsContext );
        }
        #endif

        pD3D12Context->m_device.Reset();

        // Verify all per-descriptor-type allocations have been freed
        for ( auto const& pair : pD3D12Context->m_bufferStats )
        {
            EE_ASSERT( pair.second.m_numAllocations == 0 );
            EE_ASSERT( pair.second.m_numBytes == 0 );
        }

        for ( auto const& pair : pD3D12Context->m_textureStats )
        {
            EE_ASSERT( pair.second.m_numAllocations == 0 );
            EE_ASSERT( pair.second.m_numBytes == 0 );
        }

        pD3D12Context->~Direct3D12Context();
        Memory::Allocators::g_RHI.Free( (void*&) pD3D12Context );
    }

    EE_BASE_API uint64_t GetTotalAllocatedDeviceMemory( Context* pContext )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );

        D3D12MA::Budget localBudget = {};
        D3D12MA::Budget nonLocalBudget = {};
        pD3D12Context->m_resourceAllocator->GetBudget( &localBudget, &nonLocalBudget );

        return localBudget.UsageBytes + nonLocalBudget.UsageBytes;
    }

    EE_BASE_API void GetDetailedMemoryStatistics( Context* pContext, uint64_t& localUsageBytes, uint64_t& localAvailableBytes, uint64_t& nonLocalUsageBytes, uint64_t& nonLocalAvailableBytes )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );

        D3D12MA::Budget localBudget = {};
        D3D12MA::Budget nonLocalBudget = {};
        pD3D12Context->m_resourceAllocator->GetBudget( &localBudget, &nonLocalBudget );

        localUsageBytes = localBudget.UsageBytes;
        localAvailableBytes = localBudget.BudgetBytes;

        nonLocalUsageBytes = nonLocalBudget.UsageBytes;
        nonLocalAvailableBytes = nonLocalBudget.BudgetBytes;
    }

    EE_BASE_API void GetResourceAllocationStatistics( Context* pContext, TVector<ResourceAllocationStatistic>& outBufferStats, TVector<ResourceAllocationStatistic>& outTextureStats )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );

        outBufferStats.clear();
        outTextureStats.clear();

        for ( auto const& pair : pD3D12Context->m_bufferStats )
        {
            outBufferStats.push_back( { pair.first, pair.second.m_numAllocations, pair.second.m_numBytes } );
        }

        for ( auto const& pair : pD3D12Context->m_textureStats )
        {
            outTextureStats.push_back( { pair.first, pair.second.m_numAllocations, pair.second.m_numBytes } );
        }
    }

    EE_BASE_API void BeginFrameCapture( Context* pContext )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );

        if ( pD3D12Context->m_renderDoc && pD3D12Context->m_pRenderDocAPI )
        {
            pD3D12Context->m_pRenderDocAPI->StartFrameCapture( pD3D12Context->m_device.Get(), nullptr );
        }
    }

    EE_BASE_API void EndFrameCapture( Context* pContext )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );

        if ( pD3D12Context->m_renderDoc && pD3D12Context->m_pRenderDocAPI )
        {
            pD3D12Context->m_pRenderDocAPI->EndFrameCapture( pD3D12Context->m_device.Get(), nullptr );
        }
    }

    EE_BASE_API Queue* CreateQueue( Context* pContext, QueueParameters const& parameters )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12Queue*   pD3D12Queue = pD3D12Context->CreateObject<Direct3D12Queue>();

        uint32_t nodeIndex = parameters.m_nodeIndex;
        EE_ASSERT( ( nodeIndex == 0 ) || pD3D12Context->m_deviceMode == DeviceMode::Unlinked );

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12CommandBufferType( parameters.m_queueType );
        queueDesc.Flags = D3D12QueueFlags( parameters.m_flags );
        queueDesc.Priority = D3D12QueuePriority( parameters.m_priority );
        queueDesc.NodeMask = nodeIndex;

        HRESULT result = pD3D12Context->m_device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( pD3D12Queue->m_queue.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12Queue, parameters.m_debugName );

        result = pD3D12Context->m_device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( pD3D12Queue->m_fence.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        pD3D12Queue->m_fenceValue = 1;
        pD3D12Queue->m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );

        pD3D12Queue->m_queueType = parameters.m_queueType;
        pD3D12Queue->m_nodeIndex = nodeIndex;
        pD3D12Queue->m_unifiedMemory = pD3D12Context->m_resourceAllocator->IsUMA();

        return pD3D12Queue;
    }

    EE_BASE_API void DestroyQueue( Context* pContext, Queue*&& pQueue )
    {
        if ( pQueue )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );

            pD3D12Context->DestroyObject( eastl::move( pD3D12Queue ) );
            pQueue = nullptr;
        }
    }

    EE_BASE_API uint64_t QueueGetCurrentSemaphore( Queue* pQueue )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );
        return pD3D12Queue->m_fenceValue;
    }

    EE_BASE_API uint64_t QueueGetCompletedSemaphore( Queue* pQueue )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );
        return pD3D12Queue->m_fence->GetCompletedValue();
    }

    EE_BASE_API void QueueHostWait( Queue* pQueue, uint64_t semaphore )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );

        UINT64 d3d12FenceValue = pD3D12Queue->m_fence->GetCompletedValue();

        if ( d3d12FenceValue <= semaphore )
        {
            // D3D has a new warning: ID3D12Fence1::SetEventOnCompletion: Fence values can never be less than zero, so waiting for a fence value of zero will always be satisfied
            // This happens on the first application frame and it is a correct behaviour to not wait on 0 fence value.
            if ( semaphore >= 1 )
            {
                pD3D12Queue->m_fence->SetEventOnCompletion( semaphore - 1, pD3D12Queue->m_fenceEvent );
                WaitForSingleObject( pD3D12Queue->m_fenceEvent, INFINITE );
            }
        }
    }

    EE_BASE_API void QueueDeviceWait( Queue* pQueueThatWaits, Queue* pQueueToWaitFor, uint64_t semaphore )
    {
        EE_ASSERT( pQueueThatWaits != pQueueToWaitFor );

        Direct3D12Queue* pD3D12QueueThatWaits = static_cast<Direct3D12Queue*>( pQueueThatWaits );
        Direct3D12Queue* pD3D12QueueToWaitFor = static_cast<Direct3D12Queue*>( pQueueToWaitFor );

        // D3D has a new warning: ID3D12Fence1::SetEventOnCompletion: Fence values can never be less than zero, so waiting for a fence value of zero will always be satisfied
        // This happens on the first application frame and it is a correct behaviour to not wait on 0 fence value.
        if ( pD3D12QueueToWaitFor->m_fenceValue >= 1 )
        {
            HRESULT const result = pD3D12QueueThatWaits->m_queue->Wait( pD3D12QueueToWaitFor->m_fence.Get(), pD3D12QueueToWaitFor->m_fenceValue - 1 );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API uint64_t QueueSubmit( Queue* pQueue, TArrayView<CommandBuffer*> commandBuffers )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );

        if ( !commandBuffers.empty() )
        {
            pD3D12Queue->m_submitCommandLists.reserve( commandBuffers.size() );
            pD3D12Queue->m_submitCommandLists.clear();

            for ( CommandBuffer* pCommandBuffer : commandBuffers )
            {
                Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
                pD3D12Queue->m_submitCommandLists.push_back( pD3D12CommandBuffer->m_commandList.Get() );
            }

            pD3D12Queue->m_queue->ExecuteCommandLists( UINT( pD3D12Queue->m_submitCommandLists.size() ), pD3D12Queue->m_submitCommandLists.data() );
        }

        uint64_t signalSemaphore = pD3D12Queue->m_fenceValue;

        HRESULT const result = pD3D12Queue->m_queue->Signal( pD3D12Queue->m_fence.Get(), pD3D12Queue->m_fenceValue++ );
        EE_ASSERT( SUCCEEDED( result ) );

        return signalSemaphore;
    }

    EE_BASE_API uint64_t QueuePresent( Queue* pQueue, Swapchain* pSwapchain, uint32_t imageIndex )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );
        Direct3D12Swapchain* pD3D12Swapchain = static_cast<Direct3D12Swapchain*>( pSwapchain );

        EE_ASSERT( imageIndex == pD3D12Swapchain->m_swapchain->GetCurrentBackBufferIndex() );

        HRESULT result = pD3D12Swapchain->m_swapchain->Present( pD3D12Swapchain->m_syncInterval, pD3D12Swapchain->m_presentFlags );
        EE_ASSERT( SUCCEEDED( result ) );

        uint64_t signalSemaphore = pD3D12Queue->m_fenceValue;

        result = pD3D12Queue->m_queue->Signal( pD3D12Queue->m_fence.Get(), pD3D12Queue->m_fenceValue++ );
        EE_ASSERT( SUCCEEDED( result ) );

        return signalSemaphore;
    }

    EE_BASE_API void WaitQueueIdle( Queue* pQueue )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );

        HRESULT result = pD3D12Queue->m_queue->Signal( pD3D12Queue->m_fence.Get(), pD3D12Queue->m_fenceValue );
        EE_ASSERT( SUCCEEDED( result ) );

        result = pD3D12Queue->m_fence->SetEventOnCompletion( pD3D12Queue->m_fenceValue, pD3D12Queue->m_fenceEvent );
        EE_ASSERT( SUCCEEDED( result ) );

        pD3D12Queue->m_fenceValue++;
        WaitForSingleObject( pD3D12Queue->m_fenceEvent, INFINITE );
    }

    EE_BASE_API Swapchain* CreateSwapchain( Context* pContext, SwapchainParameters const& parameters )
    {
        Direct3D12Context*   pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12Queue*     pD3D12PresentQueue = static_cast<Direct3D12Queue*>( parameters.m_presentQueues[0] );
        Direct3D12Swapchain* pD3D12Swapchain = pD3D12Context->CreateObject<Direct3D12Swapchain>();

        DXGI_SWAP_CHAIN_DESC1 dxgiSwapchainDesc1 = {};
        dxgiSwapchainDesc1.Width = parameters.m_width;
        dxgiSwapchainDesc1.Height = parameters.m_height;
        dxgiSwapchainDesc1.Format = DXGIFormat( parameters.m_colorFormat );
        dxgiSwapchainDesc1.Stereo = false;
        dxgiSwapchainDesc1.SampleDesc = { 1, 0 };
        dxgiSwapchainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        dxgiSwapchainDesc1.BufferCount = parameters.m_numImages;
        dxgiSwapchainDesc1.Scaling = DXGI_SCALING_STRETCH;
        dxgiSwapchainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        dxgiSwapchainDesc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        dxgiSwapchainDesc1.Flags = 0;

        BOOL allowTearing = FALSE;
        HRESULT result = pD3D12Context->m_factory->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof( allowTearing ) );
        EE_ASSERT( SUCCEEDED( result ) );

        if ( allowTearing )
        {
            dxgiSwapchainDesc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapchainFullscreenDesc = {};
        dxgiSwapchainFullscreenDesc.Windowed = TRUE;

        HWND hwnd = HWND( parameters.m_pNativeWindowHandle );

        ComPtr<IDXGISwapChain1> dxgiSwapchain1 = {};
        result = pD3D12Context->m_factory->CreateSwapChainForHwnd( pD3D12PresentQueue->m_queue.Get(), hwnd, &dxgiSwapchainDesc1, &dxgiSwapchainFullscreenDesc, nullptr, dxgiSwapchain1.ReleaseAndGetAddressOf() );
        EE_ASSERT( SUCCEEDED( result ) );

        result = pD3D12Context->m_factory->MakeWindowAssociation( hwnd, DXGI_MWA_NO_ALT_ENTER );
        EE_ASSERT( SUCCEEDED( result ) );

        result = dxgiSwapchain1->QueryInterface( IID_PPV_ARGS( pD3D12Swapchain->m_swapchain.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        if ( pD3D12Context->m_deviceMode == DeviceMode::Linked && parameters.m_presentQueues.size() > 1 )
        {
            TVector<IUnknown*> presentQueues{ Memory::Allocators::g_RHI };
            TVector<UINT>      creationMasks{ Memory::Allocators::g_RHI };

            presentQueues.resize( parameters.m_presentQueues.size() );
            creationMasks.resize( parameters.m_presentQueues.size() );

            for ( size_t presentQueueIndex = 0; presentQueueIndex < parameters.m_presentQueues.size(); ++presentQueueIndex )
            {
                Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( parameters.m_presentQueues[presentQueueIndex] );
                presentQueues[presentQueueIndex] = pD3D12Queue->m_queue.Get();
                creationMasks[presentQueueIndex] = ( 1 << pD3D12Queue->m_nodeIndex );
            }

            result = pD3D12Swapchain->m_swapchain->ResizeBuffers1( dxgiSwapchainDesc1.BufferCount, parameters.m_width, parameters.m_height, dxgiSwapchainDesc1.Format, dxgiSwapchainDesc1.Flags, creationMasks.data(), presentQueues.data() );
            EE_ASSERT( SUCCEEDED( result ) );
        }

        TextureParameters swapchainTextureParameters = {};
        swapchainTextureParameters.m_width = parameters.m_width;
        swapchainTextureParameters.m_height = parameters.m_height;
        swapchainTextureParameters.m_depth = 1;
        swapchainTextureParameters.m_arrayLayers = 1;
        swapchainTextureParameters.m_format = parameters.m_renderTargetFormat;
        swapchainTextureParameters.m_clearValue = parameters.m_clearValue;
        swapchainTextureParameters.m_numSamples = 1;
        swapchainTextureParameters.m_sampleQuality = 0;
        swapchainTextureParameters.m_textureFlags = TextureFlags::AllowDisplayTarget;
        swapchainTextureParameters.m_descriptorTypes = DescriptorTypeFlags::RenderTarget;
        swapchainTextureParameters.m_initialState = TextureState::Present;

        wchar_t swapchainBufferName[Limits::MaxDebugNameLength] = {};

        for ( UINT bufferIndex = 0; bufferIndex < parameters.m_numImages; ++bufferIndex )
        {
            ComPtr<ID3D12Resource> swapchainBuffer = {};
            result = pD3D12Swapchain->m_swapchain->GetBuffer( bufferIndex, IID_PPV_ARGS( swapchainBuffer.ReleaseAndGetAddressOf() ) );
            EE_ASSERT( SUCCEEDED( result ) );

            swprintf_s( swapchainBufferName, L"Swapchain Render Target %i", bufferIndex );
            result = swapchainBuffer->SetName( swapchainBufferName );
            EE_ASSERT( SUCCEEDED( result ) );

            swapchainTextureParameters.m_pNativeHandle = swapchainBuffer.Get();
            swapchainTextureParameters.m_debugName.sprintf( "Swapchain Render Target %i", bufferIndex );
            pD3D12Swapchain->m_renderTargets[bufferIndex] = CreateTexture( pContext, swapchainTextureParameters );
        }

        SetVSync( pD3D12Swapchain, parameters.m_enableVSync );

        return pD3D12Swapchain;
    }

    EE_BASE_API void DestroySwapchain( Context* pContext, Swapchain*&& pSwapchain )
    {
        if ( pSwapchain )
        {
            for ( size_t textureIndex = 0; textureIndex < pSwapchain->m_renderTargets.size(); ++textureIndex )
            {
                DestroyTexture( pContext, eastl::move( pSwapchain->m_renderTargets[textureIndex] ) );
            }

            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Swapchain* pD3D12Swapchain = static_cast<Direct3D12Swapchain*>( pSwapchain );

            pD3D12Context->DestroyObject( eastl::move( pD3D12Swapchain ) );
            pSwapchain = nullptr;
        }
    }

    EE_BASE_API uint32_t AcquireNextImage( Context* pContext, Swapchain* pSwapchain )
    {
        Direct3D12Swapchain* pD3D12Swapchain = static_cast<Direct3D12Swapchain*>( pSwapchain );
        return pD3D12Swapchain->m_swapchain->GetCurrentBackBufferIndex();
    }

    EE_BASE_API void SetVSync( Swapchain* pSwapchain, bool vsync )
    {
        Direct3D12Swapchain* pD3D12Swapchain = static_cast<Direct3D12Swapchain*>( pSwapchain );
        pD3D12Swapchain->m_vsync = vsync;
        if ( vsync )
        {
            pD3D12Swapchain->m_syncInterval = 1;
            pD3D12Swapchain->m_presentFlags &= ~DXGI_PRESENT_ALLOW_TEARING;
        }
        else
        {
            pD3D12Swapchain->m_syncInterval = 0;
            pD3D12Swapchain->m_presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }
    }

    EE_BASE_API CommandPool* CreateCommandPool( Context* pContext, CommandPoolParameters const& parameters )
    {
        Direct3D12Context*     pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12CommandPool* pD3D12CommandPool = pD3D12Context->CreateObject<Direct3D12CommandPool>();

        HRESULT const result = pD3D12Context->m_device->CreateCommandAllocator( D3D12CommandBufferType( parameters.m_pQueue->m_queueType ), IID_PPV_ARGS( pD3D12CommandPool->m_commandAllocator.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        pD3D12CommandPool->m_pQueue = parameters.m_pQueue;

        SetDebugName( pD3D12Context, pD3D12CommandPool, parameters.m_debugName );

        return pD3D12CommandPool;
    }

    EE_BASE_API void DestroyCommandPool( Context* pContext, CommandPool*&& pCommandPool )
    {
        if ( pCommandPool )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12CommandPool* pD3D12CommandPool = static_cast<Direct3D12CommandPool*>( pCommandPool );

            pD3D12Context->DestroyObject( eastl::move( pD3D12CommandPool ) );
            pCommandPool = nullptr;
        }
    }

    EE_BASE_API void ResetCommandPool( Context* pContext, CommandPool* pCommandPool )
    {
        Direct3D12CommandPool* pD3D12CommandPool = static_cast<Direct3D12CommandPool*>( pCommandPool );
        HRESULT const result = pD3D12CommandPool->m_commandAllocator->Reset();
        EE_ASSERT( SUCCEEDED( result ) );
    }

    EE_BASE_API CommandBuffer* CreateCommandBuffer( Context* pContext, CommandBufferParameters const& parameters )
    {
        Direct3D12Context*     pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12CommandPool* pD3D12CommandPool = static_cast<Direct3D12CommandPool*>( parameters.m_pCommandPool );

        uint32_t nodeIndex = parameters.m_pCommandPool->m_pQueue->m_nodeIndex;

        Direct3D12CommandBuffer* pD3D12CommandBuffer = pD3D12Context->CreateObject<Direct3D12CommandBuffer>();
        pD3D12CommandBuffer->m_pQueue = parameters.m_pCommandPool->m_pQueue;
        pD3D12CommandBuffer->m_pCommandPool = parameters.m_pCommandPool;
        pD3D12CommandBuffer->m_commandListType = D3D12CommandBufferType( parameters.m_pCommandPool->m_pQueue->m_queueType );

        pD3D12CommandBuffer->m_pDeviceResourceDescriptorAllocator = &pD3D12Context->m_deviceResourceDescriptorAllocators[nodeIndex];
        pD3D12CommandBuffer->m_pDeviceSamplerDescriptorAllocator = &pD3D12Context->m_deviceSamplerDescriptorAllocators[nodeIndex];

        pD3D12CommandBuffer->m_shadingRateCaps = pContext->m_deviceCapabilities.m_shadingRateCaps;

        pD3D12CommandBuffer->m_stage = Direct3D12CommandBuffer::Stage::Created;

        HRESULT result = pD3D12Context->m_device->CreateCommandList( pD3D12Context->NodeMask( nodeIndex ), pD3D12CommandBuffer->m_commandListType, pD3D12CommandPool->m_commandAllocator.Get(), nullptr, IID_PPV_ARGS( pD3D12CommandBuffer->m_commandList.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        // In D3D12 command lists are created in the recording state, which is not consistent with other APIs.
        // Close it immediately to be consistent.
        result = pD3D12CommandBuffer->m_commandList->Close();
        EE_ASSERT( SUCCEEDED( result ) );


        pD3D12CommandBuffer->m_stage = Direct3D12CommandBuffer::Stage::Closed;

        SetDebugName( pD3D12Context, pD3D12CommandBuffer, parameters.m_debugName );

        #ifdef USE_AGS
        if ( pD3D12Context->m_amdAgsEnabled )
        {
            pD3D12CommandBuffer->m_pAmdAgsContext = pD3D12Context->m_pAmdAgsContext;
        }
        #endif

        return pD3D12CommandBuffer;
    }

    EE_BASE_API void DestroyCommandBuffer( Context* pContext, CommandBuffer*&& pCommandBuffer )
    {
        if ( pCommandBuffer )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

            pD3D12Context->DestroyObject( eastl::move( pD3D12CommandBuffer ) );
            pCommandBuffer = nullptr;
        }
    }

    EE_BASE_API void BeginCommandBuffer( CommandBuffer* pCommandBuffer )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12CommandPool*   pD3D12CommandPool = static_cast<Direct3D12CommandPool*>( pD3D12CommandBuffer->m_pCommandPool );

        EE_ASSERT( pD3D12CommandBuffer->m_globalBarriers.empty() );
        EE_ASSERT( pD3D12CommandBuffer->m_bufferBarriers.empty() );
        EE_ASSERT( pD3D12CommandBuffer->m_textureBarriers.empty() );

        EE_ASSERT( pD3D12CommandBuffer->m_stage == Direct3D12CommandBuffer::Stage::Closed );

        HRESULT const result = pD3D12CommandBuffer->m_commandList->Reset( pD3D12CommandPool->m_commandAllocator.Get(), nullptr );
        EE_ASSERT( SUCCEEDED( result ) );

        if ( pD3D12CommandBuffer->m_commandListType != D3D12_COMMAND_LIST_TYPE_COPY )
        {
            ID3D12DescriptorHeap* pD3D12DescriptorHeaps[2] =
            {
                pD3D12CommandBuffer->m_pDeviceResourceDescriptorAllocator->m_heap.Get(),
                pD3D12CommandBuffer->m_pDeviceSamplerDescriptorAllocator->m_heap.Get(),
            };

            EE_ASSERT( pD3D12DescriptorHeaps[0] );
            EE_ASSERT( pD3D12DescriptorHeaps[1] );

            pD3D12CommandBuffer->m_commandList->SetDescriptorHeaps( 2, pD3D12DescriptorHeaps );
        }

        pD3D12CommandBuffer->m_stage = Direct3D12CommandBuffer::Stage::Recording;

        pD3D12CommandBuffer->m_pBoundRootSignature = nullptr;
        pD3D12CommandBuffer->m_pBoundPipeline = nullptr;
    }

    EE_BASE_API void EndCommandBuffer( CommandBuffer* pCommandBuffer )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        EE_ASSERT( pD3D12CommandBuffer->m_stage == Direct3D12CommandBuffer::Stage::Recording );

        pD3D12CommandBuffer->FlushBarriers();
        HRESULT const result = pD3D12CommandBuffer->m_commandList->Close();
        EE_ASSERT( SUCCEEDED( result ) );

        pD3D12CommandBuffer->m_stage = Direct3D12CommandBuffer::Stage::Closed;

        EE_ASSERT( pD3D12CommandBuffer->m_debugMarkerScopeCounter == 0 );
    }

    EE_BASE_API void CmdSetRenderTargets( CommandBuffer* pCommandBuffer, TArrayView<Texture* const> renderTargets, Texture* pDepthStencil, LoadAction* pLoadAction, TArrayView<uint32_t const> colorArraySlices, TArrayView<uint32_t const> colorMipSlices, uint32_t depthArraySlice, uint32_t depthMipSlice )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();

        D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor = {};
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptors[MaxRenderTargets] = {};

        for ( size_t renderTargetIndex = 0; renderTargetIndex < renderTargets.size(); ++renderTargetIndex )
        {
            Direct3D12Texture* pD3D12Texture = static_cast<Direct3D12Texture*>( renderTargets[renderTargetIndex] );
            if ( colorArraySlices.empty() && colorMipSlices.empty() )
            {
                renderTargetDescriptors[renderTargetIndex] = pD3D12Texture->RenderTargetDescriptorHandle( 0, 0 );
            }
            else
            {
                renderTargetDescriptors[renderTargetIndex] = pD3D12Texture->RenderTargetDescriptorHandle( colorArraySlices[renderTargetIndex], colorMipSlices[renderTargetIndex] );
            }
        }

        if ( pDepthStencil )
        {
            Direct3D12Texture* pD3D12Texture = static_cast<Direct3D12Texture*>( pDepthStencil );
            depthStencilDescriptor = pD3D12Texture->RenderTargetDescriptorHandle( depthArraySlice, depthMipSlice );
        }

        pD3D12CommandBuffer->m_commandList->OMSetRenderTargets( UINT( renderTargets.size() ),
                                                                renderTargetDescriptors,
                                                                FALSE,
                                                                pDepthStencil ? &depthStencilDescriptor : nullptr );

        if ( pLoadAction )
        {
            LoadAction const& loadAction = *pLoadAction;
            for ( size_t renderTargetIndex = 0; renderTargetIndex < renderTargets.size(); ++renderTargetIndex )
            {
                if ( loadAction.m_loadActionsColor[renderTargetIndex] == LoadActionType::Clear )
                {
                    float const clearColor[4] =
                    {
                        loadAction.m_colorClearValues[renderTargetIndex].m_red,
                        loadAction.m_colorClearValues[renderTargetIndex].m_green,
                        loadAction.m_colorClearValues[renderTargetIndex].m_blue,
                        loadAction.m_colorClearValues[renderTargetIndex].m_alpha,
                    };
                    pD3D12CommandBuffer->m_commandList->ClearRenderTargetView( renderTargetDescriptors[renderTargetIndex],
                                                                               clearColor,
                                                                               0, nullptr );
                }
            }

            if ( pDepthStencil )
            {
                UINT d3d12ClearFlags = 0;
                if ( loadAction.m_loadActionDepth == LoadActionType::Clear )
                {
                    d3d12ClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
                }
                if ( loadAction.m_loadActionStencil == LoadActionType::Clear )
                {
                    d3d12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
                }

                if ( d3d12ClearFlags )
                {
                    pD3D12CommandBuffer->m_commandList->ClearDepthStencilView( depthStencilDescriptor,
                                                                               D3D12_CLEAR_FLAGS( d3d12ClearFlags ),
                                                                               loadAction.m_depthClearValue.m_depth,
                                                                               loadAction.m_depthClearValue.m_stencil,
                                                                               0, nullptr );
                }
            }
        }
    }

    EE_BASE_API void CmdSetShadingRate( CommandBuffer* pCommandBuffer, ShadingRate shadingRate, Texture* pShadingRateTexture, ShadingRateCombiner postRasterizerCombiner, ShadingRateCombiner finalCombiner )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        ID3D12GraphicsCommandList5* pD3D12CommandList5 = static_cast<ID3D12GraphicsCommandList5*>( pD3D12CommandBuffer->m_commandList.Get() );

        if ( pD3D12CommandBuffer->m_shadingRateCaps.IsFlagSet( ShadingRateCaps::PerDraw ) )
        {
            D3D12_SHADING_RATE_COMBINER d3d12ShadingRateCombiners[2] =
            {
                D3D12ShadingRateCombiner( postRasterizerCombiner ),
                D3D12ShadingRateCombiner( finalCombiner ) };
            pD3D12CommandList5->RSSetShadingRate( D3D12ShadingRate( shadingRate ), d3d12ShadingRateCombiners );
        }

        if ( pShadingRateTexture && pD3D12CommandBuffer->m_shadingRateCaps.IsFlagSet( ShadingRateCaps::PerTile ) )
        {
            Direct3D12Texture* pD3D12Texture = static_cast<Direct3D12Texture*>( pShadingRateTexture );
            pD3D12CommandList5->RSSetShadingRateImage( pD3D12Texture->m_resource.Get() );
        }
    }

    EE_BASE_API void CmdSetViewport( CommandBuffer* pCommandBuffer, float x, float y, float width, float height, float minDepth, float maxDepth )
    {
        D3D12_VIEWPORT d3d12Viewport = {};
        d3d12Viewport.TopLeftX = x;
        d3d12Viewport.TopLeftY = y;
        d3d12Viewport.Width = width;
        d3d12Viewport.Height = height;
        d3d12Viewport.MinDepth = minDepth;
        d3d12Viewport.MaxDepth = maxDepth;

        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        pD3D12CommandBuffer->m_commandList->RSSetViewports( 1, &d3d12Viewport );
    }

    EE_BASE_API void CmdSetScissor( CommandBuffer* pCommandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height )
    {
        D3D12_RECT d3d12Rect = {};
        d3d12Rect.left = LONG( x );
        d3d12Rect.right = LONG( x ) + LONG( width );
        d3d12Rect.top = LONG( y );
        d3d12Rect.bottom = LONG( y ) + LONG( height );

        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        pD3D12CommandBuffer->m_commandList->RSSetScissorRects( 1, &d3d12Rect );
    }

    EE_BASE_API void CmdSetStencilReference( CommandBuffer* pCommandBuffer, uint32_t value )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        pD3D12CommandBuffer->m_commandList->OMSetStencilRef( value );
    }

    EE_BASE_API void CmdSetPipeline( CommandBuffer* pCommandBuffer, Pipeline* pPipeline )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Pipeline*      pD3D12Pipeline = static_cast<Direct3D12Pipeline*>( pPipeline );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( pD3D12Pipeline->m_pRootSignature );

        pD3D12CommandBuffer->ResetRootSignature( pD3D12Pipeline->m_pipelineType, pD3D12RootSignature );
        pD3D12CommandBuffer->ResetPipeline( pD3D12Pipeline );
    }

    EE_BASE_API void CmdSetRootConstants( CommandBuffer* pCommandBuffer, uint32_t constantIndex, void const* pConstantData, size_t constantSize )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Pipeline*      pD3D12Pipeline = static_cast<Direct3D12Pipeline*>( pD3D12CommandBuffer->m_pBoundPipeline );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( pD3D12CommandBuffer->m_pBoundRootSignature );

        DescriptorReflection const& descriptorReflection = pD3D12RootSignature->m_descriptorReflections[constantIndex];
        EE_ASSERT( descriptorReflection.m_descriptorTypeFlags == TBitFlags( DescriptorTypeFlags::RootConstant ) );

        EE_ASSERT( constantSize == sizeof( UINT ) * descriptorReflection.m_numConstants );

        if ( pConstantData )
        {
            if ( pD3D12Pipeline->m_pipelineType == PipelineType::Graphics )
            {
                pD3D12CommandBuffer->m_commandList->SetGraphicsRoot32BitConstants( constantIndex, descriptorReflection.m_numConstants, pConstantData, 0 );
            }
            else
            {
                pD3D12CommandBuffer->m_commandList->SetComputeRoot32BitConstants( constantIndex, descriptorReflection.m_numConstants, pConstantData, 0 );
            }
        }
    }

    EE_BASE_API void CmdSetRootParameter( CommandBuffer* pCommandBuffer, uint32_t parameterIndex, Buffer* pBuffer, size_t bufferOffset )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Pipeline*      pD3D12Pipeline = static_cast<Direct3D12Pipeline*>( pD3D12CommandBuffer->m_pBoundPipeline );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( pD3D12CommandBuffer->m_pBoundRootSignature );
        Direct3D12Buffer*        pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

        DescriptorReflection const& descriptorReflection = pD3D12RootSignature->m_descriptorReflections[parameterIndex];

        EE_ASSERT( bufferOffset < pD3D12Buffer->m_size );
        size_t deviceAddress = pD3D12Buffer->m_deviceAddress + bufferOffset;

        if ( pD3D12Pipeline->m_pipelineType == PipelineType::Graphics )
        {
            if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) )
            {
                EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) );
                pD3D12CommandBuffer->m_commandList->SetGraphicsRootConstantBufferView( descriptorReflection.m_parameterIndex, deviceAddress );
            }
            else
            {
                if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::Buffer ) )
                {
                    EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::Buffer ) );
                    pD3D12CommandBuffer->m_commandList->SetGraphicsRootShaderResourceView( descriptorReflection.m_parameterIndex, deviceAddress );
                }
                else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::RWBuffer ) )
                {
                    EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWBuffer ) );
                    pD3D12CommandBuffer->m_commandList->SetGraphicsRootUnorderedAccessView( descriptorReflection.m_parameterIndex, deviceAddress );
                }
                else
                {
                    EE_ASSERT( false );
                }
            }
        }
        else
        {
            if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) )
            {
                EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) );
                pD3D12CommandBuffer->m_commandList->SetComputeRootConstantBufferView( descriptorReflection.m_parameterIndex, deviceAddress );
            }
            else
            {
                if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::Buffer ) )
                {
                    EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::Buffer ) );
                    pD3D12CommandBuffer->m_commandList->SetComputeRootShaderResourceView( descriptorReflection.m_parameterIndex, deviceAddress );
                }
                else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::RWBuffer ) )
                {
                    EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWBuffer ) );
                    pD3D12CommandBuffer->m_commandList->SetComputeRootUnorderedAccessView( descriptorReflection.m_parameterIndex, deviceAddress );
                }
                else
                {
                    EE_ASSERT( false );
                }
            }
        }
    }

    EE_BASE_API void CmdSetIndexBuffer( CommandBuffer* pCommandBuffer, Buffer const* pIndexBuffer, IndexType indexType, uint64_t offset )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Buffer const*  pD3D12Buffer = static_cast<Direct3D12Buffer const*>( pIndexBuffer );

        D3D12_INDEX_BUFFER_VIEW d3d12IndexBufferView = {};
        d3d12IndexBufferView.BufferLocation = pD3D12Buffer->m_deviceAddress + offset;
        d3d12IndexBufferView.Format = D3D12IndexType( indexType );
        d3d12IndexBufferView.SizeInBytes = UINT( pD3D12Buffer->m_size - offset );

        pD3D12CommandBuffer->m_commandList->IASetIndexBuffer( &d3d12IndexBufferView );
    }

    EE_BASE_API void CmdDraw( CommandBuffer* pCommandBuffer, uint32_t numVertices, uint32_t firstVertex )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->DrawInstanced( numVertices, 1, firstVertex, 0 );
    }

    EE_BASE_API void CmdDrawInstanced( CommandBuffer* pCommandBuffer, uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->DrawInstanced( numVertices, numInstances, firstVertex, firstInstance );
    }

    EE_BASE_API void CmdDrawIndexed( CommandBuffer* pCommandBuffer, uint32_t numIndices, uint32_t firstIndex )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->DrawIndexedInstanced( numIndices, 1, firstIndex, 0, 0 );
    }

    EE_BASE_API void CmdDrawIndexedInstanced( CommandBuffer* pCommandBuffer, uint32_t numIndices, uint32_t numInstances, uint32_t firstIndex, uint32_t firstInstance )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->DrawIndexedInstanced( numIndices, numInstances, firstIndex, 0, firstInstance );
    }

    EE_BASE_API void CmdDispatchCompute( CommandBuffer* pCommandBuffer, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->Dispatch( numGroupsX, numGroupsY, numGroupsZ );
    }

    EE_BASE_API void CmdDispatchMesh( CommandBuffer* pCommandBuffer, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();

        ID3D12GraphicsCommandList6* pD3D12CommandList6 = static_cast<ID3D12GraphicsCommandList6*>( pD3D12CommandBuffer->m_commandList.Get() );
        pD3D12CommandList6->DispatchMesh( numGroupsX, numGroupsY, numGroupsZ );
    }

    EE_BASE_API void CmdDispatchRays( CommandBuffer* pCommandBuffer, RaytracingShaderTable* pShaderTable, AccelerationStructure* pAccelerationStructure, uint32_t width, uint32_t height )
    {
        Direct3D12CommandBuffer*         pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12RaytracingShaderTable* pD3D12RaytracingShaderTable = static_cast<Direct3D12RaytracingShaderTable*>( pShaderTable );
        Direct3D12Buffer*                pD3D12RaytracingShaderTableBuffer = static_cast<Direct3D12Buffer*>( pD3D12RaytracingShaderTable->m_buffer );

        D3D12_GPU_VIRTUAL_ADDRESS d3d12ShaderTableStartAddress = pD3D12RaytracingShaderTableBuffer->m_deviceAddress;

        D3D12_GPU_VIRTUAL_ADDRESS_RANGE d312RayGenShaderRecord = {};
        d312RayGenShaderRecord.SizeInBytes = pD3D12RaytracingShaderTable->m_maxEntrySize;
        d312RayGenShaderRecord.StartAddress = d3d12ShaderTableStartAddress;

        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE d3d12RayMissShaderTable = {};
        d3d12RayMissShaderTable.SizeInBytes = pD3D12RaytracingShaderTable->m_missRecordSize;
        d3d12RayMissShaderTable.StartAddress = d3d12ShaderTableStartAddress + pD3D12RaytracingShaderTable->m_maxEntrySize;
        d3d12RayMissShaderTable.StrideInBytes = pD3D12RaytracingShaderTable->m_maxEntrySize;

        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE d3d12HitGroupTable = {};
        d3d12HitGroupTable.SizeInBytes = pD3D12RaytracingShaderTable->m_hitGroupRecordSize;
        d3d12HitGroupTable.StartAddress = d3d12ShaderTableStartAddress + pD3D12RaytracingShaderTable->m_maxEntrySize + pD3D12RaytracingShaderTable->m_missRecordSize;
        d3d12HitGroupTable.StrideInBytes = pD3D12RaytracingShaderTable->m_maxEntrySize;

        D3D12_DISPATCH_RAYS_DESC d3d12DispatchDesc = {};
        d3d12DispatchDesc.Width = width;
        d3d12DispatchDesc.Height = height;
        d3d12DispatchDesc.Depth = 1;
        d3d12DispatchDesc.RayGenerationShaderRecord = d312RayGenShaderRecord;
        d3d12DispatchDesc.MissShaderTable = d3d12RayMissShaderTable;
        d3d12DispatchDesc.HitGroupTable = d3d12HitGroupTable;

        CmdSetPipeline( pCommandBuffer, pD3D12RaytracingShaderTable->m_pPipeline );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->DispatchRays( &d3d12DispatchDesc );
    }

    EE_BASE_API void CmdExecuteIndirect( CommandBuffer* pCommandBuffer, CommandSignature const* pCommandSignature, uint32_t maxNumCommands, Buffer const* pIndirectBuffer, uint64_t indirectBufferOffset, Buffer const* pCounterBuffer, uint64_t counterBufferOffset )
    {
        Direct3D12CommandBuffer*            pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12CommandSignature const*   pD3D12CommandSignature = static_cast<Direct3D12CommandSignature const*>( pCommandSignature );

        Direct3D12Buffer const*             pD3D12IndirectBuffer = static_cast<Direct3D12Buffer const*>( pIndirectBuffer );

        ID3D12Resource* pD3D12CounterBuffer = nullptr;
        if ( pCounterBuffer )
        {
            pD3D12CounterBuffer = static_cast<Direct3D12Buffer const*>( pCounterBuffer )->m_resource.Get();
        }

        bool strideValid = ( pD3D12IndirectBuffer->m_stride % IndirectCommandAlignment ) == 0;
        EE_ASSERT( strideValid );
        EE_ASSERT( ( pD3D12IndirectBuffer->m_stride == pD3D12CommandSignature->m_stride ) );

        pD3D12CommandBuffer->FlushBarriers();

        pD3D12CommandBuffer->m_commandList->ExecuteIndirect( pD3D12CommandSignature->m_pCommandSignature.Get(),
                                                             maxNumCommands,
                                                             pD3D12IndirectBuffer->m_resource.Get(),
                                                             indirectBufferOffset,
                                                             pD3D12CounterBuffer, counterBufferOffset );
    }

    EE_BASE_API void CmdClearTexture( CommandBuffer* pCommandBuffer, Texture const* pTexture, uint32_t value )
    {
        Direct3D12CommandBuffer*    pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Texture const*    pD3D12Texture = static_cast<Direct3D12Texture const*>( pTexture );

        EE_ASSERT( pD3D12Texture->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWTexture ) );

        pD3D12CommandBuffer->FlushBarriers();

        UINT d3d12ClearValues[4] = { value, value, value, value };

        for ( uint32_t mipLevel = 0; mipLevel < pD3D12Texture->m_mipLevels; ++mipLevel )
        {
            GenericResourceHandle const descriptorHandle = pD3D12Texture->ResourceDescriptorHandle( DescriptorTypeFlags::RWTexture, mipLevel );

            pD3D12CommandBuffer->m_commandList->ClearUnorderedAccessViewUint(
                pD3D12Texture->m_pDeviceResourceDescriptorAllocator->DescriptorDeviceHandle( descriptorHandle ),
                pD3D12Texture->m_pHostResourceDescriptorAllocator->DescriptorHostHandle( descriptorHandle ),
                pD3D12Texture->m_resource.Get(),
                d3d12ClearValues,
                0, nullptr );
        }
    }

    EE_BASE_API void CmdClearBuffer( CommandBuffer* pCommandBuffer, Buffer const* pBuffer, uint32_t value )
    {
        Direct3D12CommandBuffer*    pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Buffer const*     pD3D12Buffer = static_cast<Direct3D12Buffer const*>( pBuffer );

        pD3D12CommandBuffer->FlushBarriers();

        EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWBuffer ) );

        UINT d3d12ClearValues[4] = { value, value, value, value };

        pD3D12CommandBuffer->m_commandList->ClearUnorderedAccessViewUint(
            pD3D12Buffer->m_pDeviceResourceDescriptorAllocator->DescriptorDeviceHandle( pD3D12Buffer->m_deviceDescriptorHandles.m_offset + pD3D12Buffer->m_uavDescriptorOffset ),
            pD3D12Buffer->m_pHostResourceDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_hostDescriptorHandles.m_offset + pD3D12Buffer->m_uavDescriptorOffset ),
            pD3D12Buffer->m_resource.Get(),
            d3d12ClearValues,
            0, nullptr );
    }

    EE_BASE_API void CmdBuildAccelerationStructure( CommandBuffer* pCommandBuffer, TArrayView<AccelerationStructure* const> accelerationStructures, TArrayView<uint32_t const> bottomLevelAccelerationStructureIndices )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        pD3D12CommandBuffer->FlushBarriers();

        //TInlineVector<D3D12_RESOURCE_BARRIER, 2> d3d12Barriers;

        for ( size_t accelerationStructureIndex = 0; accelerationStructureIndex < bottomLevelAccelerationStructureIndices.size(); ++accelerationStructureIndex )
        {
            Direct3D12AccelerationStructure* pD3D12AccelerationStructure = static_cast<Direct3D12AccelerationStructure*>( accelerationStructures[bottomLevelAccelerationStructureIndices[accelerationStructureIndex]] );
            Direct3D12Buffer*                pD3D12ScratchBuffer = static_cast<Direct3D12Buffer*>( pD3D12AccelerationStructure->m_scratchBuffer );
            Direct3D12Buffer*                pD3D12StructureBufferBottomLevel = static_cast<Direct3D12Buffer*>( pD3D12AccelerationStructure->m_bottomLevel.m_structureBuffer );

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3d12AccelerationStructureDesc = {};
            d3d12AccelerationStructureDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            d3d12AccelerationStructureDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            d3d12AccelerationStructureDesc.Inputs.Flags = pD3D12AccelerationStructure->m_flags;
            d3d12AccelerationStructureDesc.Inputs.NumDescs = UINT( pD3D12AccelerationStructure->m_bottomLevel.m_geometries.size() );
            d3d12AccelerationStructureDesc.Inputs.pGeometryDescs = pD3D12AccelerationStructure->m_bottomLevel.m_geometries.data();
            d3d12AccelerationStructureDesc.DestAccelerationStructureData = pD3D12StructureBufferBottomLevel->m_deviceAddress;
            d3d12AccelerationStructureDesc.ScratchAccelerationStructureData = pD3D12ScratchBuffer->m_deviceAddress;

            pD3D12CommandBuffer->m_commandList->BuildRaytracingAccelerationStructure( &d3d12AccelerationStructureDesc, 0, nullptr );

            //D3D12_RESOURCE_BARRIER d3d12Barrier = {};
            //d3d12Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            //d3d12Barrier.UAV.pResource = pD3D12StructureBufferBottomLevel->m_resource.Get();
            //d3d12Barriers.push_back( d3d12Barrier );
        }

        for ( auto const& accelerationStructure : accelerationStructures )
        {
            Direct3D12AccelerationStructure* pD3D12AccelerationStructure = static_cast<Direct3D12AccelerationStructure*>( accelerationStructure );
            Direct3D12Buffer*                pD3D12ScratchBuffer = static_cast<Direct3D12Buffer*>( pD3D12AccelerationStructure->m_scratchBuffer );
            Direct3D12Buffer*                pD3D12StructureBufferTopLevel = static_cast<Direct3D12Buffer*>( pD3D12AccelerationStructure->m_structureBuffer );
            Direct3D12Buffer*                pD3D12InstanceBufferTopLevel = static_cast<Direct3D12Buffer*>( pD3D12AccelerationStructure->m_instanceBuffer );

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3d12AccelerationStructureDesc = {};
            d3d12AccelerationStructureDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            d3d12AccelerationStructureDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            d3d12AccelerationStructureDesc.Inputs.Flags = pD3D12AccelerationStructure->m_flags;
            d3d12AccelerationStructureDesc.Inputs.NumDescs = UINT( pD3D12AccelerationStructure->m_numInstances );
            d3d12AccelerationStructureDesc.Inputs.InstanceDescs = pD3D12InstanceBufferTopLevel->m_deviceAddress;
            d3d12AccelerationStructureDesc.DestAccelerationStructureData = pD3D12StructureBufferTopLevel->m_deviceAddress;
            d3d12AccelerationStructureDesc.ScratchAccelerationStructureData = pD3D12ScratchBuffer->m_deviceAddress;

            pD3D12CommandBuffer->m_commandList->BuildRaytracingAccelerationStructure( &d3d12AccelerationStructureDesc, 0, nullptr );

            //D3D12_RESOURCE_BARRIER d3d12Barrier = {};
            //d3d12Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            //d3d12Barrier.UAV.pResource = pD3D12StructureBufferTopLevel->m_resource.Get();
            //d3d12Barriers.push_back( d3d12Barrier );
        }
    }

    EE_BASE_API void CmdBarrier( CommandBuffer* pCommandBuffer, TBitFlags<PipelineStage> sourceSync, TBitFlags<PipelineStage> destinationSync, TBitFlags<ResourceAccess> sourceAccess, TBitFlags<ResourceAccess> destinationAccess )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        // TODO: Is this a good idea to clear these here?
        if ( pD3D12CommandBuffer->m_commandListType != D3D12_COMMAND_LIST_TYPE_DIRECT )
        {
            sourceSync.ClearFlags( PipelineStageFlags_GraphicsQueueOnly );
            EE_ASSERT( ( destinationSync & PipelineStageFlags_GraphicsQueueOnly ) == 0 );
        }

        D3D12_GLOBAL_BARRIER d3d12GlobalBarrier = {};
        d3d12GlobalBarrier.SyncBefore = D3D12BarrierSync( sourceSync );
        d3d12GlobalBarrier.SyncAfter = D3D12BarrierSync( destinationSync );
        d3d12GlobalBarrier.AccessBefore = D3D12BarrierAccess( sourceAccess );
        d3d12GlobalBarrier.AccessAfter = D3D12BarrierAccess( destinationAccess );

        pD3D12CommandBuffer->m_globalBarriers.emplace_back( eastl::move( d3d12GlobalBarrier ) );
    }

    EE_BASE_API void CmdBarrier( CommandBuffer* pCommandBuffer, Buffer* pBuffer, TBitFlags<PipelineStage> sourceSync, TBitFlags<PipelineStage> destinationSync, TBitFlags<ResourceAccess> sourceAccess, TBitFlags<ResourceAccess> destinationAccess )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

        // TODO: Is this a good idea to clear these here?
        if ( pD3D12CommandBuffer->m_commandListType != D3D12_COMMAND_LIST_TYPE_DIRECT )
        {
            sourceSync.ClearFlags( PipelineStageFlags_GraphicsQueueOnly );
            EE_ASSERT( ( destinationSync & PipelineStageFlags_GraphicsQueueOnly ) == 0 );
        }

        D3D12_BUFFER_BARRIER d3d12BufferBarrier = {};
        d3d12BufferBarrier.pResource = pD3D12Buffer->m_resource.Get();
        d3d12BufferBarrier.Offset = 0;
        d3d12BufferBarrier.Size = pD3D12Buffer->m_size;
        d3d12BufferBarrier.SyncBefore = D3D12BarrierSync( sourceSync );
        d3d12BufferBarrier.SyncAfter = D3D12BarrierSync( destinationSync );
        d3d12BufferBarrier.AccessBefore = D3D12BarrierAccess( sourceAccess );
        d3d12BufferBarrier.AccessAfter = D3D12BarrierAccess( destinationAccess );

        pD3D12CommandBuffer->m_bufferBarriers.emplace_back( eastl::move( d3d12BufferBarrier ) );
    }

    EE_BASE_API void CmdBarrier( CommandBuffer* pCommandBuffer, Texture* pTexture, TBitFlags<PipelineStage> sourceSync, TBitFlags<PipelineStage> destinationSync, TBitFlags<ResourceAccess> sourceAccess, TBitFlags<ResourceAccess> destinationAccess, TextureState sourceState, TextureState destinationState, TextureBarrierRegion region, TBitFlags<TextureBarrierFlags> flags )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Texture* pD3D12Texture = static_cast<Direct3D12Texture*>( pTexture );

        // TODO: Is this a good idea to clear these here?
        if ( pD3D12CommandBuffer->m_commandListType != D3D12_COMMAND_LIST_TYPE_DIRECT )
        {
            sourceSync.ClearFlags( PipelineStageFlags_GraphicsQueueOnly );
            EE_ASSERT( ( destinationSync & PipelineStageFlags_GraphicsQueueOnly ) == 0 );
        }

        D3D12_TEXTURE_BARRIER d3d12TextureBarrier = {};
        d3d12TextureBarrier.pResource = pD3D12Texture->m_resource.Get();
        d3d12TextureBarrier.Subresources.IndexOrFirstMipLevel = region.m_mipLevel;
        d3d12TextureBarrier.Subresources.NumMipLevels = region.m_numMipLevels ? region.m_numMipLevels : pD3D12Texture->m_mipLevels;
        d3d12TextureBarrier.Subresources.FirstArraySlice = region.m_arraySlice;
        d3d12TextureBarrier.Subresources.NumArraySlices = region.m_numArraySlices ? region.m_numArraySlices : pD3D12Texture->m_arrayLayers;
        d3d12TextureBarrier.Subresources.FirstPlane = 0;
        d3d12TextureBarrier.Subresources.NumPlanes = 1; // Why 1???
        d3d12TextureBarrier.SyncBefore = D3D12BarrierSync( sourceSync );
        d3d12TextureBarrier.SyncAfter = D3D12BarrierSync( destinationSync );
        d3d12TextureBarrier.AccessBefore = D3D12BarrierAccess( sourceAccess );
        d3d12TextureBarrier.AccessAfter = D3D12BarrierAccess( destinationAccess );
        d3d12TextureBarrier.LayoutBefore = D3D12BarrierLayout( sourceState );
        d3d12TextureBarrier.LayoutAfter = D3D12BarrierLayout( destinationState );
        d3d12TextureBarrier.Flags = D3D12TextureBarrierFlags( flags );

        pD3D12CommandBuffer->m_textureBarriers.emplace_back( eastl::move( d3d12TextureBarrier ) );
    }

    EE_BASE_API void CmdResetQueryPool( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, uint32_t startQuery, uint32_t numQueries )
    {
        // Nothing on D3D12
    }

    EE_BASE_API void CmdBeginQuery( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, uint32_t queryIndex )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12QueryPool*     pD3D12QueryPool = static_cast<Direct3D12QueryPool*>( pQueryPool );

        switch ( pD3D12QueryPool->m_queryType )
        {
            case D3D12_QUERY_TYPE_TIMESTAMP:
            {
                pD3D12CommandBuffer->m_commandList->BeginQuery( pD3D12QueryPool->m_queryHeap.Get(), pD3D12QueryPool->m_queryType, queryIndex );
            }
            break;

            default: break;
        }
    }

    EE_BASE_API void CmdEndQuery( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, uint32_t queryIndex )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12QueryPool*     pD3D12QueryPool = static_cast<Direct3D12QueryPool*>( pQueryPool );

        switch ( pD3D12QueryPool->m_queryType )
        {
            case D3D12_QUERY_TYPE_TIMESTAMP:
            {
                pD3D12CommandBuffer->m_commandList->EndQuery( pD3D12QueryPool->m_queryHeap.Get(), pD3D12QueryPool->m_queryType, queryIndex );
            }
            break;

            default: break;
        }
    }

    EE_BASE_API void CmdResolveQuery( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, Buffer* pReadbackBuffer, uint32_t startQuery, uint32_t numQueries )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12QueryPool*     pD3D12QueryPool = static_cast<Direct3D12QueryPool*>( pQueryPool );
        Direct3D12Buffer*        pD3D12ReadbackBuffer = static_cast<Direct3D12Buffer*>( pReadbackBuffer );

        pD3D12CommandBuffer->m_commandList->ResolveQueryData( pD3D12QueryPool->m_queryHeap.Get(),
                                                              pD3D12QueryPool->m_queryType,
                                                              startQuery,
                                                              numQueries,
                                                              pD3D12ReadbackBuffer->m_resource.Get(),
                                                              static_cast<UINT64>( startQuery ) * 8 );
    }

    EE_BASE_API void CmdCopyBuffer( CommandBuffer* pCommandBuffer, Buffer const* pDstBuffer, uint64_t dstOffset, Buffer const* pSrcBuffer, uint64_t srcOffset, uint64_t srcSize )
    {
        Direct3D12CommandBuffer*    pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Buffer const*     pD3D12DstBuffer = static_cast<Direct3D12Buffer const*>( pDstBuffer );
        Direct3D12Buffer const*     pD3D12SrcBuffer = static_cast<Direct3D12Buffer const*>( pSrcBuffer );

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->CopyBufferRegion( pD3D12DstBuffer->m_resource.Get(), dstOffset,
                                                              pD3D12SrcBuffer->m_resource.Get(), srcOffset,
                                                              srcSize );
    }

    EE_BASE_API void CmdCopyTexture( CommandBuffer* pCommandBuffer, Texture const* pDstTexture, TextureCopyRegion const& dstRegion, Buffer const* pSrcBuffer, uint64_t srcOffset )
    {
        Direct3D12CommandBuffer*    pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Texture const*    pD3D12DstTexture = static_cast<Direct3D12Texture const*>( pDstTexture );
        Direct3D12Buffer const*     pD3D12SrcBuffer = static_cast<Direct3D12Buffer const*>( pSrcBuffer );

        uint32_t dstSubresourceIndex = SubresourceIndex( dstRegion.m_mipLevel,
                                                         dstRegion.m_arrayLayer,
                                                         0,
                                                         pD3D12DstTexture->m_mipLevels,
                                                         pD3D12DstTexture->m_arrayLayers );

        D3D12_TEXTURE_COPY_LOCATION d3d12SrcLocation = {};
        d3d12SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        d3d12SrcLocation.pResource = pD3D12SrcBuffer->m_resource.Get();
        d3d12SrcLocation.PlacedFootprint.Footprint = pD3D12DstTexture->m_footprints[dstSubresourceIndex];
        d3d12SrcLocation.PlacedFootprint.Offset = srcOffset;

        D3D12_TEXTURE_COPY_LOCATION d3d12DstLocation = {};
        d3d12DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        d3d12DstLocation.pResource = pD3D12DstTexture->m_resource.Get();
        d3d12DstLocation.SubresourceIndex = dstSubresourceIndex;

        D3D12_BOX d3d12SrcBox = {};
        d3d12SrcBox.left = 0;
        d3d12SrcBox.right = dstRegion.m_width;
        d3d12SrcBox.top = 0;
        d3d12SrcBox.bottom = dstRegion.m_height;
        d3d12SrcBox.front = 0;
        d3d12SrcBox.back = dstRegion.m_depth;

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->CopyTextureRegion( &d3d12DstLocation,
                                                               dstRegion.m_x,
                                                               dstRegion.m_y,
                                                               dstRegion.m_z,
                                                               &d3d12SrcLocation,
                                                               &d3d12SrcBox );
    }

    EE_BASE_API void CmdCopyTexture( CommandBuffer* pCommandBuffer, Buffer const* pDstBuffer, uint64_t dstOffset, Texture const* pSrcTexture, TextureCopyRegion const& srcRegion )
    {
        Direct3D12CommandBuffer*    pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Texture const*    pD3D12SrcTexture = static_cast<Direct3D12Texture const*>( pSrcTexture );
        Direct3D12Buffer const*     pD3D12DstBuffer = static_cast<Direct3D12Buffer const*>( pDstBuffer );

        uint32_t srcSubresourceIndex = SubresourceIndex( srcRegion.m_mipLevel,
                                                         srcRegion.m_arrayLayer,
                                                         0,
                                                         pD3D12SrcTexture->m_mipLevels,
                                                         pD3D12SrcTexture->m_arrayLayers );

        D3D12_TEXTURE_COPY_LOCATION d3d12SrcLocation = {};
        d3d12SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        d3d12SrcLocation.pResource = pD3D12SrcTexture->m_resource.Get();
        d3d12SrcLocation.SubresourceIndex = srcSubresourceIndex;

        D3D12_TEXTURE_COPY_LOCATION d3d12DstLocation = {};
        d3d12DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        d3d12DstLocation.pResource = pD3D12DstBuffer->m_resource.Get();
        d3d12DstLocation.PlacedFootprint.Footprint = pD3D12DstBuffer->m_footprint;
        d3d12DstLocation.PlacedFootprint.Offset = dstOffset;

        D3D12_BOX d3d12SrcBox = {};
        d3d12SrcBox.left = srcRegion.m_x;
        d3d12SrcBox.right = srcRegion.m_x + srcRegion.m_width;
        d3d12SrcBox.top = srcRegion.m_y;
        d3d12SrcBox.bottom = srcRegion.m_y + srcRegion.m_height;
        d3d12SrcBox.front = srcRegion.m_z;
        d3d12SrcBox.back = srcRegion.m_z + srcRegion.m_depth;

        pD3D12CommandBuffer->FlushBarriers();
        pD3D12CommandBuffer->m_commandList->CopyTextureRegion( &d3d12DstLocation,
                                                               0,
                                                               0,
                                                               0,
                                                               &d3d12SrcLocation,
                                                               &d3d12SrcBox );
    }

    EE_BASE_API void CmdBeginDebugMarker( CommandBuffer* pCommandBuffer, char const* pName )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        // stolen from ImGui::ColorConvertHSVtoRGB
        auto HSVtoRGB = [] ( float h, float s, float v, float& out_r, float& out_g, float& out_b )
        {
            if ( s == 0.0f )
            {
                // gray
                out_r = out_g = out_b = v;
                return;
            }

            h = Math::FModF( h, 1.0f ) / ( 60.0f / 360.0f );
            int   i = (int) h;
            float f = h - (float) i;
            float p = v * ( 1.0f - s );
            float q = v * ( 1.0f - s * f );
            float t = v * ( 1.0f - s * ( 1.0f - f ) );

            switch ( i )
            {
                case 0: out_r = v; out_g = t; out_b = p; break;
                case 1: out_r = q; out_g = v; out_b = p; break;
                case 2: out_r = p; out_g = v; out_b = t; break;
                case 3: out_r = p; out_g = q; out_b = v; break;
                case 4: out_r = t; out_g = p; out_b = v; break;
                case 5: default: out_r = v; out_g = p; out_b = q; break;
            }
        };

        float h = pD3D12CommandBuffer->m_currentDebugMarkerColorValue;
        float s = 0.5F;
        float v = 0.95F;

        float r = 0.0F;
        float g = 0.0F;
        float b = 0.0F;
        HSVtoRGB( h, s, v, r, g, b );

        // https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
        // Random colors that are consistent and distinct from each other
        pD3D12CommandBuffer->m_currentDebugMarkerColorValue += 0.618033988749895F;
        pD3D12CommandBuffer->m_currentDebugMarkerColorValue = Math::FModF( pD3D12CommandBuffer->m_currentDebugMarkerColorValue, 1.0F );

        PIXBeginEvent( pD3D12CommandBuffer->m_commandList.Get(), PIX_COLOR( uint8_t( r * 255.0F ), uint8_t( g * 255.0F ), uint8_t( b * 255.0F ) ), pName );

        #ifdef USE_AGS
        if ( pD3D12CommandBuffer->m_pAmdAgsContext )
        {
            agsDriverExtensionsDX12_PushMarker( pD3D12CommandBuffer->m_pAmdAgsContext, pD3D12CommandBuffer->m_commandList.Get(), pName );
        }
        #endif

        pD3D12CommandBuffer->m_debugMarkerScopeCounter++;
    }

    EE_BASE_API void CmdEndDebugMarker( CommandBuffer* pCommandBuffer )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

        PIXEndEvent( pD3D12CommandBuffer->m_commandList.Get() );

        #ifdef USE_AGS
        if ( pD3D12CommandBuffer->m_pAmdAgsContext )
        {
            agsDriverExtensionsDX12_PopMarker( pD3D12CommandBuffer->m_pAmdAgsContext, pD3D12CommandBuffer->m_commandList.Get() );
        }
        #endif

        pD3D12CommandBuffer->m_debugMarkerScopeCounter--;
        EE_ASSERT( pD3D12CommandBuffer->m_debugMarkerScopeCounter >= 0 );
    }

    EE_BASE_API uint32_t CmdWriteDebugMarker( CommandBuffer* pCommandBuffer, TBitFlags<MarkerTypeFlags> const& markerType, uint32_t const markerValue, Buffer* pBuffer, size_t const offset, bool const useAutoFlags )
    {
        Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );
        Direct3D12Buffer*        pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

        D3D12_GPU_VIRTUAL_ADDRESS d3d12DeviceAddress = pD3D12Buffer->m_deviceAddress + offset * sizeof( uint32_t );

        uint32_t numMarkers = 1;
        if ( markerType == TBitFlags( MarkerTypeFlags::InOut ) ) { numMarkers = 2; }

        D3D12_WRITEBUFFERIMMEDIATE_MODE      d3d12WriteBufferMode[2] = {};
        D3D12_WRITEBUFFERIMMEDIATE_PARAMETER d3d12WriteBufferParameter[2] = {};

        if ( numMarkers > 1 )
        {
            d3d12WriteBufferParameter[0].Dest = d3d12WriteBufferParameter[1].Dest = d3d12DeviceAddress;
            d3d12WriteBufferParameter[0].Value = markerValue | ( useAutoFlags ? ( UINT( MarkerTypeFlags::In ) << 30 ) : 0 );
            d3d12WriteBufferParameter[1].Value = markerValue | ( useAutoFlags ? ( UINT( MarkerTypeFlags::Out ) << 30 ) : 0 );

            d3d12WriteBufferMode[0] = D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_IN;
            d3d12WriteBufferMode[1] = D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
        }
        else
        {
            d3d12WriteBufferParameter[0].Dest = d3d12DeviceAddress;
            d3d12WriteBufferParameter[0].Value = markerValue | ( useAutoFlags ? ( markerType << 30 ) : 0 );

            d3d12WriteBufferMode[0] = D3D12_WRITEBUFFERIMMEDIATE_MODE( markerType.Get() );
        }

        ID3D12GraphicsCommandList2* pD3D12CommandList2 = static_cast<ID3D12GraphicsCommandList2*>( pD3D12CommandBuffer->m_commandList.Get() );
        pD3D12CommandList2->WriteBufferImmediate( numMarkers, d3d12WriteBufferParameter, d3d12WriteBufferMode );

        return markerValue;
    }

    EE_BASE_API CommandSignature* CreateCommandSignature( Context* pContext, CommandSignatureParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pRootSignature );

        bool                 needRootSignature = false;
        uint32_t             commandStride = 0;
        IndirectArgumentType commandType = IndirectArgumentType::Invalid;

        TVector<D3D12_INDIRECT_ARGUMENT_DESC> d3d12Arguments{ Memory::Allocators::g_RHI };
        d3d12Arguments.reserve( parameters.m_indirectArgumentParameters.size() );

        for ( IndirectArgumentDescriptor const& argument : parameters.m_indirectArgumentParameters )
        {
            DescriptorReflection const* pDescriptorReflection = nullptr;
            if ( argument.m_type != IndirectArgumentType::Draw &&
                 argument.m_type != IndirectArgumentType::DrawIndexed &&
                 argument.m_type != IndirectArgumentType::DispatchCompute &&
                 argument.m_type != IndirectArgumentType::DispatchMesh &&
                 argument.m_type != IndirectArgumentType::DispatchRays )
            {
                EE_ASSERT( argument.m_index < parameters.m_pRootSignature->m_descriptorReflections.size() );
                pDescriptorReflection = &parameters.m_pRootSignature->m_descriptorReflections[argument.m_index];
            }

            D3D12_INDIRECT_ARGUMENT_DESC d3d12ArgumentDesc = {};
            switch ( argument.m_type )
            {
                case IndirectArgumentType::Draw:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
                    commandStride += sizeof( IndirectDrawArguments );
                    commandType = argument.m_type;
                }
                break;

                case IndirectArgumentType::DrawIndexed:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
                    commandStride += sizeof( IndirectDrawIndexedArguments );
                    commandType = argument.m_type;
                }
                break;

                case IndirectArgumentType::DispatchCompute:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
                    commandStride += sizeof( IndirectDispatchArguments );
                    commandType = argument.m_type;
                }
                break;

                case IndirectArgumentType::DispatchMesh:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
                    commandStride += sizeof( IndirectDispatchArguments );
                    commandType = argument.m_type;
                }
                break;

                case IndirectArgumentType::DispatchRays:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
                    commandStride += sizeof( IndirectDispatchArguments );
                    commandType = argument.m_type;
                }
                break;

                case IndirectArgumentType::VertexBuffer:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
                    d3d12ArgumentDesc.VertexBuffer.Slot = pDescriptorReflection->m_parameterIndex;
                    commandStride += sizeof( D3D12_VERTEX_BUFFER_VIEW );
                    needRootSignature = true;
                }
                break;

                case IndirectArgumentType::IndexBuffer:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
                    // Not a typo, D3D12 uses VertexBuffer union for index buffers
                    d3d12ArgumentDesc.VertexBuffer.Slot = pDescriptorReflection->m_parameterIndex;
                    commandStride += sizeof( D3D12_VERTEX_BUFFER_VIEW );
                    needRootSignature = true;
                }
                break;

                case IndirectArgumentType::Constant:
                {
                    EE_ASSERT( argument.m_byteSize == sizeof( UINT ) * pDescriptorReflection->m_numConstants );

                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
                    d3d12ArgumentDesc.Constant.RootParameterIndex = pDescriptorReflection->m_parameterIndex;
                    d3d12ArgumentDesc.Constant.DestOffsetIn32BitValues = 0;
                    d3d12ArgumentDesc.Constant.Num32BitValuesToSet = pDescriptorReflection->m_numConstants;
                    commandStride += sizeof( UINT ) * pDescriptorReflection->m_numConstants;
                    needRootSignature = true;
                }
                break;

                case IndirectArgumentType::ConstantBufferView:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
                    d3d12ArgumentDesc.ConstantBufferView.RootParameterIndex = pDescriptorReflection->m_parameterIndex;
                    commandStride += sizeof( D3D12_GPU_VIRTUAL_ADDRESS );
                    needRootSignature = true;
                }
                break;

                case IndirectArgumentType::ShaderResourceView:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
                    d3d12ArgumentDesc.ShaderResourceView.RootParameterIndex = pDescriptorReflection->m_parameterIndex;
                    commandStride += sizeof( D3D12_GPU_VIRTUAL_ADDRESS );
                    needRootSignature = true;
                }
                break;

                case IndirectArgumentType::UnorderedAccessView:
                {
                    d3d12ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
                    d3d12ArgumentDesc.ConstantBufferView.RootParameterIndex = pDescriptorReflection->m_parameterIndex;
                    commandStride += sizeof( D3D12_GPU_VIRTUAL_ADDRESS );
                    needRootSignature = true;
                }
                break;

                case IndirectArgumentType::Invalid:
                default:
                {
                    EE_ASSERT( false );
                }
            }

            d3d12Arguments.push_back( d3d12ArgumentDesc );
        }

        EE_ASSERT( !needRootSignature || ( needRootSignature && pD3D12RootSignature ) );

        Direct3D12CommandSignature* pD3D12CommandSignature = pD3D12Context->CreateObject<Direct3D12CommandSignature>();

        D3D12_COMMAND_SIGNATURE_DESC d3d12CommandSignatureDesc = {};
        d3d12CommandSignatureDesc.pArgumentDescs = d3d12Arguments.data();
        d3d12CommandSignatureDesc.NumArgumentDescs = UINT( parameters.m_indirectArgumentParameters.size() );
        d3d12CommandSignatureDesc.ByteStride = Math::RoundUpToNearestMultiple32( commandStride, 16 );
        d3d12CommandSignatureDesc.NodeMask = pD3D12Context->SharedNodeMask();

        HRESULT const result = pD3D12Context->m_device->CreateCommandSignature( &d3d12CommandSignatureDesc, needRootSignature ? pD3D12RootSignature->m_rootSignature.Get() : nullptr, IID_PPV_ARGS( pD3D12CommandSignature->m_pCommandSignature.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12CommandSignature, parameters.m_debugName );

        pD3D12CommandSignature->m_argumentType = commandType;
        pD3D12CommandSignature->m_stride = d3d12CommandSignatureDesc.ByteStride;

        return pD3D12CommandSignature;
    }

    EE_BASE_API void DestroyCommandSignature( Context* pContext, CommandSignature*&& pCommandSignature )
    {
        if ( pCommandSignature )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12CommandSignature* pD3D12CommandSignature = static_cast<Direct3D12CommandSignature*>( pCommandSignature );

            pD3D12Context->DestroyObject( eastl::move( pD3D12CommandSignature ) );
            pCommandSignature = nullptr;
        }
    }

    EE_BASE_API AccelerationStructure* CreateAccelerationStructure( Context* pContext, AccelerationStructureTopLevelCreateParameters const& topLevelParameters, AccelerationStructureBottomLevelCreateParameters const& bottomLevelParameters )
    {
        Direct3D12Context*               pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12AccelerationStructure* pD3D12AccelerationStructure = pD3D12Context->CreateObject<Direct3D12AccelerationStructure>();

        ID3D12Device5* pD3D12Device5 = static_cast<ID3D12Device5*>( pD3D12Context->m_device.Get() );

        pD3D12AccelerationStructure->m_bottomLevel.m_flags = D3D12AccelerationStructureBuildFlags( bottomLevelParameters.m_flags );
        pD3D12AccelerationStructure->m_flags = D3D12AccelerationStructureBuildFlags( topLevelParameters.m_flags );

        pD3D12AccelerationStructure->m_bottomLevel.m_geometries.reserve( bottomLevelParameters.m_geometries.size() );

        for ( AccelerationStructureGeometry const& geometry : bottomLevelParameters.m_geometries )
        {
            Direct3D12Buffer* pD3D12IndexBuffer = static_cast<Direct3D12Buffer*>( geometry.m_pIndexBuffer );
            Direct3D12Buffer* pD3D12VertexBuffer = static_cast<Direct3D12Buffer*>( geometry.m_pVertexBuffer );

            D3D12_RAYTRACING_GEOMETRY_DESC d3d12GeometryDesc = {};
            d3d12GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            d3d12GeometryDesc.Flags = D3D12GeometryFlags( geometry.m_flags );

            d3d12GeometryDesc.Triangles.IndexBuffer = pD3D12IndexBuffer->m_deviceAddress + geometry.m_indexOffset;
            d3d12GeometryDesc.Triangles.IndexCount = geometry.m_numIndices;
            d3d12GeometryDesc.Triangles.IndexFormat = D3D12IndexType( geometry.m_indexType );

            d3d12GeometryDesc.Triangles.VertexBuffer = { pD3D12VertexBuffer->m_deviceAddress + geometry.m_vertexOffset, pD3D12VertexBuffer->m_stride };
            d3d12GeometryDesc.Triangles.VertexCount = geometry.m_numVertices;
            d3d12GeometryDesc.Triangles.VertexFormat = DXGIFormat( geometry.m_vertexFormat );

            pD3D12AccelerationStructure->m_bottomLevel.m_geometries.push_back( d3d12GeometryDesc );
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS d3d12BottomLevelInputs = {};
        d3d12BottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        d3d12BottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        d3d12BottomLevelInputs.Flags = pD3D12AccelerationStructure->m_bottomLevel.m_flags;
        d3d12BottomLevelInputs.NumDescs = UINT( pD3D12AccelerationStructure->m_bottomLevel.m_geometries.size() );
        d3d12BottomLevelInputs.pGeometryDescs = pD3D12AccelerationStructure->m_bottomLevel.m_geometries.data();

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO d3d12BottomLevelPrebuild = {};
        pD3D12Device5->GetRaytracingAccelerationStructurePrebuildInfo( &d3d12BottomLevelInputs, &d3d12BottomLevelPrebuild );

        BufferParameters bottomLevelStructureBufferParameters = {};
        bottomLevelStructureBufferParameters.m_descriptorTypes = DescriptorTypeFlags::RWBuffer;
        bottomLevelStructureBufferParameters.m_memoryType = ResourceMemoryType::DeviceLocal;
        bottomLevelStructureBufferParameters.m_flags.SetMultipleFlags( BufferFlags::OwnMemory, BufferFlags::NoDescriptors );
        bottomLevelStructureBufferParameters.m_bufferSize = d3d12BottomLevelPrebuild.ResultDataMaxSizeInBytes;
        bottomLevelStructureBufferParameters.m_bufferStride = sizeof( UINT32 );
        pD3D12AccelerationStructure->m_bottomLevel.m_structureBuffer = CreateBuffer( pD3D12Context, bottomLevelStructureBufferParameters );

        Direct3D12Buffer* pD3D12InstanceBuffer = static_cast<Direct3D12Buffer*>( topLevelParameters.m_pInstanceBuffer );

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS d3d12TopLevelInputs = {};
        d3d12TopLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        d3d12TopLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        d3d12TopLevelInputs.Flags = pD3D12AccelerationStructure->m_flags;
        d3d12TopLevelInputs.NumDescs = UINT( topLevelParameters.m_numInstances );
        d3d12TopLevelInputs.InstanceDescs = pD3D12InstanceBuffer->m_deviceAddress + topLevelParameters.m_instanceBufferOffset;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO d3d12TopLevelPrebuild = {};
        pD3D12Device5->GetRaytracingAccelerationStructurePrebuildInfo( &d3d12TopLevelInputs, &d3d12TopLevelPrebuild );

        BufferParameters scratchBufferParameters = {};
        scratchBufferParameters.m_descriptorTypes = DescriptorTypeFlags::RWBuffer;
        scratchBufferParameters.m_memoryType = ResourceMemoryType::DeviceLocal;
        scratchBufferParameters.m_bufferSize = d3d12BottomLevelPrebuild.ScratchDataSizeInBytes;
        scratchBufferParameters.m_flags = BufferFlags::NoDescriptors;
        pD3D12AccelerationStructure->m_scratchBuffer = CreateBuffer( pD3D12Context, scratchBufferParameters );

        BufferParameters topLevelStructureBufferParameters = {};
        topLevelStructureBufferParameters.m_descriptorTypes.SetMultipleFlags( DescriptorTypeFlags::RWBuffer, DescriptorTypeFlags::Raw );
        topLevelStructureBufferParameters.m_memoryType = ResourceMemoryType::DeviceLocal;
        topLevelStructureBufferParameters.m_bufferSize = d3d12TopLevelPrebuild.ResultDataMaxSizeInBytes;
        topLevelStructureBufferParameters.m_bufferStride = sizeof( UINT32 );
        topLevelStructureBufferParameters.m_flags = BufferFlags::NoDescriptors;
        pD3D12AccelerationStructure->m_structureBuffer = CreateBuffer( pD3D12Context, topLevelStructureBufferParameters );

        //pD3D12AccelerationStructure->m_instanceBuffer = topLevelParameters.m_pInstanceBuffer;
        pD3D12AccelerationStructure->m_numInstances = topLevelParameters.m_numInstances;

        return pD3D12AccelerationStructure;
    }

    EE_BASE_API void DestroyAccelerationStructure( Context* pContext, AccelerationStructure*&& pAccelerationStructure )
    {
        if ( pAccelerationStructure )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12AccelerationStructure* pD3D12AccelerationStructure = static_cast<Direct3D12AccelerationStructure*>( pAccelerationStructure );

            pD3D12Context->DestroyObject( eastl::move( pD3D12AccelerationStructure ) );
            pAccelerationStructure = nullptr;
        }
    }

    EE_BASE_API AccelerationStructureHandle GetAccelerationStructureHandle( AccelerationStructure const* pAccelerationStructure )
    {
        Direct3D12AccelerationStructure const* pD3D12AccelerationStructure = static_cast<Direct3D12AccelerationStructure const*>( pAccelerationStructure );
        return GetBufferHandle( pD3D12AccelerationStructure->m_structureBuffer, DescriptorTypeFlags::Buffer );
    }

    EE_BASE_API Buffer* CreateBuffer( Context* pContext, BufferParameters const& parameters )
    {
        EE_ASSERT( pContext != nullptr );

        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12Buffer*  pD3D12Buffer = pD3D12Context->CreateObject<Direct3D12Buffer>();

        uint64_t allocationSize = parameters.m_bufferSize;
        if ( parameters.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) )
        {
            allocationSize = Math::RoundUpToNearestMultiple64( allocationSize, pD3D12Context->m_deviceCapabilities.m_constantBufferAlignment );
        }

        EE_ASSERT( allocationSize );

        uint64_t bufferStride = parameters.m_bufferStride;
        if ( parameters.m_format != DataFormat::Undefined )
        {
            bufferStride = FormatBlockBitSize( parameters.m_format ) / 8;
        }

        if ( parameters.m_descriptorTypes.IsFlagSet( RHI::DescriptorTypeFlags::Raw ) )
        {
            EE_ASSERT( parameters.m_format == DataFormat::Undefined ||
                       parameters.m_format == DataFormat::R32_UInt ||
                       parameters.m_format == DataFormat::R32_SInt ||
                       parameters.m_format == DataFormat::R32_SFloat );
            bufferStride = sizeof( UINT );
        }

        D3D12_RESOURCE_DESC d3d12BufferDesc = {};
        d3d12BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d3d12BufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        d3d12BufferDesc.Width = allocationSize;
        d3d12BufferDesc.Height = 1;
        d3d12BufferDesc.DepthOrArraySize = 1;
        d3d12BufferDesc.MipLevels = 1;
        d3d12BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        d3d12BufferDesc.SampleDesc = { 1, 0 };
        d3d12BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        d3d12BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if ( parameters.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWBuffer ) )
        {
            d3d12BufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT d3d12PlacedFootprint = {};
        UINT64                             paddedSize = 0;
        pD3D12Context->m_device->GetCopyableFootprints( &d3d12BufferDesc, 0, 1, 0, &d3d12PlacedFootprint, nullptr, nullptr, &paddedSize );

        EE_ASSERT( d3d12PlacedFootprint.Offset == 0 );
        pD3D12Buffer->m_footprint = d3d12PlacedFootprint.Footprint;

        allocationSize = paddedSize;
        d3d12BufferDesc.Width = paddedSize;

        D3D12MA::ALLOCATION_DESC d3d12AllocationDesc = {};

        if ( parameters.m_memoryType == ResourceMemoryType::HostToDevice )
        {
            d3d12AllocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if ( parameters.m_memoryType == ResourceMemoryType::DeviceToHost )
        {
            d3d12AllocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
        }
        else
        {
            d3d12AllocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        }

        if ( parameters.m_flags.IsFlagSet( BufferFlags::OwnMemory ) )
        {
            d3d12AllocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
        }

        TBitFlags<DescriptorTypeFlags> descriptorTypes = parameters.m_descriptorTypes;
        if ( parameters.m_flags.IsFlagSet( BufferFlags::NoDescriptors ) )
        {
            descriptorTypes = {};
        }

        // TODO: Implement multi-GPU support in D3D12MA
        // if ( d3d12Context->deviceMode == DeviceMode::Linked )
        // {
        //     allocationDesc.CreationNodeMask = d3d12Context->NodeMask( parameters.nodeIndex );
        //     allocationDesc.VisibleNodeMask = allocationDesc.CreationNodeMask;
        //     for ( uint32_t sharedNodeIndex : parameters.sharedNodeIndices )
        //     {
        //         allocationDesc.VisibleNodeMask |= d3d12Context->NodeMask( sharedNodeIndex );
        //     }
        // }
        // else
        // {
        //     allocationDesc.CreationNodeMask = 1;
        //     allocationDesc.VisibleNodeMask = 1;
        // }

        HRESULT result = pD3D12Context->m_resourceAllocator->CreateResource( &d3d12AllocationDesc, &d3d12BufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, pD3D12Buffer->m_allocation.ReleaseAndGetAddressOf(), IID_PPV_ARGS( pD3D12Buffer->m_resource.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        TrackResourceAllocation( pD3D12Context, descriptorTypes, false, true, pD3D12Buffer->m_allocation.Get() );

        pD3D12Buffer->m_deviceAddress = pD3D12Buffer->m_resource->GetGPUVirtualAddress();
        EE_ASSERT( pD3D12Buffer->m_deviceAddress );

        SetDebugName( pD3D12Context, pD3D12Buffer, parameters.m_debugName );

        if ( parameters.m_memoryType != ResourceMemoryType::DeviceLocal && parameters.m_flags.IsFlagSet( BufferFlags::PersistentMap ) )
        {
            result = pD3D12Buffer->m_resource->Map( 0, nullptr, &pD3D12Buffer->m_pMappedAddress_WriteCombined );
            EE_ASSERT( SUCCEEDED( result ) );
        }

        if ( !parameters.m_flags.IsFlagSet( BufferFlags::NoDescriptors ) && descriptorTypes.AreAnyFlagsSet( DescriptorTypeFlags::ConstantBuffer, DescriptorTypeFlags::Buffer, DescriptorTypeFlags::RWBuffer ) )
        {
            uint32_t numDescriptors = 0;
            if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) ) { numDescriptors++; }
            if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::Buffer ) ) { numDescriptors++; }
            if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWBuffer ) ) { numDescriptors++; }

            DescriptorAllocator* pHostDescriptorAllocator = &pD3D12Context->m_hostResourceDescriptorAllocator;
            pD3D12Buffer->m_pHostResourceDescriptorAllocator = pHostDescriptorAllocator;
            pD3D12Buffer->m_hostDescriptorHandles = pHostDescriptorAllocator->AllocateDescriptors( numDescriptors, parameters.m_debugName.c_str() );

            DescriptorAllocator* pDeviceDescriptorAllocator = &pD3D12Context->m_deviceResourceDescriptorAllocators[parameters.m_nodeIndex];
            pD3D12Buffer->m_pDeviceResourceDescriptorAllocator = pDeviceDescriptorAllocator;
            pD3D12Buffer->m_deviceDescriptorHandles = pDeviceDescriptorAllocator->AllocateDescriptors( numDescriptors, parameters.m_debugName.c_str() );

            EE_ASSERT( pD3D12Buffer->m_hostDescriptorHandles.m_offset == pD3D12Buffer->m_deviceDescriptorHandles.m_offset );

            if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) )
            {
                pD3D12Buffer->m_srvDescriptorOffset = 1;

                D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc = {};
                d3d12ConstantBufferViewDesc.BufferLocation = pD3D12Buffer->m_deviceAddress;
                d3d12ConstantBufferViewDesc.SizeInBytes = UINT( allocationSize );

                D3D12_CPU_DESCRIPTOR_HANDLE d3d12HostDescriptorHandle = pHostDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_hostDescriptorHandles.m_offset );
                D3D12_CPU_DESCRIPTOR_HANDLE d3d12DeviceDescriptorHandle = pDeviceDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_deviceDescriptorHandles.m_offset );

                pD3D12Context->m_device->CreateConstantBufferView( &d3d12ConstantBufferViewDesc, d3d12HostDescriptorHandle );
                pD3D12Context->m_device->CopyDescriptorsSimple( 1, d3d12DeviceDescriptorHandle, d3d12HostDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
            }
            else
            {
                pD3D12Buffer->m_srvDescriptorOffset = 0;
            }

            if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::Buffer ) )
            {
                EE_ASSERT( ( allocationSize % bufferStride ) == 0 );
                uint64_t numElements = parameters.m_bufferSize / bufferStride;

                EE_ASSERT( pD3D12Buffer->m_srvDescriptorOffset != -1 );
                pD3D12Buffer->m_uavDescriptorOffset = int8_t( pD3D12Buffer->m_srvDescriptorOffset + int8_t( 1 ) );

                D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
                d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                d3d12ShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                d3d12ShaderResourceViewDesc.Buffer.FirstElement = parameters.m_firstElement;
                d3d12ShaderResourceViewDesc.Buffer.NumElements = UINT( numElements );
                d3d12ShaderResourceViewDesc.Buffer.StructureByteStride = UINT( bufferStride );
                d3d12ShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                d3d12ShaderResourceViewDesc.Format = DXGIFormat( parameters.m_format );

                if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::Raw ) )
                {
                    d3d12ShaderResourceViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                    d3d12ShaderResourceViewDesc.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
                }

                if ( d3d12ShaderResourceViewDesc.Format != DXGI_FORMAT_UNKNOWN )
                {
                    d3d12ShaderResourceViewDesc.Buffer.StructureByteStride = 0;
                }

                D3D12_CPU_DESCRIPTOR_HANDLE d3d12HostDescriptorHandle = pHostDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_hostDescriptorHandles.m_offset + pD3D12Buffer->m_srvDescriptorOffset );
                D3D12_CPU_DESCRIPTOR_HANDLE d3d12DeviceDescriptorHandle = pDeviceDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_deviceDescriptorHandles.m_offset + pD3D12Buffer->m_srvDescriptorOffset );

                pD3D12Context->m_device->CreateShaderResourceView( pD3D12Buffer->m_resource.Get(), &d3d12ShaderResourceViewDesc, d3d12HostDescriptorHandle );
                pD3D12Context->m_device->CopyDescriptorsSimple( 1, d3d12DeviceDescriptorHandle, d3d12HostDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
            }
            else
            {
                pD3D12Buffer->m_uavDescriptorOffset = pD3D12Buffer->m_srvDescriptorOffset;
            }

            if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWBuffer ) )
            {
                EE_ASSERT( pD3D12Buffer->m_uavDescriptorOffset != -1 );
                EE_ASSERT( ( allocationSize % bufferStride ) == 0 );
                uint64_t numElements = parameters.m_bufferSize / bufferStride;

                D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12UnorderedAccessViewDesc = {};
                d3d12UnorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                d3d12UnorderedAccessViewDesc.Buffer.FirstElement = parameters.m_firstElement;
                d3d12UnorderedAccessViewDesc.Buffer.NumElements = UINT( numElements );
                d3d12UnorderedAccessViewDesc.Buffer.StructureByteStride = UINT( bufferStride );
                d3d12UnorderedAccessViewDesc.Buffer.CounterOffsetInBytes = 0;
                d3d12UnorderedAccessViewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

                if ( descriptorTypes.IsFlagSet( DescriptorTypeFlags::Raw ) )
                {
                    d3d12UnorderedAccessViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                    d3d12UnorderedAccessViewDesc.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
                }
                else if ( parameters.m_format != DataFormat::Undefined )
                {
                    EE_ASSERT( pD3D12Context->m_deviceCapabilities.m_canShaderWriteTo[size_t( parameters.m_format )] );

                    d3d12UnorderedAccessViewDesc.Format = DXGIFormat( parameters.m_format );
                }

                if ( d3d12UnorderedAccessViewDesc.Format != DXGI_FORMAT_UNKNOWN )
                {
                    d3d12UnorderedAccessViewDesc.Buffer.StructureByteStride = 0;
                }

                ID3D12Resource* pD3D12CounterResource = nullptr;
                if ( parameters.m_pCounterBuffer )
                {
                    pD3D12CounterResource = static_cast<Direct3D12Buffer*>( parameters.m_pCounterBuffer )->m_resource.Get();
                }

                D3D12_CPU_DESCRIPTOR_HANDLE d3d12HostDescriptorHandle = pHostDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_hostDescriptorHandles.m_offset + pD3D12Buffer->m_uavDescriptorOffset );
                D3D12_CPU_DESCRIPTOR_HANDLE d3d12DeviceDescriptorHandle = pDeviceDescriptorAllocator->DescriptorHostHandle( pD3D12Buffer->m_deviceDescriptorHandles.m_offset + pD3D12Buffer->m_uavDescriptorOffset );

                pD3D12Context->m_device->CreateUnorderedAccessView( pD3D12Buffer->m_resource.Get(), pD3D12CounterResource, &d3d12UnorderedAccessViewDesc, d3d12HostDescriptorHandle );
                pD3D12Context->m_device->CopyDescriptorsSimple( 1, d3d12DeviceDescriptorHandle, d3d12HostDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
            }
        }

        if ( parameters.m_flags.IsFlagSet( BufferFlags::SubAllocations ) )
        {
            D3D12MA::ALLOCATION_CALLBACKS d3d12AllocationCallbacks = {};
            d3d12AllocationCallbacks.pAllocate = [] ( size_t size, size_t alignment, void* )
            {
                return Memory::Allocators::g_D3D12.Alloc( size, alignment );
            };
            d3d12AllocationCallbacks.pFree = [] ( void* pPtr, void* )
            {
                if ( pPtr )
                {
                    Memory::Allocators::g_D3D12.Free( pPtr );
                }
            };

            D3D12MA::VIRTUAL_BLOCK_DESC d3d12VirtualBlockDesc = {};
            d3d12VirtualBlockDesc.pAllocationCallbacks = &d3d12AllocationCallbacks;
            d3d12VirtualBlockDesc.Size = paddedSize;

            result = D3D12MA::CreateVirtualBlock( &d3d12VirtualBlockDesc, pD3D12Buffer->m_virtualBlock.ReleaseAndGetAddressOf() );
            EE_ASSERT( SUCCEEDED( result ) );
        }

        pD3D12Buffer->m_size = paddedSize;
        pD3D12Buffer->m_stride = parameters.m_bufferStride;
        pD3D12Buffer->m_memoryType = parameters.m_memoryType;
        pD3D12Buffer->m_nodeIndex = parameters.m_nodeIndex;
        pD3D12Buffer->m_descriptorTypes = descriptorTypes;

        #ifdef ENABLE_VERBOSE_BUFFER_CREATE
        EE_LOG_MESSAGE( LogCategory::Render, "RHI/CreateBuffer", "Created buffer \"%s\" with size %d", parameters.m_debugName.c_str(), parameters.m_bufferSize );
        #endif

        return pD3D12Buffer;
    }

    EE_BASE_API void DestroyBuffer( Context* pContext, Buffer*&& pBuffer )
    {
        if ( pBuffer )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

            if ( pD3D12Buffer->m_virtualBlock )
            {
                EE_ASSERT( pD3D12Buffer->m_virtualBlock->IsEmpty() );
            }

            if ( pD3D12Buffer->m_hostDescriptorHandles.IsValid() )
            {
                pD3D12Context->m_hostResourceDescriptorAllocator.FreeDescriptors( eastl::move( pD3D12Buffer->m_hostDescriptorHandles ) );
                pD3D12Context->m_deviceResourceDescriptorAllocators[pD3D12Buffer->m_nodeIndex].FreeDescriptors( eastl::move( pD3D12Buffer->m_deviceDescriptorHandles ) );
            }

            TrackResourceAllocation( pD3D12Context, pD3D12Buffer->m_descriptorTypes, false, false, pD3D12Buffer->m_allocation.Get() );

            pD3D12Context->DestroyObject( eastl::move( pD3D12Buffer ) );
            pBuffer = nullptr;
        }
    }

    EE_BASE_API void MapBuffer( Context* pContext, Buffer* pBuffer, ReadRange range )
    {
        Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

        pD3D12Buffer->m_mappedRange = { 0, pD3D12Buffer->m_size };
        if ( range.m_offset != 0 || range.m_size != 0 )
        {
            pD3D12Buffer->m_mappedRange.Begin += range.m_offset;
            pD3D12Buffer->m_mappedRange.End = pD3D12Buffer->m_mappedRange.Begin + range.m_size;
        }

        EE_ASSERT( pD3D12Buffer->m_mappedRange.Begin < pD3D12Buffer->m_size );
        EE_ASSERT( pD3D12Buffer->m_mappedRange.End <= pD3D12Buffer->m_size );

        HRESULT const result = pD3D12Buffer->m_resource->Map( 0, &pD3D12Buffer->m_mappedRange, &pD3D12Buffer->m_pMappedAddress_WriteCombined );
        EE_ASSERT( SUCCEEDED( result ) );
    }

    EE_BASE_API void UnmapBuffer( Context* pContext, Buffer* pBuffer )
    {
        Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

        pD3D12Buffer->m_resource->Unmap( 0, &pD3D12Buffer->m_mappedRange );
        pD3D12Buffer->m_pMappedAddress_WriteCombined = nullptr;
    }

    EE_BASE_API BufferHandle GetBufferHandle( Buffer const* pBuffer, DescriptorTypeFlags descriptorType )
    {
        Direct3D12Buffer const* pD3D12Buffer = static_cast<Direct3D12Buffer const*>( pBuffer );

        EE_ASSERT( pD3D12Buffer->m_descriptorTypes.IsFlagSet( descriptorType ) );
        EE_ASSERT( pD3D12Buffer->m_deviceDescriptorHandles.IsValid() );

        switch ( descriptorType )
        {
            case DescriptorTypeFlags::ConstantBuffer:
            {
                return uint16_t( pD3D12Buffer->m_deviceDescriptorHandles.m_offset );
            }

            case DescriptorTypeFlags::Buffer:
            {
                EE_ASSERT( pD3D12Buffer->m_srvDescriptorOffset != -1 );
                return uint16_t( pD3D12Buffer->m_deviceDescriptorHandles.m_offset ) + uint8_t( pD3D12Buffer->m_srvDescriptorOffset );
            }

            case DescriptorTypeFlags::RWBuffer:
            {
                EE_ASSERT( pD3D12Buffer->m_uavDescriptorOffset != -1 );
                return uint16_t( pD3D12Buffer->m_deviceDescriptorHandles.m_offset ) + uint8_t( pD3D12Buffer->m_uavDescriptorOffset );
            }

            default:
            {
                EE_ASSERT( false );
                return InvalidResourceHandle;
            }
        }
    }

    EE_BASE_API BufferSubAllocation BufferSubAllocate( Buffer* pBuffer, uint64_t size, uint64_t alignment )
    {
        Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );
        EE_ASSERT( pD3D12Buffer->m_virtualBlock != nullptr ); // Buffer was not created with suballocations flag

        D3D12MA::VIRTUAL_ALLOCATION_DESC d3d12Allocation = {};
        d3d12Allocation.Size = size;
        d3d12Allocation.Alignment = alignment;

        D3D12MA::VirtualAllocation d3d12VirtualAllocation = {};
        UINT64 d3d12Offset = 0;

        HRESULT const result = pD3D12Buffer->m_virtualBlock->Allocate( &d3d12Allocation, &d3d12VirtualAllocation, &d3d12Offset );
        if ( SUCCEEDED( result ) )
        {
            return BufferSubAllocation{ d3d12Offset, d3d12VirtualAllocation.AllocHandle };
        }

        EE_ASSERT( result == E_OUTOFMEMORY ); // Out of memory is expected, any other error needs investigation.
        return {};
    }

    EE_BASE_API void BufferSubDeallocate( Buffer* pBuffer, BufferSubAllocation&& subAllocation )
    {
        Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );
        EE_ASSERT( pD3D12Buffer->m_virtualBlock != nullptr ); // Buffer was not created with suballocations flag

        EE_ASSERT( subAllocation.IsValid() );

        static_assert( sizeof( D3D12MA::VirtualAllocation ) == sizeof( UINT64 ), "Internal structure has changed, BufferSubAllocation needs to be updated to accomodate it" );
        D3D12MA::VirtualAllocation d3d12VirtualAllocation = { subAllocation.m_internal };

        pD3D12Buffer->m_virtualBlock->FreeAllocation( d3d12VirtualAllocation );

        subAllocation = {};
    }

    EE_BASE_API Texture* CreateTexture( Context* pContext, TextureParameters const& parameters )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12Texture* pD3D12Texture = pD3D12Context->CreateObject<Direct3D12Texture>();

        if ( parameters.m_pNativeHandle )
        {
            pD3D12Texture->m_resource = static_cast<ID3D12Resource*>( parameters.m_pNativeHandle );
        }
        else if ( parameters.m_pTextureToAlias )
        {
            Direct3D12Texture* pD3D12TextureToAlias = static_cast<Direct3D12Texture*>( parameters.m_pTextureToAlias );

            pD3D12Texture->m_resource = pD3D12TextureToAlias->m_resource;
        }
        else
        {
            D3D12_RESOURCE_DESC1 d3d12ResourceDesc = {};
            if ( parameters.m_depth > 1 )
            {
                d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            }
            else if ( parameters.m_height > 1 )
            {
                d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            }
            else
            {
                d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            }

            if ( parameters.m_numSamples > 1 )
            {
                d3d12ResourceDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
            }

            d3d12ResourceDesc.Width = parameters.m_width;
            d3d12ResourceDesc.Height = parameters.m_height;
            if ( parameters.m_arrayLayers > 1 )
            {
                d3d12ResourceDesc.DepthOrArraySize = UINT16( parameters.m_arrayLayers );
            }
            else
            {
                d3d12ResourceDesc.DepthOrArraySize = UINT16( parameters.m_depth );
            }
            d3d12ResourceDesc.MipLevels = UINT16( parameters.m_mipLevels );
            d3d12ResourceDesc.Format = DXGIFormat( parameters.m_format );
            d3d12ResourceDesc.SampleDesc = { parameters.m_numSamples, parameters.m_sampleQuality };
            d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;


            D3D12_CLEAR_VALUE d3d12ClearValue = {};
            d3d12ClearValue.Format = d3d12ResourceDesc.Format;

            D3D12_CLEAR_VALUE* pD3D12ClearValue = nullptr;

            if ( parameters.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWTexture ) )
            {
                d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            if ( parameters.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RenderTarget ) )
            {
                if ( parameters.m_format == DataFormat::D32_SFloat || parameters.m_format == DataFormat::D32_SFloat_S8_UInt || parameters.m_format == DataFormat::S8_Uint )
                {
                    d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                    d3d12ClearValue.DepthStencil.Depth = parameters.m_clearValue.m_depth;
                    d3d12ClearValue.DepthStencil.Stencil = parameters.m_clearValue.m_stencil;
                    pD3D12ClearValue = &d3d12ClearValue;
                }
                else
                {
                    d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                    d3d12ClearValue.Color[0] = parameters.m_clearValue.m_red;
                    d3d12ClearValue.Color[1] = parameters.m_clearValue.m_green;
                    d3d12ClearValue.Color[2] = parameters.m_clearValue.m_blue;
                    d3d12ClearValue.Color[3] = parameters.m_clearValue.m_alpha;
                    pD3D12ClearValue = &d3d12ClearValue;
                }
            }
            if ( parameters.m_textureFlags.IsFlagSet( TextureFlags::ExportAdapter ) )
            {
                d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
                d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            }

            D3D12MA::ALLOCATION_DESC d3d12AllocationDesc = {};
            d3d12AllocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            if ( parameters.m_textureFlags.IsFlagSet( TextureFlags::OwnMemory ) )
            {
                d3d12AllocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
            }

            if ( parameters.m_textureFlags.IsFlagSet( TextureFlags::AllowDisplayTarget ) )
            {
                d3d12AllocationDesc.ExtraHeapFlags |= D3D12_HEAP_FLAG_ALLOW_DISPLAY;
            }

            // TODO: Implement multi-GPU support in D3D12MA
            // if ( d3d12Context->deviceMode == DeviceMode::Linked )
            // {
            //     allocationDesc.CreationNodeMask = d3d12Context->NodeMask( parameters.nodeIndex );
            //     allocationDesc.VisibleNodeMask = allocationDesc.CreationNodeMask;
            //     for ( uint32_t sharedNodeIndex : parameters.sharedNodeIndices )
            //     {
            //         allocationDesc.VisibleNodeMask |= d3d12Context->NodeMask( sharedNodeIndex );
            //     }
            // }
            // else
            // {
            //     allocationDesc.CreationNodeMask = 1;
            //     allocationDesc.VisibleNodeMask = 1;
            // }

            D3D12_BARRIER_LAYOUT d3d12InitialLayout = D3D12BarrierLayout( parameters.m_initialState );

            HRESULT const result = pD3D12Context->m_resourceAllocator->CreateResource3( &d3d12AllocationDesc, &d3d12ResourceDesc, d3d12InitialLayout, pD3D12ClearValue, 0, nullptr, pD3D12Texture->m_allocation.ReleaseAndGetAddressOf(), IID_PPV_ARGS( pD3D12Texture->m_resource.ReleaseAndGetAddressOf() ) );
            EE_ASSERT( SUCCEEDED( result ) );

            TrackResourceAllocation( pD3D12Context, parameters.m_descriptorTypes, true, true, pD3D12Texture->m_allocation.Get() );

            SetDebugName( pD3D12Context, pD3D12Texture, parameters.m_debugName );

            pD3D12Texture->m_footprints.resize( parameters.m_mipLevels * parameters.m_arrayLayers );

            for ( uint32_t arrayLayer = 0; arrayLayer < parameters.m_arrayLayers; ++arrayLayer )
            {
                for ( uint32_t mipLevel = 0; mipLevel < parameters.m_mipLevels; ++mipLevel )
                {
                    uint32_t subresourceIndex = SubresourceIndex( mipLevel, arrayLayer, 0,
                                                                  parameters.m_mipLevels,
                                                                  parameters.m_arrayLayers );

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT d3d12PlacedFootprint = {};
                    pD3D12Context->m_device->GetCopyableFootprints1( &d3d12ResourceDesc, subresourceIndex, 1, 0,
                                                                     &d3d12PlacedFootprint,
                                                                     nullptr, nullptr, nullptr );

                    EE_ASSERT( d3d12PlacedFootprint.Offset == 0 );

                    pD3D12Texture->m_footprints[subresourceIndex] = d3d12PlacedFootprint.Footprint;
                }
            }
        }

        pD3D12Texture->m_width = parameters.m_width;
        pD3D12Texture->m_height = parameters.m_height;
        pD3D12Texture->m_depth = parameters.m_depth;
        pD3D12Texture->m_arrayLayers = parameters.m_arrayLayers;
        pD3D12Texture->m_mipLevels = parameters.m_mipLevels;
        pD3D12Texture->m_format = parameters.m_format;
        pD3D12Texture->m_numSamples = parameters.m_numSamples;
        pD3D12Texture->m_sampleQuality = parameters.m_sampleQuality;
        pD3D12Texture->m_clearValue = parameters.m_clearValue;
        pD3D12Texture->m_descriptorTypes = parameters.m_descriptorTypes;
        pD3D12Texture->m_initialState = parameters.m_initialState;

        ViewDimension textureViewDimension = TextureViewDimension( parameters.m_width, parameters.m_height, parameters.m_depth,
                                                                   parameters.m_arrayLayers, parameters.m_numSamples,
                                                                   parameters.m_descriptorTypes );

        if ( parameters.m_descriptorTypes.AreAnyFlagsSet( DescriptorTypeFlags::Texture, DescriptorTypeFlags::TextureCube, DescriptorTypeFlags::RWTexture ) )
        {
            uint32_t numDescriptors = 0;
            if ( parameters.m_descriptorTypes.AreAnyFlagsSet( DescriptorTypeFlags::Texture, DescriptorTypeFlags::TextureCube ) )
            {
                numDescriptors++;
            }

            if ( parameters.m_descriptorTypes.AreAnyFlagsSet( DescriptorTypeFlags::RWTexture ) )
            {
                numDescriptors += parameters.m_mipLevels;
            }

            DescriptorAllocator* pHostDescriptorAllocator = &pD3D12Context->m_hostResourceDescriptorAllocator;
            pD3D12Texture->m_pHostResourceDescriptorAllocator = pHostDescriptorAllocator;
            pD3D12Texture->m_hostResourceDescriptorHandles = pHostDescriptorAllocator->AllocateDescriptors( numDescriptors, parameters.m_debugName.c_str() );

            DescriptorAllocator* pDeviceDescriptorAllocator = &pD3D12Context->m_deviceResourceDescriptorAllocators[parameters.m_nodeIndex];
            pD3D12Texture->m_pDeviceResourceDescriptorAllocator = pDeviceDescriptorAllocator;
            pD3D12Texture->m_deviceResourceDescriptorHandles = pDeviceDescriptorAllocator->AllocateDescriptors( numDescriptors, parameters.m_debugName.c_str() );

            EE_ASSERT( pD3D12Texture->m_hostResourceDescriptorHandles.m_offset == pD3D12Texture->m_deviceResourceDescriptorHandles.m_offset );

            // D32 formats should be converted to R32 formats
            DXGI_FORMAT dxgiShaderResourceFormat = DXGIFormat( parameters.m_format );
            if ( dxgiShaderResourceFormat == DXGI_FORMAT_D32_FLOAT )
            {
                dxgiShaderResourceFormat = DXGI_FORMAT_R32_FLOAT;
            }
            else if ( dxgiShaderResourceFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT )
            {
                dxgiShaderResourceFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            }
            else if ( dxgiShaderResourceFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT )
            {
                // TODO: Assume we want the depth path? What if we want the stencil part, does anybody need it?
                dxgiShaderResourceFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            }
            else if ( dxgiShaderResourceFormat == DXGI_FORMAT_D24_UNORM_S8_UINT )
            {
                // Assume we want stencil only
                dxgiShaderResourceFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
            }

            if ( parameters.m_descriptorTypes.AreAnyFlagsSet( DescriptorTypeFlags::Texture, DescriptorTypeFlags::TextureCube ) )
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
                d3d12ShaderResourceViewDesc.ViewDimension = D3D12ShaderResourceViewDimension( textureViewDimension );
                d3d12ShaderResourceViewDesc.Format = dxgiShaderResourceFormat;
                d3d12ShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

                // MipLevels address is the same for all types of texture (always the same member of the union)
                d3d12ShaderResourceViewDesc.Texture1D.MipLevels = parameters.m_mipLevels;

                if ( parameters.m_arrayLayers > 1 )
                {
                    if ( parameters.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::TextureCube ) )
                    {
                        d3d12ShaderResourceViewDesc.TextureCubeArray.NumCubes = parameters.m_arrayLayers / 6;
                    }
                    else
                    {
                        // ArraySize address is the same for all types of texture (always the same member of the union)
                        d3d12ShaderResourceViewDesc.Texture1DArray.ArraySize = parameters.m_arrayLayers;
                    }
                }

                pD3D12Texture->m_uavDescriptorOffset = 1;

                D3D12_CPU_DESCRIPTOR_HANDLE d3d12HostDescriptorHandle = pHostDescriptorAllocator->DescriptorHostHandle( pD3D12Texture->m_hostResourceDescriptorHandles.m_offset );
                D3D12_CPU_DESCRIPTOR_HANDLE d3d12DeviceDescriptorHandle = pDeviceDescriptorAllocator->DescriptorHostHandle( pD3D12Texture->m_deviceResourceDescriptorHandles.m_offset );

                pD3D12Context->m_device->CreateShaderResourceView( pD3D12Texture->m_resource.Get(), &d3d12ShaderResourceViewDesc, d3d12HostDescriptorHandle );
                pD3D12Context->m_device->CopyDescriptorsSimple( 1, d3d12DeviceDescriptorHandle, d3d12HostDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
            }
            else
            {
                pD3D12Texture->m_uavDescriptorOffset = 0;
            }

            if ( parameters.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWTexture ) )
            {
                EE_ASSERT( pD3D12Texture->m_uavDescriptorOffset != -1 );

                D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12UnorderedAccessViewDesc = {};
                d3d12UnorderedAccessViewDesc.ViewDimension = D3D12UnorderedAccessViewDimension( textureViewDimension );
                d3d12UnorderedAccessViewDesc.Format = dxgiShaderResourceFormat;

                if ( parameters.m_arrayLayers > 1 )
                {
                    // ArraySize address is the same for all types of texture (always the same member of the union)
                    d3d12UnorderedAccessViewDesc.Texture1DArray.ArraySize = parameters.m_arrayLayers;
                }

                for ( int32_t mipLevel = 0; mipLevel < int32_t( parameters.m_mipLevels ); ++mipLevel )
                {
                    // MipSlice address is the same for all types of texture (always the first member of the union)
                    d3d12UnorderedAccessViewDesc.Texture1D.MipSlice = mipLevel;

                    if ( parameters.m_depth > 1 )
                    {
                        d3d12UnorderedAccessViewDesc.Texture3D.WSize = parameters.m_depth / ( 1 << mipLevel );
                    }

                    D3D12_CPU_DESCRIPTOR_HANDLE d3d12HostDescriptorHandle = pHostDescriptorAllocator->DescriptorHostHandle( pD3D12Texture->m_hostResourceDescriptorHandles.m_offset + pD3D12Texture->m_uavDescriptorOffset + mipLevel );
                    D3D12_CPU_DESCRIPTOR_HANDLE d3d12DeviceDescriptorHandle = pDeviceDescriptorAllocator->DescriptorHostHandle( pD3D12Texture->m_deviceResourceDescriptorHandles.m_offset + pD3D12Texture->m_uavDescriptorOffset + mipLevel );

                    pD3D12Context->m_device->CreateUnorderedAccessView( pD3D12Texture->m_resource.Get(), nullptr, &d3d12UnorderedAccessViewDesc, d3d12HostDescriptorHandle );
                    pD3D12Context->m_device->CopyDescriptorsSimple( 1, d3d12DeviceDescriptorHandle, d3d12HostDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
                }
            }
        }

        if ( parameters.m_descriptorTypes.AreAnyFlagsSet( DescriptorTypeFlags::RenderTarget ) )
        {
            if ( parameters.m_format == DataFormat::D32_SFloat || parameters.m_format == DataFormat::D32_SFloat_S8_UInt || parameters.m_format == DataFormat::S8_Uint )
            {
                pD3D12Texture->m_pRenderTargetDescriptorAllocator = &pD3D12Context->m_hostDepthStencilDescriptorAllocator;

                D3D12_DEPTH_STENCIL_VIEW_DESC d3d12DepthStencilViewDesc = {};
                d3d12DepthStencilViewDesc.ViewDimension = D3D12DepthStencilViewDimension( textureViewDimension );
                d3d12DepthStencilViewDesc.Format = DXGIFormat( parameters.m_format );

                uint32_t numHandles = parameters.m_mipLevels * parameters.m_arrayLayers;
                pD3D12Texture->m_renderTargetDescriptorHandles = pD3D12Context->m_hostDepthStencilDescriptorAllocator.AllocateDescriptors( numHandles, parameters.m_debugName.c_str() );

                for ( uint32_t arrayLayer = 0; arrayLayer < parameters.m_arrayLayers; ++arrayLayer )
                {
                    if ( parameters.m_arrayLayers > 1 )
                    {
                        // ArraySize address is the same for all types of texture (always the same member of the union)
                        d3d12DepthStencilViewDesc.Texture1DArray.ArraySize = 1;
                        d3d12DepthStencilViewDesc.Texture1DArray.FirstArraySlice = arrayLayer;
                    }

                    for ( uint32_t mipLevel = 0; mipLevel < parameters.m_mipLevels; ++mipLevel )
                    {
                        // MipSlice address is the same for all types of texture (always the first member of the union)
                        d3d12DepthStencilViewDesc.Texture1D.MipSlice = mipLevel;

                        pD3D12Context->m_device->CreateDepthStencilView( pD3D12Texture->m_resource.Get(), &d3d12DepthStencilViewDesc,
                                                                         pD3D12Texture->RenderTargetDescriptorHandle( arrayLayer, mipLevel ) );
                    }
                }
            }
            else
            {
                pD3D12Texture->m_pRenderTargetDescriptorAllocator = &pD3D12Context->m_hostRenderTargetDescriptorAllocator;

                D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
                d3d12RenderTargetViewDesc.ViewDimension = D3D12RenderTargetViewDimension( textureViewDimension );
                d3d12RenderTargetViewDesc.Format = DXGIFormat( parameters.m_format );

                uint32_t numHandles = parameters.m_mipLevels * parameters.m_arrayLayers;
                pD3D12Texture->m_renderTargetDescriptorHandles = pD3D12Context->m_hostRenderTargetDescriptorAllocator.AllocateDescriptors( numHandles, parameters.m_debugName.c_str() );

                for ( uint32_t arrayLayer = 0; arrayLayer < parameters.m_arrayLayers; ++arrayLayer )
                {
                    if ( parameters.m_arrayLayers > 1 )
                    {
                        // ArraySize address is the same for all types of texture (always the same member of the union)
                        d3d12RenderTargetViewDesc.Texture1DArray.ArraySize = 1;
                        d3d12RenderTargetViewDesc.Texture1DArray.FirstArraySlice = arrayLayer;
                    }

                    for ( uint32_t mipLevel = 0; mipLevel < parameters.m_mipLevels; ++mipLevel )
                    {
                        // MipSlice address is the same for all types of texture (always the first member of the union)
                        d3d12RenderTargetViewDesc.Texture1D.MipSlice = mipLevel;

                        pD3D12Context->m_device->CreateRenderTargetView( pD3D12Texture->m_resource.Get(), &d3d12RenderTargetViewDesc,
                                                                         pD3D12Texture->RenderTargetDescriptorHandle( arrayLayer, mipLevel ) );
                    }
                }
            }
        }

        return pD3D12Texture;
    }

    EE_BASE_API void DestroyTexture( Context* pContext, Texture*&& pTexture )
    {
        if ( pTexture )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Texture* pD3D12Texture = static_cast<Direct3D12Texture*>( pTexture );

            if ( pD3D12Texture->m_hostResourceDescriptorHandles.IsValid() )
            {
                pD3D12Context->m_hostResourceDescriptorAllocator.FreeDescriptors( eastl::move( pD3D12Texture->m_hostResourceDescriptorHandles ) );
                pD3D12Context->m_deviceResourceDescriptorAllocators[pD3D12Texture->m_nodeIndex].FreeDescriptors( eastl::move( pD3D12Texture->m_deviceResourceDescriptorHandles ) );
            }

            if ( pD3D12Texture->m_renderTargetDescriptorHandles.IsValid() )
            {
                if ( pD3D12Texture->m_format == DataFormat::D32_SFloat || pD3D12Texture->m_format == DataFormat::D32_SFloat_S8_UInt || pD3D12Texture->m_format == DataFormat::S8_Uint )
                {
                    pD3D12Context->m_hostDepthStencilDescriptorAllocator.FreeDescriptors( eastl::move( pD3D12Texture->m_renderTargetDescriptorHandles ) );
                }
                else
                {
                    pD3D12Context->m_hostRenderTargetDescriptorAllocator.FreeDescriptors( eastl::move( pD3D12Texture->m_renderTargetDescriptorHandles ) );
                }
            }

            TrackResourceAllocation( pD3D12Context, pD3D12Texture->m_descriptorTypes, true, false, pD3D12Texture->m_allocation.Get() );

            pD3D12Context->DestroyObject( eastl::move( pD3D12Texture ) );
            pTexture = nullptr;
        }
    }

    EE_BASE_API uint32_t GetTextureCopyRowStride( Texture const* pTexture, uint32_t mipLevel, uint32_t arrayLayer )
    {
        Direct3D12Texture const* pD3D12Texture = static_cast<Direct3D12Texture const*>( pTexture );

        uint32_t                    subresourceIndex = SubresourceIndex( mipLevel, arrayLayer, 0, pD3D12Texture->m_mipLevels, pD3D12Texture->m_arrayLayers );
        D3D12_SUBRESOURCE_FOOTPRINT d3d12Footprint = pD3D12Texture->m_footprints[subresourceIndex];

        return d3d12Footprint.RowPitch;
    }

    EE_BASE_API TextureHandle GetTextureHandle( Texture const* pTexture, DescriptorTypeFlags descriptorType, uint32_t rwTextureMipLevel )
    {
        Direct3D12Texture const* pD3D12Texture = static_cast<Direct3D12Texture const*>( pTexture );
        EE_ASSERT( pD3D12Texture->m_descriptorTypes.IsFlagSet( descriptorType ) );
        return pD3D12Texture->ResourceDescriptorHandle( descriptorType, rwTextureMipLevel );
    }

    EE_BASE_API Sampler* CreateSampler( Context* pContext, SamplerParameters const& parameters )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12Sampler* pD3D12Sampler = pD3D12Context->CreateObject<Direct3D12Sampler>();

        D3D12_SAMPLER_DESC d3d12SamplerDesc = {};
        d3d12SamplerDesc.Filter = D3D12Filter( parameters.m_minFilter, parameters.m_magFilter, parameters.m_mipMapMode, parameters.m_compareMode, parameters.m_filterMode, parameters.m_maxAnisotropy );
        d3d12SamplerDesc.AddressU = D3D12TextureAddressMode( parameters.m_addressModeU );
        d3d12SamplerDesc.AddressV = D3D12TextureAddressMode( parameters.m_addressModeV );
        d3d12SamplerDesc.AddressW = D3D12TextureAddressMode( parameters.m_addressModeW );
        d3d12SamplerDesc.MipLODBias = parameters.m_mipLODBias;
        d3d12SamplerDesc.MaxAnisotropy = parameters.m_maxAnisotropy;
        d3d12SamplerDesc.ComparisonFunc = D3D12ComparisonFunc( parameters.m_compareMode );
        d3d12SamplerDesc.BorderColor[0] = parameters.m_borderColor[0];
        d3d12SamplerDesc.BorderColor[1] = parameters.m_borderColor[1];
        d3d12SamplerDesc.BorderColor[2] = parameters.m_borderColor[2];
        d3d12SamplerDesc.BorderColor[3] = parameters.m_borderColor[3];
        d3d12SamplerDesc.MinLOD = parameters.m_minLOD;
        d3d12SamplerDesc.MaxLOD = parameters.m_maxLOD;

        pD3D12Sampler->m_samplerDesc = d3d12SamplerDesc;

        pD3D12Sampler->m_hostDescriptorHandle = pD3D12Context->m_hostSamplerDescriptorAllocator.AllocateDescriptors( 1, "SamplerState" );
        pD3D12Sampler->m_deviceDescriptorHandle = pD3D12Context->m_deviceSamplerDescriptorAllocators[parameters.m_nodeIndex].AllocateDescriptors( 1, "SamplerState" );

        pD3D12Sampler->m_nodeIndex = parameters.m_nodeIndex;

        EE_ASSERT( pD3D12Sampler->m_hostDescriptorHandle.m_offset == pD3D12Sampler->m_deviceDescriptorHandle.m_offset );

        D3D12_CPU_DESCRIPTOR_HANDLE d3d12HostDescriptorHandle = pD3D12Context->m_hostSamplerDescriptorAllocator.DescriptorHostHandle( pD3D12Sampler->m_hostDescriptorHandle.m_offset );
        D3D12_CPU_DESCRIPTOR_HANDLE d3d12DeviceDescriptorHandle = pD3D12Context->m_deviceSamplerDescriptorAllocators[parameters.m_nodeIndex].DescriptorHostHandle( pD3D12Sampler->m_deviceDescriptorHandle.m_offset );

        pD3D12Context->m_device->CreateSampler( &d3d12SamplerDesc, d3d12HostDescriptorHandle );
        pD3D12Context->m_device->CopyDescriptorsSimple( 1, d3d12DeviceDescriptorHandle, d3d12HostDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );

        return pD3D12Sampler;
    }

    EE_BASE_API void DestroySampler( Context* pContext, Sampler*&& pSampler )
    {
        if ( pSampler )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Sampler* pD3D12Sampler = static_cast<Direct3D12Sampler*>( pSampler );

            if ( pD3D12Sampler->m_hostDescriptorHandle.IsValid() )
            {
                pD3D12Context->m_hostSamplerDescriptorAllocator.FreeDescriptors( eastl::move( pD3D12Sampler->m_hostDescriptorHandle ) );
                pD3D12Context->m_deviceSamplerDescriptorAllocators[pD3D12Sampler->m_nodeIndex].FreeDescriptors( eastl::move( pD3D12Sampler->m_deviceDescriptorHandle ) );
            }

            pD3D12Context->DestroyObject( eastl::move( pD3D12Sampler ) );
            pSampler = nullptr;
        }
    }

    EE_BASE_API SamplerStateHandle GetSamplerStateHandle( Sampler const* pSampler )
    {
        Direct3D12Sampler const* pD3D12Sampler = static_cast<Direct3D12Sampler const*>( pSampler );
        EE_ASSERT( pD3D12Sampler->m_deviceDescriptorHandle.IsValid() );
        return SamplerStateHandle( pD3D12Sampler->m_deviceDescriptorHandle.m_offset );
    }

    EE_BASE_API Shader* CreateShader( Context* pContext, TInlineVector<ShaderByteCode, 2> const& shaderParameters )
    {
        Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12Shader*  pD3D12Shader = pD3D12Context->CreateObject<Direct3D12Shader>();

        size_t const numStages = shaderParameters.size();
        pD3D12Shader->m_shaderBlobs.resize( numStages );
        pD3D12Shader->m_stageReflections.resize( numStages );

        HRESULT result;

        for ( size_t shaderIndex = 0; shaderIndex < numStages; ++shaderIndex )
        {
            TBitFlags<ShaderStage> shaderStage = shaderParameters[shaderIndex].m_stage;

            pD3D12Shader->m_stages.AppendFlags( shaderStage );

            Blob byteCode = Embed::DecompressEmbeddedFile( shaderParameters[shaderIndex].m_pCompressedData, shaderParameters[shaderIndex].m_decodedSize, shaderParameters[shaderIndex].m_decompressedSize );

            if ( shaderStage.IsFlagSet( ShaderStage::Vertex ) )
            {
                EE_ASSERT( pD3D12Shader->m_vertexStageIndex == -1 );
                result = pD3D12Context->m_dxcUtils->CreateBlob( byteCode.data(), UINT( byteCode.size() ), DXC_CP_ACP, pD3D12Shader->m_shaderBlobs[shaderIndex].ReleaseAndGetAddressOf() );

                EE_ASSERT( SUCCEEDED( result ) );

                pD3D12Shader->m_stageReflections[shaderIndex] = ExtractReflection( pD3D12Shader->m_shaderBlobs[shaderIndex].Get(), ShaderStage::Vertex );
                pD3D12Shader->m_vertexStageIndex = int32_t( shaderIndex );
            }
            else if ( shaderStage.IsFlagSet( ShaderStage::Pixel ) )
            {
                EE_ASSERT( pD3D12Shader->m_pixelStageIndex == -1 );
                result = pD3D12Context->m_dxcUtils->CreateBlob( byteCode.data(), UINT( byteCode.size() ), DXC_CP_ACP, pD3D12Shader->m_shaderBlobs[shaderIndex].ReleaseAndGetAddressOf() );

                EE_ASSERT( SUCCEEDED( result ) );

                pD3D12Shader->m_stageReflections[shaderIndex] = ExtractReflection( pD3D12Shader->m_shaderBlobs[shaderIndex].Get(), ShaderStage::Pixel );
                pD3D12Shader->m_pixelStageIndex = int32_t( shaderIndex );
            }
            else if ( shaderStage.IsFlagSet( ShaderStage::Task ) )
            {
                EE_ASSERT( pD3D12Shader->m_taskStageIndex == -1 );
                result = pD3D12Context->m_dxcUtils->CreateBlob( byteCode.data(), UINT( byteCode.size() ), DXC_CP_ACP, pD3D12Shader->m_shaderBlobs[shaderIndex].ReleaseAndGetAddressOf() );

                EE_ASSERT( SUCCEEDED( result ) );

                pD3D12Shader->m_stageReflections[shaderIndex] = ExtractReflection( pD3D12Shader->m_shaderBlobs[shaderIndex].Get(), ShaderStage::Task );
                pD3D12Shader->m_taskStageIndex = int32_t( shaderIndex );
            }
            else if ( shaderStage.IsFlagSet( ShaderStage::Mesh ) )
            {
                EE_ASSERT( pD3D12Shader->m_meshStageIndex == -1 );
                result = pD3D12Context->m_dxcUtils->CreateBlob( byteCode.data(), UINT( byteCode.size() ), DXC_CP_ACP, pD3D12Shader->m_shaderBlobs[shaderIndex].ReleaseAndGetAddressOf() );

                EE_ASSERT( SUCCEEDED( result ) );

                pD3D12Shader->m_stageReflections[shaderIndex] = ExtractReflection( pD3D12Shader->m_shaderBlobs[shaderIndex].Get(), ShaderStage::Mesh );
                pD3D12Shader->m_meshStageIndex = int32_t( shaderIndex );
            }
            else if ( shaderStage.IsFlagSet( ShaderStage::Compute ) )
            {
                EE_ASSERT( pD3D12Shader->m_computeStageIndex == -1 );
                result = pD3D12Context->m_dxcUtils->CreateBlob( byteCode.data(), UINT( byteCode.size() ), DXC_CP_ACP, pD3D12Shader->m_shaderBlobs[shaderIndex].ReleaseAndGetAddressOf() );

                EE_ASSERT( SUCCEEDED( result ) );

                pD3D12Shader->m_stageReflections[shaderIndex] = ExtractReflection( pD3D12Shader->m_shaderBlobs[shaderIndex].Get(), ShaderStage::Compute );
                pD3D12Shader->m_computeStageIndex = int32_t( shaderIndex );
            }
        }

        return pD3D12Shader;
    }

    EE_BASE_API void DestroyShader( Context* pContext, Shader*&& pShader )
    {
        if ( pShader )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Shader* pD3D12Shader = static_cast<Direct3D12Shader*>( pShader );

            pD3D12Context->DestroyObject( eastl::move( pD3D12Shader ) );
            pShader = nullptr;
        }
    }

    EE_BASE_API RootSignature* CreateRootSignature( Context* pContext, RootSignatureParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12RootSignature* pD3D12RootSignature = pD3D12Context->CreateObject<Direct3D12RootSignature>();

        TBitFlags<ShaderStage> shaderStages = {};

        THashMap<StringView, size_t> descriptorNameToIndex{ Memory::Allocators::g_RHI };
        TVector<uint32_t>            constantSizes{ Memory::Allocators::g_RHI };

        for ( ShaderReflection const& shaderReflection : parameters.m_pShader->m_stageReflections )
        {
            shaderStages.AppendFlags( shaderReflection.m_shaderStages );

            for ( ShaderResource const& shaderResource : shaderReflection.m_shaderResources )
            {
                if ( auto foundDescriptorIndex = descriptorNameToIndex.find( shaderResource.m_name ); foundDescriptorIndex == descriptorNameToIndex.end() )
                {
                    size_t shaderResourceIndex = pD3D12RootSignature->m_shaderResources.size();
                    pD3D12RootSignature->m_shaderResources.push_back( shaderResource );

                    uint32_t constantSize = 0;
                    if ( shaderResource.m_descriptorTypeFlags.AreAnyFlagsSet( DescriptorTypeFlags::ConstantBuffer, DescriptorTypeFlags::RootConstant ) )
                    {
                        for ( ShaderVariable const& variable : shaderReflection.m_shaderVariables )
                        {
                            size_t parentResourceIndex = ( &shaderResource - shaderReflection.m_shaderResources.data() );
                            if ( variable.m_parentResourceIndex == parentResourceIndex )
                            {
                                constantSize += variable.m_size;
                            }
                        }
                    }
                    constantSizes.push_back( constantSize );

                    descriptorNameToIndex.insert( { shaderResource.m_name, shaderResourceIndex } );
                }
                else
                {
                    ShaderResource& targetShaderResource = pD3D12RootSignature->m_shaderResources[foundDescriptorIndex->second];
                    EE_ASSERT( shaderResource.m_descriptorTypeFlags == targetShaderResource.m_descriptorTypeFlags );
                    EE_ASSERT( shaderResource.m_registerIndex == targetShaderResource.m_registerIndex );
                    EE_ASSERT( shaderResource.m_setIndex == targetShaderResource.m_setIndex );

                    targetShaderResource.m_usedStages.AppendFlags( shaderResource.m_usedStages );
                }
            }
        }

        pD3D12RootSignature->m_shaderResources.shrink_to_fit();
        pD3D12RootSignature->m_descriptorReflections.reserve( pD3D12RootSignature->m_shaderResources.size() );

        TInlineVector<D3D12_ROOT_PARAMETER1, D3D12_MAX_ROOT_COST> d3d12RootParameters;

        for ( uint32_t shaderResourceIndex = 0; shaderResourceIndex < pD3D12RootSignature->m_shaderResources.size(); ++shaderResourceIndex )
        {
            ShaderResource const& shaderResource = pD3D12RootSignature->m_shaderResources[shaderResourceIndex];
            DescriptorReflection  descriptorReflection = {};
            descriptorReflection.m_name = shaderResource.m_name;
            descriptorReflection.m_descriptorTypeFlags = shaderResource.m_descriptorTypeFlags;
            descriptorReflection.m_viewDimension = shaderResource.m_viewDimension;
            descriptorReflection.m_numConstants = shaderResource.m_numConstants;
            descriptorReflection.m_parameterIndex = int32_t( d3d12RootParameters.size() );
            descriptorReflection.m_setIndex = int32_t( shaderResource.m_setIndex );

            D3D12_ROOT_PARAMETER1 d3d12RootParameter = {};
            d3d12RootParameter.ShaderVisibility = D3D12ShaderVisibility( shaderResource.m_usedStages );
            d3d12RootParameter.Descriptor.ShaderRegister = shaderResource.m_registerIndex;
            d3d12RootParameter.Descriptor.RegisterSpace = shaderResource.m_setIndex;

            if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::RootConstant ) )
            {
                uint32_t numRootConstants = constantSizes[shaderResourceIndex] / sizeof( uint32_t );
                descriptorReflection.m_numConstants = numRootConstants;
                d3d12RootParameter.Constants.Num32BitValues = numRootConstants;
                d3d12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) )
            {
                d3d12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( DescriptorTypeFlags::Buffer, DescriptorTypeFlags::Texture ) )
            {
                d3d12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            }
            else if ( descriptorReflection.m_descriptorTypeFlags.AreAnyFlagsSet( DescriptorTypeFlags::RWBuffer, DescriptorTypeFlags::RWTexture ) )
            {
                d3d12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
            }

            d3d12RootParameters.push_back( d3d12RootParameter );
            pD3D12RootSignature->m_descriptorReflections.push_back( descriptorReflection );
        }

        TInlineVector<D3D12_STATIC_SAMPLER_DESC, 16> d3d12StaticSamplers = {};
        for ( size_t staticSamplerIndex = 0; staticSamplerIndex < parameters.m_staticSamplerNames.size(); ++staticSamplerIndex )
        {
            auto pSamplerShaderResource = eastl::find_if( pD3D12RootSignature->m_shaderResources.begin(), pD3D12RootSignature->m_shaderResources.end(),
                                                          [&] ( ShaderResource const& shaderResource )
            {
                return shaderResource.m_name == parameters.m_staticSamplerNames[staticSamplerIndex];
            } );

            if ( !pSamplerShaderResource || pSamplerShaderResource == pD3D12RootSignature->m_shaderResources.end() )
            {
                EE_LOG_WARNING( LogCategory::Render, "RHI/CreateRootSignature", "Static sampler \"%s\" not found in RootSignature \"%s\"",
                                parameters.m_staticSamplerNames[staticSamplerIndex], parameters.m_debugName.data() );

                continue;
            }

            Direct3D12Sampler* pD3D12Sampler = static_cast<Direct3D12Sampler*>( parameters.m_staticSamplers[staticSamplerIndex] );

            D3D12_STATIC_SAMPLER_DESC d3d12StaticSamplerDesc = {};
            d3d12StaticSamplerDesc.Filter = pD3D12Sampler->m_samplerDesc.Filter;
            d3d12StaticSamplerDesc.AddressU = pD3D12Sampler->m_samplerDesc.AddressU;
            d3d12StaticSamplerDesc.AddressV = pD3D12Sampler->m_samplerDesc.AddressV;
            d3d12StaticSamplerDesc.AddressW = pD3D12Sampler->m_samplerDesc.AddressW;
            d3d12StaticSamplerDesc.MipLODBias = pD3D12Sampler->m_samplerDesc.MipLODBias;
            d3d12StaticSamplerDesc.MaxAnisotropy = pD3D12Sampler->m_samplerDesc.MaxAnisotropy;
            d3d12StaticSamplerDesc.ComparisonFunc = pD3D12Sampler->m_samplerDesc.ComparisonFunc;

            d3d12StaticSamplerDesc.ShaderRegister = pSamplerShaderResource->m_registerIndex;
            d3d12StaticSamplerDesc.RegisterSpace = pSamplerShaderResource->m_setIndex;
            d3d12StaticSamplerDesc.ShaderVisibility = D3D12ShaderVisibility( pSamplerShaderResource->m_usedStages );

            if ( pD3D12Sampler->m_samplerDesc.BorderColor[0] == 0.0 &&
                 pD3D12Sampler->m_samplerDesc.BorderColor[1] == 0.0 &&
                 pD3D12Sampler->m_samplerDesc.BorderColor[2] == 0.0 &&
                 pD3D12Sampler->m_samplerDesc.BorderColor[3] == 0.0 )
            {
                d3d12StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            }
            else if ( pD3D12Sampler->m_samplerDesc.BorderColor[0] == 0.0 &&
                      pD3D12Sampler->m_samplerDesc.BorderColor[1] == 0.0 &&
                      pD3D12Sampler->m_samplerDesc.BorderColor[2] == 0.0 &&
                      pD3D12Sampler->m_samplerDesc.BorderColor[3] == 1.0 )
            {
                d3d12StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            }
            else if ( pD3D12Sampler->m_samplerDesc.BorderColor[0] == 1.0 &&
                      pD3D12Sampler->m_samplerDesc.BorderColor[1] == 1.0 &&
                      pD3D12Sampler->m_samplerDesc.BorderColor[2] == 1.0 &&
                      pD3D12Sampler->m_samplerDesc.BorderColor[3] == 1.0 )
            {
                d3d12StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            }
            else
            {
                EE_ASSERT( false );
            }

            d3d12StaticSamplerDesc.MinLOD = pD3D12Sampler->m_samplerDesc.MinLOD;
            d3d12StaticSamplerDesc.MaxLOD = pD3D12Sampler->m_samplerDesc.MaxLOD;

            d3d12StaticSamplers.push_back( d3d12StaticSamplerDesc );
        }

        uint32_t rootSignatureSizeDWORDs = 0;
        for ( const D3D12_ROOT_PARAMETER1& rootParameter : d3d12RootParameters )
        {
            if ( rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS )
            {
                rootSignatureSizeDWORDs += rootParameter.Constants.Num32BitValues;
            }
            else
            {
                rootSignatureSizeDWORDs += 2; // Root descriptor is a 64-bit address, so it's 2 DWORDs
            }
        }

        if ( rootSignatureSizeDWORDs > pD3D12Context->m_deviceCapabilities.m_optimalRootSignatureSizeInDWORDs )
        {
            EE_LOG_WARNING( LogCategory::Render, "RHI/CreateRootSignature", "RootSignature \"%s\" size is %u DWORDs, %u DWORDs is recommended for optimal performance",
                            parameters.m_debugName.data(), rootSignatureSizeDWORDs, pD3D12Context->m_deviceCapabilities.m_optimalRootSignatureSizeInDWORDs );
        }

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc = {};
        d3d12RootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        d3d12RootSignatureDesc.Desc_1_1.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

        if ( !shaderStages.IsFlagSet( ShaderStage::Vertex ) )
        {
            d3d12RootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        }
        if ( !shaderStages.IsFlagSet( ShaderStage::Pixel ) )
        {
            d3d12RootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
        }
        if ( !shaderStages.IsFlagSet( RHI::ShaderStage::Task ) )
        {
            d3d12RootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
        }
        if ( !shaderStages.IsFlagSet( RHI::ShaderStage::Mesh ) )
        {
            d3d12RootSignatureDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
        }

        d3d12RootSignatureDesc.Desc_1_1.NumParameters = UINT( d3d12RootParameters.size() );
        d3d12RootSignatureDesc.Desc_1_1.pParameters = d3d12RootParameters.data();
        d3d12RootSignatureDesc.Desc_1_1.NumStaticSamplers = UINT( d3d12StaticSamplers.size() );
        d3d12RootSignatureDesc.Desc_1_1.pStaticSamplers = d3d12StaticSamplers.data();

        ComPtr<ID3DBlob> d3d12RootSignatureError = {};
        ComPtr<ID3DBlob> d3d12RootSignatureString = {};

        HRESULT result = D3D12SerializeVersionedRootSignature( &d3d12RootSignatureDesc, d3d12RootSignatureString.ReleaseAndGetAddressOf(), d3d12RootSignatureError.ReleaseAndGetAddressOf() );
        EE_ASSERT( SUCCEEDED( result ) );

        result = pD3D12Context->m_device->CreateRootSignature( pD3D12Context->SharedNodeMask(), d3d12RootSignatureString->GetBufferPointer(), d3d12RootSignatureString->GetBufferSize(), IID_PPV_ARGS( pD3D12RootSignature->m_rootSignature.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12RootSignature, parameters.m_debugName );

        return pD3D12RootSignature;
    }

    EE_BASE_API void DestroyRootSignature( Context* pContext, RootSignature*&& pRootSignature )
    {
        if ( pRootSignature )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( pRootSignature );

            pD3D12Context->DestroyObject( eastl::move( pD3D12RootSignature ) );
            pRootSignature = nullptr;
        }
    }

    EE_BASE_API PipelineCache* CreatePipelineCache( Context* pContext, PipelineCacheParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12PipelineCache* pD3D12PipelineCache = pD3D12Context->CreateObject<Direct3D12PipelineCache>();

        pD3D12PipelineCache->m_pipelineLibraryData.resize( parameters.m_initialCacheData.size() );
        memcpy( pD3D12PipelineCache->m_pipelineLibraryData.data(), parameters.m_initialCacheData.data(),
                parameters.m_initialCacheData.size_bytes() );

        D3D12_FEATURE_DATA_SHADER_CACHE d3d12ShaderCacheFeature = {};
        HRESULT result = pD3D12Context->m_device->CheckFeatureSupport( D3D12_FEATURE_SHADER_CACHE, &d3d12ShaderCacheFeature, sizeof( d3d12ShaderCacheFeature ) );

        EE_ASSERT( SUCCEEDED( result ) );

        if ( d3d12ShaderCacheFeature.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_LIBRARY )
        {
            ComPtr<ID3D12Device1> pD3D12Device1 = {};
            result = pD3D12Context->m_device->QueryInterface( IID_PPV_ARGS( pD3D12Device1.ReleaseAndGetAddressOf() ) );
            EE_ASSERT( SUCCEEDED( result ) );

            result = pD3D12Device1->CreatePipelineLibrary( pD3D12PipelineCache->m_pipelineLibraryData.data(), pD3D12PipelineCache->m_pipelineLibraryData.size(), IID_PPV_ARGS( pD3D12PipelineCache->m_pipelineLibrary.ReleaseAndGetAddressOf() ) );
            EE_ASSERT( SUCCEEDED( result ) );
        }

        return pD3D12PipelineCache;
    }

    EE_BASE_API void DestroyPipelineCache( Context* pContext, PipelineCache*&& pPipelineCache )
    {
        if ( pPipelineCache )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12PipelineCache* pD3D12PipelineCache = static_cast<Direct3D12PipelineCache*>( pPipelineCache );

            pD3D12Context->DestroyObject( eastl::move( pD3D12PipelineCache ) );
            pPipelineCache = nullptr;
        }
    }

    EE_BASE_API TArrayView<uint8_t> GetPipelineCacheData( Context* pContext, PipelineCache* pPipelineCache )
    {
        Direct3D12PipelineCache* pD3D12PipelineCache = static_cast<Direct3D12PipelineCache*>( pPipelineCache );

        size_t serializedSize = pD3D12PipelineCache->m_pipelineLibrary->GetSerializedSize();
        pD3D12PipelineCache->m_pipelineLibraryData.resize( serializedSize );
        HRESULT const result = pD3D12PipelineCache->m_pipelineLibrary->Serialize( pD3D12PipelineCache->m_pipelineLibraryData.data(), pD3D12PipelineCache->m_pipelineLibraryData.size() );

        EE_ASSERT( SUCCEEDED( result ) );

        return pD3D12PipelineCache->m_pipelineLibraryData;
    }

    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, GraphicsPipelineParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12PipelineCache* pD3D12PipelineCache = static_cast<Direct3D12PipelineCache*>( parameters.m_pPipelineCache );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pRootSignature );
        Direct3D12Shader*        pD3D12Shader = static_cast<Direct3D12Shader*>( parameters.m_pShader );
        Direct3D12Pipeline*      pD3D12Pipeline = pD3D12Context->CreateObject<Direct3D12Pipeline>();

        EE_ASSERT( pD3D12PipelineCache == nullptr ); // Not implemented yet

        pD3D12Pipeline->m_pRootSignature = pD3D12RootSignature;
        pD3D12Pipeline->m_pipelineType = PipelineType::Graphics;
        pD3D12Pipeline->m_primitiveTopology = D3D12PrimitiveTopology( parameters.m_primitiveTopology );

        D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12PipelineStateDesc = {};
        d3d12PipelineStateDesc.pRootSignature = pD3D12RootSignature->m_rootSignature.Get();

        if ( pD3D12Shader->m_stages.IsFlagSet( ShaderStage::Vertex ) )
        {
            IDxcBlobEncoding* pDxcShaderBlob = pD3D12Shader->m_shaderBlobs[pD3D12Shader->m_vertexStageIndex].Get();

            d3d12PipelineStateDesc.VS.BytecodeLength = pDxcShaderBlob->GetBufferSize();
            d3d12PipelineStateDesc.VS.pShaderBytecode = pDxcShaderBlob->GetBufferPointer();
        }

        if ( pD3D12Shader->m_stages.IsFlagSet( ShaderStage::Pixel ) )
        {
            IDxcBlobEncoding* pDxcShaderBlob = pD3D12Shader->m_shaderBlobs[pD3D12Shader->m_pixelStageIndex].Get();

            d3d12PipelineStateDesc.PS.BytecodeLength = pDxcShaderBlob->GetBufferSize();
            d3d12PipelineStateDesc.PS.pShaderBytecode = pDxcShaderBlob->GetBufferPointer();
        }

        d3d12PipelineStateDesc.DepthStencilState.DepthEnable = parameters.m_depthStencilState.m_depthTest;
        d3d12PipelineStateDesc.DepthStencilState.DepthFunc = D3D12ComparisonFunc( parameters.m_depthStencilState.m_depthCompareMode );
        if ( parameters.m_depthStencilState.m_depthWrite )
        {
            d3d12PipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        }
        else
        {
            d3d12PipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        }

        d3d12PipelineStateDesc.DepthStencilState.StencilEnable = parameters.m_depthStencilState.m_stencilTest;
        d3d12PipelineStateDesc.DepthStencilState.StencilReadMask = parameters.m_depthStencilState.m_stencilReadMask;
        d3d12PipelineStateDesc.DepthStencilState.StencilWriteMask = parameters.m_depthStencilState.m_stencilWriteMask;

        d3d12PipelineStateDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_depthBackFail );
        d3d12PipelineStateDesc.DepthStencilState.BackFace.StencilFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilBackFail );
        d3d12PipelineStateDesc.DepthStencilState.BackFace.StencilPassOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilBackPass );
        d3d12PipelineStateDesc.DepthStencilState.BackFace.StencilFunc = D3D12ComparisonFunc( parameters.m_depthStencilState.m_stencilBackCompareMode );

        d3d12PipelineStateDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_depthFrontFail );
        d3d12PipelineStateDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilFrontFail );
        d3d12PipelineStateDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilFrontPass );
        d3d12PipelineStateDesc.DepthStencilState.FrontFace.StencilFunc = D3D12ComparisonFunc( parameters.m_depthStencilState.m_stencilFrontCompareMode );

        d3d12PipelineStateDesc.RasterizerState.CullMode = D3D12CullMode( parameters.m_rasterizerState.m_cullMode );
        d3d12PipelineStateDesc.RasterizerState.DepthBias = parameters.m_rasterizerState.m_depthBias;
        d3d12PipelineStateDesc.RasterizerState.DepthBiasClamp = parameters.m_rasterizerState.m_depthBiasClamp;
        d3d12PipelineStateDesc.RasterizerState.DepthClipEnable = parameters.m_rasterizerState.m_depthClip;
        d3d12PipelineStateDesc.RasterizerState.FillMode = D3D12FillMode( parameters.m_rasterizerState.m_fillMode );
        d3d12PipelineStateDesc.RasterizerState.FrontCounterClockwise = parameters.m_rasterizerState.m_frontFace == FrontFace::ClockWise;
        d3d12PipelineStateDesc.RasterizerState.MultisampleEnable = parameters.m_rasterizerState.m_multisample;
        d3d12PipelineStateDesc.RasterizerState.SlopeScaledDepthBias = parameters.m_rasterizerState.m_slopeScaledDepthBias;

        d3d12PipelineStateDesc.BlendState.AlphaToCoverageEnable = parameters.m_blendState.m_alphaToCoverage;
        d3d12PipelineStateDesc.BlendState.IndependentBlendEnable = parameters.m_blendState.m_independentBlend;

        for ( uint32_t renderTargetIndex = 0; renderTargetIndex < MaxRenderTargets; ++renderTargetIndex )
        {
            if ( parameters.m_blendState.m_renderTargetMask.IsFlagSet( BlendStateTargetFlags( renderTargetIndex ) ) )
            {
                D3D12_RENDER_TARGET_BLEND_DESC d3d12BlendDesc = {};
                d3d12BlendDesc.BlendEnable = parameters.m_blendState.m_blendEnabled;
                d3d12BlendDesc.SrcBlend = D3D12Blend( parameters.m_blendState.m_srcFactors[renderTargetIndex] );
                d3d12BlendDesc.DestBlend = D3D12Blend( parameters.m_blendState.m_dstFactors[renderTargetIndex] );
                d3d12BlendDesc.BlendOp = D3D12BlendOp( parameters.m_blendState.m_blendModes[renderTargetIndex] );
                d3d12BlendDesc.SrcBlendAlpha = D3D12Blend( parameters.m_blendState.m_srcAlphaFactors[renderTargetIndex] );
                d3d12BlendDesc.DestBlendAlpha = D3D12Blend( parameters.m_blendState.m_dstAlphaFactors[renderTargetIndex] );
                d3d12BlendDesc.BlendOpAlpha = D3D12BlendOp( parameters.m_blendState.m_blendModesAlpha[renderTargetIndex] );
                d3d12BlendDesc.RenderTargetWriteMask = parameters.m_blendState.m_writeMasks[renderTargetIndex];

                d3d12PipelineStateDesc.BlendState.RenderTarget[renderTargetIndex] = d3d12BlendDesc;
            }
        }

        d3d12PipelineStateDesc.NumRenderTargets = parameters.m_numRenderTargets;

        for ( size_t colorFormatIndex = 0; colorFormatIndex < parameters.m_colorFormats.size(); ++colorFormatIndex )
        {
            d3d12PipelineStateDesc.RTVFormats[colorFormatIndex] = DXGIFormat( parameters.m_colorFormats[colorFormatIndex] );
        }
        d3d12PipelineStateDesc.DSVFormat = DXGIFormat( parameters.m_depthStencilFormat );

        d3d12PipelineStateDesc.SampleDesc.Count = parameters.m_numSamples;
        d3d12PipelineStateDesc.SampleDesc.Quality = parameters.m_sampleQuality;
        d3d12PipelineStateDesc.SampleMask = UINT_MAX;

        d3d12PipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        d3d12PipelineStateDesc.NodeMask = pD3D12Context->SharedNodeMask();

        HRESULT const result = pD3D12Context->m_device->CreateGraphicsPipelineState( &d3d12PipelineStateDesc, IID_PPV_ARGS( pD3D12Pipeline->m_pipelineState.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12Pipeline, parameters.m_debugName );

        return pD3D12Pipeline;
    }

    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, MeshPipelineParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12PipelineCache* pD3D12PipelineCache = static_cast<Direct3D12PipelineCache*>( parameters.m_pPipelineCache );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pRootSignature );
        Direct3D12Shader*        pD3D12Shader = static_cast<Direct3D12Shader*>( parameters.m_pShader );
        Direct3D12Pipeline*      pD3D12Pipeline = pD3D12Context->CreateObject<Direct3D12Pipeline>();

        EE_ASSERT( pD3D12PipelineCache == nullptr ); // Not implemented yet

        pD3D12Pipeline->m_pipelineType = PipelineType::Graphics;
        pD3D12Pipeline->m_pRootSignature = pD3D12RootSignature;

        D3D12_SHADER_BYTECODE d3d12TaskShaderBytecodeDesc = {};
        if ( pD3D12Shader->m_stages.IsFlagSet( ShaderStage::Task ) )
        {
            IDxcBlobEncoding* pTaskDxcShaderBlob = pD3D12Shader->m_shaderBlobs[pD3D12Shader->m_taskStageIndex].Get();

            d3d12TaskShaderBytecodeDesc.BytecodeLength = pTaskDxcShaderBlob->GetBufferSize();
            d3d12TaskShaderBytecodeDesc.pShaderBytecode = pTaskDxcShaderBlob->GetBufferPointer();
        }

        D3D12_SHADER_BYTECODE d3d12MeshShaderBytecodeDesc = {};
        if ( pD3D12Shader->m_stages.IsFlagSet( ShaderStage::Mesh ) )
        {
            IDxcBlobEncoding* pMeshDxcShaderBlob = pD3D12Shader->m_shaderBlobs[pD3D12Shader->m_meshStageIndex].Get();
            d3d12MeshShaderBytecodeDesc.BytecodeLength = pMeshDxcShaderBlob->GetBufferSize();
            d3d12MeshShaderBytecodeDesc.pShaderBytecode = pMeshDxcShaderBlob->GetBufferPointer();
        }

        D3D12_SHADER_BYTECODE d3d12PixelShaderBytecodeDesc = {};
        if ( pD3D12Shader->m_stages.IsFlagSet( ShaderStage::Pixel ) )
        {
            IDxcBlobEncoding* pDxcShaderBlob = pD3D12Shader->m_shaderBlobs[pD3D12Shader->m_pixelStageIndex].Get();

            d3d12PixelShaderBytecodeDesc.BytecodeLength = pDxcShaderBlob->GetBufferSize();
            d3d12PixelShaderBytecodeDesc.pShaderBytecode = pDxcShaderBlob->GetBufferPointer();
        }

        D3D12_DEPTH_STENCIL_DESC1 d3d12DepthStencilStateDesc = {};

        d3d12DepthStencilStateDesc.DepthEnable = parameters.m_depthStencilState.m_depthTest;
        d3d12DepthStencilStateDesc.DepthFunc = D3D12ComparisonFunc( parameters.m_depthStencilState.m_depthCompareMode );
        if ( parameters.m_depthStencilState.m_depthWrite )
        {
            d3d12DepthStencilStateDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        }
        else
        {
            d3d12DepthStencilStateDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        }

        d3d12DepthStencilStateDesc.StencilEnable = parameters.m_depthStencilState.m_stencilTest;
        d3d12DepthStencilStateDesc.StencilReadMask = parameters.m_depthStencilState.m_stencilReadMask;
        d3d12DepthStencilStateDesc.StencilWriteMask = parameters.m_depthStencilState.m_stencilWriteMask;

        d3d12DepthStencilStateDesc.BackFace.StencilDepthFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_depthBackFail );
        d3d12DepthStencilStateDesc.BackFace.StencilFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilBackFail );
        d3d12DepthStencilStateDesc.BackFace.StencilPassOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilBackPass );
        d3d12DepthStencilStateDesc.BackFace.StencilFunc = D3D12ComparisonFunc( parameters.m_depthStencilState.m_stencilBackCompareMode );

        d3d12DepthStencilStateDesc.FrontFace.StencilDepthFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_depthFrontFail );
        d3d12DepthStencilStateDesc.FrontFace.StencilFailOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilFrontFail );
        d3d12DepthStencilStateDesc.FrontFace.StencilPassOp = D3D12StencilOp( parameters.m_depthStencilState.m_stencilFrontPass );
        d3d12DepthStencilStateDesc.FrontFace.StencilFunc = D3D12ComparisonFunc( parameters.m_depthStencilState.m_stencilFrontCompareMode );

        D3D12_RASTERIZER_DESC d3d12RasterizerStateDesc = {};
        d3d12RasterizerStateDesc.CullMode = D3D12CullMode( parameters.m_rasterizerState.m_cullMode );
        d3d12RasterizerStateDesc.DepthBias = parameters.m_rasterizerState.m_depthBias;
        d3d12RasterizerStateDesc.DepthBiasClamp = parameters.m_rasterizerState.m_depthBiasClamp;
        d3d12RasterizerStateDesc.DepthClipEnable = parameters.m_rasterizerState.m_depthClip;
        d3d12RasterizerStateDesc.FillMode = D3D12FillMode( parameters.m_rasterizerState.m_fillMode );
        d3d12RasterizerStateDesc.FrontCounterClockwise = parameters.m_rasterizerState.m_frontFace == FrontFace::ClockWise;
        d3d12RasterizerStateDesc.MultisampleEnable = parameters.m_rasterizerState.m_multisample;
        d3d12RasterizerStateDesc.SlopeScaledDepthBias = parameters.m_rasterizerState.m_slopeScaledDepthBias;

        D3D12_BLEND_DESC d3d12BlendStateDesc = {};
        d3d12BlendStateDesc.AlphaToCoverageEnable = parameters.m_blendState.m_alphaToCoverage;
        d3d12BlendStateDesc.IndependentBlendEnable = parameters.m_blendState.m_independentBlend;

        for ( uint32_t renderTargetIndex = 0; renderTargetIndex < MaxRenderTargets; ++renderTargetIndex )
        {
            if ( parameters.m_blendState.m_renderTargetMask.IsFlagSet( BlendStateTargetFlags( renderTargetIndex ) ) )
            {
                D3D12_RENDER_TARGET_BLEND_DESC d3d12BlendDesc = {};
                d3d12BlendDesc.BlendEnable = parameters.m_blendState.m_blendEnabled;
                d3d12BlendDesc.SrcBlend = D3D12Blend( parameters.m_blendState.m_srcFactors[renderTargetIndex] );
                d3d12BlendDesc.DestBlend = D3D12Blend( parameters.m_blendState.m_dstFactors[renderTargetIndex] );
                d3d12BlendDesc.BlendOp = D3D12BlendOp( parameters.m_blendState.m_blendModes[renderTargetIndex] );
                d3d12BlendDesc.SrcBlendAlpha = D3D12Blend( parameters.m_blendState.m_srcAlphaFactors[renderTargetIndex] );
                d3d12BlendDesc.DestBlendAlpha = D3D12Blend( parameters.m_blendState.m_dstAlphaFactors[renderTargetIndex] );
                d3d12BlendDesc.BlendOpAlpha = D3D12BlendOp( parameters.m_blendState.m_blendModesAlpha[renderTargetIndex] );
                d3d12BlendDesc.RenderTargetWriteMask = parameters.m_blendState.m_writeMasks[renderTargetIndex];

                d3d12BlendStateDesc.RenderTarget[renderTargetIndex] = d3d12BlendDesc;
            }
        }

        D3D12_RT_FORMAT_ARRAY d3d12RTVFormatsDesc = {};
        d3d12RTVFormatsDesc.NumRenderTargets = parameters.m_numRenderTargets;

        for ( size_t colorFormatIndex = 0; colorFormatIndex < parameters.m_colorFormats.size(); ++colorFormatIndex )
        {
            d3d12RTVFormatsDesc.RTFormats[colorFormatIndex] = DXGIFormat( parameters.m_colorFormats[colorFormatIndex] );
        }

        DXGI_SAMPLE_DESC dxgiSampleDesc = { parameters.m_numSamples, parameters.m_sampleQuality };

        D3D12MeshShaderPipelineStream d3d12PipelineStream = {};
        d3d12PipelineStream.m_pRootSignature = pD3D12RootSignature->m_rootSignature.Get();
        d3d12PipelineStream.m_AS = d3d12TaskShaderBytecodeDesc;
        d3d12PipelineStream.m_MS = d3d12MeshShaderBytecodeDesc;
        d3d12PipelineStream.m_PS = d3d12PixelShaderBytecodeDesc;
        d3d12PipelineStream.m_DepthStencilState = d3d12DepthStencilStateDesc;
        d3d12PipelineStream.m_RasterizerState = d3d12RasterizerStateDesc;
        d3d12PipelineStream.m_BlendState = d3d12BlendStateDesc;
        d3d12PipelineStream.m_RTVFormats = d3d12RTVFormatsDesc;
        d3d12PipelineStream.m_DSVFormat = DXGIFormat( parameters.m_depthStencilFormat );
        d3d12PipelineStream.m_SampleDesc = dxgiSampleDesc;
        d3d12PipelineStream.m_SampleMask = UINT_MAX;

        d3d12PipelineStream.m_PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        d3d12PipelineStream.m_NodeMask = pD3D12Context->SharedNodeMask();

        D3D12_PIPELINE_STATE_STREAM_DESC d3d12PipelineStreamDesc = {};
        d3d12PipelineStreamDesc.SizeInBytes = sizeof( D3D12MeshShaderPipelineStream );
        d3d12PipelineStreamDesc.pPipelineStateSubobjectStream = &d3d12PipelineStream;

        ID3D12Device2* pD3D12Device2 = static_cast<ID3D12Device2*>( pD3D12Context->m_device.Get() );

        HRESULT const result = pD3D12Device2->CreatePipelineState( &d3d12PipelineStreamDesc, IID_PPV_ARGS( pD3D12Pipeline->m_pipelineState.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12Pipeline, parameters.m_debugName );

        return pD3D12Pipeline;
    }

    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, ComputePipelineParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12PipelineCache* pD3D12PipelineCache = static_cast<Direct3D12PipelineCache*>( parameters.m_pPipelineCache );
        Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pRootSignature );
        Direct3D12Shader*        pD3D12Shader = static_cast<Direct3D12Shader*>( parameters.m_pShader );
        Direct3D12Pipeline*      pD3D12Pipeline = pD3D12Context->CreateObject<Direct3D12Pipeline>();

        EE_ASSERT( pD3D12PipelineCache == nullptr ); // Not implemented yet

        pD3D12Pipeline->m_pipelineType = PipelineType::Compute;
        pD3D12Pipeline->m_pRootSignature = pD3D12RootSignature;

        EE_ASSERT( parameters.m_pShader->m_stages.IsFlagSet( ShaderStage::Compute ) );

        D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12PipelineDesc = {};
        d3d12PipelineDesc.pRootSignature = pD3D12RootSignature->m_rootSignature.Get();

        IDxcBlobEncoding* pD3D12ShaderBlob = pD3D12Shader->m_shaderBlobs[pD3D12Shader->m_computeStageIndex].Get();
        d3d12PipelineDesc.CS.BytecodeLength = pD3D12ShaderBlob->GetBufferSize();
        d3d12PipelineDesc.CS.pShaderBytecode = pD3D12ShaderBlob->GetBufferPointer();

        d3d12PipelineDesc.NodeMask = pD3D12Context->SharedNodeMask();

        HRESULT const result = pD3D12Context->m_device->CreateComputePipelineState( &d3d12PipelineDesc, IID_PPV_ARGS( pD3D12Pipeline->m_pipelineState.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12Pipeline, parameters.m_debugName );

        return pD3D12Pipeline;
    }

    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, RaytracingPipelineParameters const& parameters )
    {
        Direct3D12Context*       pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12PipelineCache* pD3D12PipelineCache = static_cast<Direct3D12PipelineCache*>( parameters.m_pPipelineCache );
        Direct3D12RootSignature* pD3D12GlobalRootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pGlobalRootSignature );
        Direct3D12RootSignature* pD3D12EmptyRootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pEmptyRootSignature );
        Direct3D12RootSignature* pD3D12RayGenRootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_pRayGenRootSignature );
        Direct3D12Shader*        pD3D12RayGenShader = static_cast<Direct3D12Shader*>( parameters.m_pRayGenShader );
        Direct3D12Pipeline*      pD3D12Pipeline = pD3D12Context->CreateObject<Direct3D12Pipeline>();

        EE_ASSERT( pD3D12PipelineCache == nullptr ); // Not implemented yet

        TArray<wchar_t, MaxEntryPointNameLength> rayGenEntryPointName = {};
        FormatEntryPointName( parameters.m_rayGenEntryPoint, rayGenEntryPointName );

        size_t numLibraries = 0;
        numLibraries += parameters.m_rayMissShaders.size();
        numLibraries += parameters.m_rayMissRootSignatures.size();
        numLibraries += parameters.m_hitGroups.size() * 3; // 3 shaders per hit group
        numLibraries += parameters.m_hitGroups.size();     // 1 local root signature per hit group
        numLibraries += 1;                                 // ray gen root signature
        numLibraries += 1;                                 // global root signature
        numLibraries += 1;                                 // pipeline config
        numLibraries += 1;                                 // shader config

        size_t numLocalRootSignatures = 1 + parameters.m_rayMissRootSignatures.size() + parameters.m_hitGroups.size();
        size_t numStateSubobjects = numLibraries + numLocalRootSignatures * 2;

        // Fixed capacity vectors below contain pointers to each others elements (thanks D3D!).
        // We calculate their maximum size upfront and use fixed allocators to make sure they never reallocate and have stable pointers.
        // If they try to reallocate this will fail with "out of memory" error, meaning that you need to adjust the calculation above.

        eastl::fixed_allocator                                                          d3d12Libraries_Allocator = {};
        TUniquePtr<D3D12_DXIL_LIBRARY_DESC[]>                                           d3d12Libraries_Memory = {};
        eastl::vector<D3D12_DXIL_LIBRARY_DESC, eastl::fixed_allocator>                  d3d12Libraries = {};
        InitializeFixedCapacityVector( d3d12Libraries_Memory, d3d12Libraries_Allocator, d3d12Libraries, numLibraries );

        eastl::fixed_allocator                                                          d3d12StateSubobjects_Allocator = {};
        TUniquePtr<D3D12_STATE_SUBOBJECT[]>                                             d3d12StateSubobjects_Memory = {};
        eastl::vector<D3D12_STATE_SUBOBJECT, eastl::fixed_allocator>                    d3d12StateSubobjects = {};
        InitializeFixedCapacityVector( d3d12StateSubobjects_Memory, d3d12StateSubobjects_Allocator, d3d12StateSubobjects, numStateSubobjects );

        eastl::fixed_allocator                                                          d3d12SubobjectAssociations_Allocator = {};
        TUniquePtr<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION[]>                            d3d12SubobjectsAssociations_Memory = {};
        eastl::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION, eastl::fixed_allocator>   d3d12SubobjectAssociations = {};
        InitializeFixedCapacityVector( d3d12SubobjectsAssociations_Memory, d3d12SubobjectAssociations_Allocator, d3d12SubobjectAssociations, numStateSubobjects );

        eastl::fixed_allocator                                                          d3d12LocalRootSignatures_Allocator = {};
        TUniquePtr<D3D12_LOCAL_ROOT_SIGNATURE[]>                                        d3d12LocalRootSignatures_Memory = {};
        eastl::vector<D3D12_LOCAL_ROOT_SIGNATURE, eastl::fixed_allocator>               d3d12LocalRootSignatures = {};
        InitializeFixedCapacityVector( d3d12LocalRootSignatures_Memory, d3d12LocalRootSignatures_Allocator, d3d12LocalRootSignatures, numLocalRootSignatures );

        D3D12_EXPORT_DESC d3d12RayGenExportDesc = {};
        d3d12RayGenExportDesc.Name = rayGenEntryPointName.data();

        D3D12_DXIL_LIBRARY_DESC d3d12RayGenLibraryDesc = {};
        d3d12RayGenLibraryDesc.NumExports = 1;
        d3d12RayGenLibraryDesc.pExports = &d3d12RayGenExportDesc;
        d3d12RayGenLibraryDesc.DXILLibrary.BytecodeLength = pD3D12RayGenShader->m_shaderBlobs[0]->GetBufferSize();
        d3d12RayGenLibraryDesc.DXILLibrary.pShaderBytecode = pD3D12RayGenShader->m_shaderBlobs[0]->GetBufferPointer();

        d3d12Libraries.push_back( d3d12RayGenLibraryDesc );

        if ( pD3D12RayGenRootSignature )
        {
            D3D12_LOCAL_ROOT_SIGNATURE d3d12LocalRootSignatureDesc = {};
            d3d12LocalRootSignatureDesc.pLocalRootSignature = pD3D12RayGenRootSignature->m_rootSignature.Get();
            d3d12LocalRootSignatures.push_back( d3d12LocalRootSignatureDesc );

            D3D12_STATE_SUBOBJECT d3d12StateSubobject = {};
            d3d12StateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            d3d12StateSubobject.pDesc = &d3d12LocalRootSignatures.back();
            d3d12StateSubobjects.push_back( d3d12StateSubobject );

            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION d3d12SubobjectAssociation = {};
            d3d12SubobjectAssociation.pSubobjectToAssociate = &d3d12StateSubobjects.back();
            d3d12SubobjectAssociation.NumExports = 1;
            d3d12SubobjectAssociation.pExports = &d3d12RayGenExportDesc.Name;
            d3d12SubobjectAssociations.push_back( d3d12SubobjectAssociation );

            D3D12_STATE_SUBOBJECT d3d12AssociationSubobject = {};
            d3d12AssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            d3d12AssociationSubobject.pDesc = &d3d12SubobjectAssociations.back();
        }

        TVector<TArray<wchar_t, MaxEntryPointNameLength>> rayMissEntryPointNames{ Memory::Allocators::g_RHI };
        rayMissEntryPointNames.resize( parameters.m_rayMissShaders.size() );

        TVector<D3D12_EXPORT_DESC> d3d12RayMissExports{ Memory::Allocators::g_RHI };
        d3d12RayMissExports.resize( parameters.m_rayMissShaders.size() + parameters.m_hitGroups.size() );

        for ( size_t rayMissShaderIndex = 0; rayMissShaderIndex < parameters.m_rayMissShaders.size(); ++rayMissShaderIndex )
        {
            Direct3D12Shader* pD3D12RayMissShader = static_cast<Direct3D12Shader*>( parameters.m_rayMissShaders[rayMissShaderIndex] );

            FormatEntryPointName( parameters.m_rayMissEntryPoints[rayMissShaderIndex], rayMissEntryPointNames[rayMissShaderIndex] );

            D3D12_EXPORT_DESC d3d12Export = {};
            d3d12Export.Name = rayMissEntryPointNames[rayMissShaderIndex].data();
            d3d12RayMissExports[rayMissShaderIndex] = d3d12Export;

            D3D12_DXIL_LIBRARY_DESC d3d12RayMissLibraryDesc = {};
            d3d12RayMissLibraryDesc.NumExports = 1;
            d3d12RayMissLibraryDesc.pExports = &d3d12RayMissExports[rayMissShaderIndex];
            d3d12RayMissLibraryDesc.DXILLibrary.BytecodeLength = pD3D12RayMissShader->m_shaderBlobs[0]->GetBufferSize();
            d3d12RayMissLibraryDesc.DXILLibrary.pShaderBytecode = pD3D12RayMissShader->m_shaderBlobs[0]->GetBufferPointer();
            d3d12Libraries.push_back( d3d12RayMissLibraryDesc );

            if ( !parameters.m_rayMissRootSignatures.empty() && parameters.m_rayMissRootSignatures[rayMissShaderIndex] )
            {
                Direct3D12RootSignature* pD3D12RayMissRootSignature = static_cast<Direct3D12RootSignature*>( parameters.m_rayMissRootSignatures[rayMissShaderIndex] );

                D3D12_LOCAL_ROOT_SIGNATURE d3d12LocalRootSignatureDesc = {};
                d3d12LocalRootSignatureDesc.pLocalRootSignature = pD3D12RayMissRootSignature->m_rootSignature.Get();
                d3d12LocalRootSignatures.push_back( d3d12LocalRootSignatureDesc );

                D3D12_STATE_SUBOBJECT d3d12StateSubobject = {};
                d3d12StateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
                d3d12StateSubobject.pDesc = &d3d12LocalRootSignatures.back();
                d3d12StateSubobjects.push_back( d3d12StateSubobject );

                D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION d3d12SubobjectAssociation = {};
                d3d12SubobjectAssociation.pSubobjectToAssociate = &d3d12StateSubobjects.back();
                d3d12SubobjectAssociation.NumExports = 1;
                d3d12SubobjectAssociation.pExports = &d3d12RayMissExports[rayMissShaderIndex].Name;
                d3d12SubobjectAssociations.push_back( d3d12SubobjectAssociation );

                D3D12_STATE_SUBOBJECT d3d12AssociationSubobject = {};
                d3d12AssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
                d3d12AssociationSubobject.pDesc = &d3d12SubobjectAssociations.back();
            }
        }

        TVector<D3D12_HIT_GROUP_DESC> d3d12HitGroups{ Memory::Allocators::g_RHI };
        d3d12HitGroups.resize( parameters.m_hitGroups.size() );

        TVector<TArray<wchar_t, MaxEntryPointNameLength>> hitGroupNames{ Memory::Allocators::g_RHI };
        hitGroupNames.resize( parameters.m_hitGroups.size() );

        TVector<TArray<wchar_t, MaxEntryPointNameLength>> intersectionEntryNames{ Memory::Allocators::g_RHI };
        intersectionEntryNames.resize( parameters.m_hitGroups.size() );

        TVector<TArray<wchar_t, MaxEntryPointNameLength>> anyHitEntryNames{ Memory::Allocators::g_RHI };
        anyHitEntryNames.resize( parameters.m_hitGroups.size() );

        TVector<TArray<wchar_t, MaxEntryPointNameLength>> closestHitEntryNames{ Memory::Allocators::g_RHI };
        closestHitEntryNames.resize( parameters.m_hitGroups.size() );

        TVector<D3D12_EXPORT_DESC> d3d12IntersectionExports{ Memory::Allocators::g_RHI };
        d3d12IntersectionExports.resize( parameters.m_hitGroups.size() );

        TVector<D3D12_EXPORT_DESC> d3d12AnyHitExports{ Memory::Allocators::g_RHI };
        d3d12AnyHitExports.resize( parameters.m_hitGroups.size() );

        TVector<D3D12_EXPORT_DESC> d3d12ClosestHitExports{ Memory::Allocators::g_RHI };
        d3d12ClosestHitExports.resize( parameters.m_hitGroups.size() );

        for ( size_t hitGroupIndex = 0; hitGroupIndex < parameters.m_hitGroups.size(); ++hitGroupIndex )
        {
            RaytracingHitGroup const& hitGroup = parameters.m_hitGroups[hitGroupIndex];

            FormatEntryPointName( hitGroup.m_hitGroupName, hitGroupNames[hitGroupIndex] );

            D3D12_HIT_GROUP_DESC d3d12HitGroup = {};
            d3d12HitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            d3d12HitGroup.HitGroupExport = hitGroupNames[hitGroupIndex].data();

            if ( hitGroup.m_pIntersectionShader )
            {
                Direct3D12Shader* pD3D12IntersectionShader = static_cast<Direct3D12Shader*>( hitGroup.m_pIntersectionShader );

                FormatEntryPointName( hitGroup.m_intersectionEntryPoint, intersectionEntryNames[hitGroupIndex] );

                d3d12HitGroup.IntersectionShaderImport = intersectionEntryNames[hitGroupIndex].data();

                D3D12_EXPORT_DESC d3d12IntersectionExportDesc = {};
                d3d12IntersectionExportDesc.Name = intersectionEntryNames[hitGroupIndex].data();

                d3d12IntersectionExports[hitGroupIndex] = d3d12IntersectionExportDesc;

                D3D12_DXIL_LIBRARY_DESC d3d12LibraryDesc = {};
                d3d12LibraryDesc.NumExports = 1;
                d3d12LibraryDesc.pExports = &d3d12IntersectionExports[hitGroupIndex];
                d3d12LibraryDesc.DXILLibrary.BytecodeLength = pD3D12IntersectionShader->m_shaderBlobs[0]->GetBufferSize();
                d3d12LibraryDesc.DXILLibrary.pShaderBytecode = pD3D12IntersectionShader->m_shaderBlobs[0]->GetBufferPointer();

                d3d12Libraries.push_back( d3d12LibraryDesc );
            }

            if ( hitGroup.m_pAnyHitShader )
            {
                Direct3D12Shader* pD3D12AnyHitShader = static_cast<Direct3D12Shader*>( hitGroup.m_pAnyHitShader );

                FormatEntryPointName( hitGroup.m_anyHitEntryPoint, anyHitEntryNames[hitGroupIndex] );

                d3d12HitGroup.AnyHitShaderImport = anyHitEntryNames[hitGroupIndex].data();

                D3D12_EXPORT_DESC d3d12AnyHitExportDesc = {};
                d3d12AnyHitExportDesc.Name = anyHitEntryNames[hitGroupIndex].data();

                d3d12AnyHitExports[hitGroupIndex] = d3d12AnyHitExportDesc;

                D3D12_DXIL_LIBRARY_DESC d3d12LibraryDesc = {};
                d3d12LibraryDesc.NumExports = 1;
                d3d12LibraryDesc.pExports = &d3d12AnyHitExports[hitGroupIndex];
                d3d12LibraryDesc.DXILLibrary.BytecodeLength = pD3D12AnyHitShader->m_shaderBlobs[0]->GetBufferSize();
                d3d12LibraryDesc.DXILLibrary.pShaderBytecode = pD3D12AnyHitShader->m_shaderBlobs[0]->GetBufferPointer();

                d3d12Libraries.push_back( d3d12LibraryDesc );
            }

            if ( hitGroup.m_pClosestHitShader )
            {
                Direct3D12Shader* pD3D12ClosestHitShader = static_cast<Direct3D12Shader*>( hitGroup.m_pClosestHitShader );

                FormatEntryPointName( hitGroup.m_closestHitEntryPoint, closestHitEntryNames[hitGroupIndex] );

                d3d12HitGroup.ClosestHitShaderImport = closestHitEntryNames[hitGroupIndex].data();

                D3D12_EXPORT_DESC d3d12Export = {};
                d3d12Export.Name = closestHitEntryNames[hitGroupIndex].data();
                d3d12ClosestHitExports[hitGroupIndex] = d3d12Export;

                D3D12_DXIL_LIBRARY_DESC d3d12LibraryDesc = {};
                d3d12LibraryDesc.NumExports = 1;
                d3d12LibraryDesc.pExports = &d3d12ClosestHitExports[hitGroupIndex];
                d3d12LibraryDesc.DXILLibrary.BytecodeLength = pD3D12ClosestHitShader->m_shaderBlobs[0]->GetBufferSize();
                d3d12LibraryDesc.DXILLibrary.pShaderBytecode = pD3D12ClosestHitShader->m_shaderBlobs[0]->GetBufferPointer();
                d3d12Libraries.push_back( d3d12LibraryDesc );
            }

            d3d12HitGroups[hitGroupIndex] = d3d12HitGroup;

            Direct3D12RootSignature* pD3D12HitGroupRootSignature = pD3D12EmptyRootSignature;
            if ( hitGroup.m_pRootSignature )
            {
                pD3D12HitGroupRootSignature = static_cast<Direct3D12RootSignature*>( hitGroup.m_pRootSignature );
            }

            if ( pD3D12HitGroupRootSignature )
            {
                D3D12_LOCAL_ROOT_SIGNATURE d3d12LocalRootSignature = {};
                d3d12LocalRootSignature.pLocalRootSignature = pD3D12HitGroupRootSignature->m_rootSignature.Get();
                d3d12LocalRootSignatures.push_back( d3d12LocalRootSignature );

                D3D12_STATE_SUBOBJECT d3d12StateSubobject = {};
                d3d12StateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
                d3d12StateSubobject.pDesc = &d3d12LocalRootSignatures.back();
                d3d12StateSubobjects.push_back( d3d12StateSubobject );

                D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION d3d12SubobjectAssociation = {};
                d3d12SubobjectAssociation.pSubobjectToAssociate = &d3d12StateSubobjects.back();
                d3d12SubobjectAssociation.NumExports = 1;
                d3d12SubobjectAssociation.pExports = &d3d12HitGroups[hitGroupIndex].HitGroupExport;
                d3d12SubobjectAssociations.push_back( d3d12SubobjectAssociation );

                D3D12_STATE_SUBOBJECT d3d12AssociationSubobject = {};
                d3d12AssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
                d3d12AssociationSubobject.pDesc = &d3d12SubobjectAssociations.back();
            }

            D3D12_STATE_SUBOBJECT d3d12StateSubobject = {};
            d3d12StateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            d3d12StateSubobject.pDesc = &d3d12HitGroups[hitGroupIndex];
            d3d12StateSubobjects.push_back( d3d12StateSubobject );
        }

        for ( D3D12_DXIL_LIBRARY_DESC const& d3d12Library : d3d12Libraries )
        {
            D3D12_STATE_SUBOBJECT d3d12StateSubobject = {};
            d3d12StateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            d3d12StateSubobject.pDesc = &d3d12Library;
            d3d12StateSubobjects.push_back( d3d12StateSubobject );
        }

        D3D12_RAYTRACING_PIPELINE_CONFIG d3d12RaytracingPipelineConfig = {};
        d3d12RaytracingPipelineConfig.MaxTraceRecursionDepth = parameters.m_maxTraceRecursionDepth;

        D3D12_STATE_SUBOBJECT d3d12RaytracingPipelineConfigSubobject = {};
        d3d12RaytracingPipelineConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        d3d12RaytracingPipelineConfigSubobject.pDesc = &d3d12RaytracingPipelineConfig;
        d3d12StateSubobjects.push_back( d3d12RaytracingPipelineConfigSubobject );

        D3D12_RAYTRACING_SHADER_CONFIG d3d12RaytracingShaderConfig = {};
        d3d12RaytracingShaderConfig.MaxAttributeSizeInBytes = parameters.m_attributeSize;
        d3d12RaytracingShaderConfig.MaxPayloadSizeInBytes = parameters.m_payloadSize;

        D3D12_STATE_SUBOBJECT d3d12RaytracingShaderConfigSubobject = {};
        d3d12RaytracingShaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        d3d12RaytracingShaderConfigSubobject.pDesc = &d3d12RaytracingShaderConfig;
        d3d12StateSubobjects.push_back( d3d12RaytracingShaderConfigSubobject );

        if ( pD3D12GlobalRootSignature )
        {
            D3D12_STATE_SUBOBJECT d3d12StateSubobject = {};
            d3d12StateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            d3d12StateSubobject.pDesc = pD3D12GlobalRootSignature->m_rootSignature.Get();
            d3d12StateSubobjects.push_back( d3d12StateSubobject );
        }

        D3D12_STATE_OBJECT_DESC d3d12StateObjectDesc = {};
        d3d12StateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        d3d12StateObjectDesc.NumSubobjects = UINT( d3d12StateSubobjects.size() );

        ID3D12Device5* pD3D12Device5 = static_cast<ID3D12Device5*>( pD3D12Context->m_device.Get() );

        HRESULT const result = pD3D12Device5->CreateStateObject( &d3d12StateObjectDesc, IID_PPV_ARGS( pD3D12Pipeline->m_raytracingState.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12Pipeline, parameters.m_debugName );

        return pD3D12Pipeline;
    }

    EE_BASE_API void DestroyPipeline( Context* pContext, Pipeline*&& pPipeline )
    {
        if ( pPipeline )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12Pipeline* pD3D12Pipeline = static_cast<Direct3D12Pipeline*>( pPipeline );

            pD3D12Context->DestroyObject( eastl::move( pD3D12Pipeline ) );
            pPipeline = nullptr;
        }
    }

    EE_BASE_API QueryPool* CreateQueryPool( Context* pContext, QueryPoolParameters const& parameters )
    {
        Direct3D12Context*   pD3D12Context = static_cast<Direct3D12Context*>( pContext );
        Direct3D12QueryPool* pD3D12QueryPool = pD3D12Context->CreateObject<Direct3D12QueryPool>();

        pD3D12QueryPool->m_queryType = D3D12QueryType( parameters.m_queryType );
        pD3D12QueryPool->m_numQueries = parameters.m_numQueries;

        D3D12_QUERY_HEAP_DESC d3d12QueryHeapDesc = {};
        d3d12QueryHeapDesc.Count = parameters.m_numQueries;
        d3d12QueryHeapDesc.NodeMask = pD3D12Context->NodeMask( parameters.m_nodeIndex );
        d3d12QueryHeapDesc.Type = D3D12QueryHeapType( parameters.m_queryType );

        HRESULT const result = pD3D12Context->m_device->CreateQueryHeap( &d3d12QueryHeapDesc, IID_PPV_ARGS( pD3D12QueryPool->m_queryHeap.ReleaseAndGetAddressOf() ) );
        EE_ASSERT( SUCCEEDED( result ) );

        SetDebugName( pD3D12Context, pD3D12QueryPool, parameters.m_debugName );

        return pD3D12QueryPool;
    }

    EE_BASE_API void DestroyQueryPool( Context* pContext, QueryPool*&& pQueryPool )
    {
        if ( pQueryPool )
        {
            Direct3D12Context* pD3D12Context = static_cast<Direct3D12Context*>( pContext );
            Direct3D12QueryPool* pD3D12QueryPool = static_cast<Direct3D12QueryPool*>( pQueryPool );

            pD3D12Context->DestroyObject( eastl::move( pD3D12QueryPool ) );
            pQueryPool = nullptr;
        }
    }

    EE_BASE_API double GetQueryTimestampFrequency( Queue* pQueue )
    {
        Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );

        UINT64 result = 0;
        HRESULT const hr = pD3D12Queue->m_queue->GetTimestampFrequency( &result );
        EE_ASSERT( SUCCEEDED( hr ) );

        return double( result );
    }

    EE_BASE_API void SetDebugName( Context* pContext, Buffer* pBuffer, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12Buffer* pD3D12Buffer = static_cast<Direct3D12Buffer*>( pBuffer );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            if ( pD3D12Buffer->m_allocation )
            {
                pD3D12Buffer->m_allocation->SetName( debugNameW );
            }
            HRESULT const result = pD3D12Buffer->m_resource->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, Texture* pTexture, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12Texture* pD3D12Texture = static_cast<Direct3D12Texture*>( pTexture );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            if ( pD3D12Texture->m_allocation )
            {
                pD3D12Texture->m_allocation->SetName( debugNameW );
            }
            HRESULT const result = pD3D12Texture->m_resource->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, Queue* pQueue, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12Queue* pD3D12Queue = static_cast<Direct3D12Queue*>( pQueue );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12Queue->m_queue->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, QueryPool* pQueryPool, StringView const debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12QueryPool* pD3D12QueryPool = static_cast<Direct3D12QueryPool*>( pQueryPool );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12QueryPool->m_queryHeap->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, RootSignature* pRootSignature, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12RootSignature* pD3D12RootSignature = static_cast<Direct3D12RootSignature*>( pRootSignature );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12RootSignature->m_rootSignature->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, CommandSignature* pCommandSignature, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12CommandSignature* pD3D12CommandSignature = static_cast<Direct3D12CommandSignature*>( pCommandSignature );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12CommandSignature->m_pCommandSignature->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, Pipeline* pPipeline, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12Pipeline* pD3D12Pipeline = static_cast<Direct3D12Pipeline*>( pPipeline );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12Pipeline->m_pipelineState->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, CommandPool* pCommandPool, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12CommandPool* pD3D12CommandPool = static_cast<Direct3D12CommandPool*>( pCommandPool );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12CommandPool->m_commandAllocator->SetName( debugNameW );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void SetDebugName( Context* pContext, CommandBuffer* pCommandBuffer, StringView debugName )
    {
        EE_ASSERT( !debugName.empty() );
        if ( !debugName.empty() )
        {
            Direct3D12CommandBuffer* pD3D12CommandBuffer = static_cast<Direct3D12CommandBuffer*>( pCommandBuffer );

            wchar_t       debugNameW[Limits::MaxDebugNameLength] = {};
            size_t        debugNameLen = 0;
            errno_t const error = mbstowcs_s<Limits::MaxDebugNameLength>(
                &debugNameLen,
                debugNameW,
                debugName.data(),
                debugName.size() );
            EE_ASSERT( error == 0 );

            HRESULT const result = pD3D12CommandBuffer->m_commandList->SetName( ( debugNameW ) );
            EE_ASSERT( SUCCEEDED( result ) );
        }
    }

    EE_BASE_API void ReportDeviceMemoryLeaks()
    {
        if ( ComPtr<IDXGIDebug> dxgiDebug; SUCCEEDED( DXGIGetDebugInterface1( 0, IID_PPV_ARGS( dxgiDebug.ReleaseAndGetAddressOf() ) ) ) )
        {
            HRESULT const result = dxgiDebug->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS( DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL ) );
            EE_ASSERT( SUCCEEDED( result ) );
        }
        else
        {
            EE_LOG_MESSAGE( LogCategory::Render, "RHI/ReportLiveObjects", "Not a debug device, no live objects are reported" );
        }
    }
}

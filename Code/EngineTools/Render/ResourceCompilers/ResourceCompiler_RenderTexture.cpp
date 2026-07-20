#include "ResourceCompiler_RenderTexture.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Import/Importer.h"
#include "EngineTools/Import/ImportedImage.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Render/RHI.h"
#include "Base/Math/Math.h"
#include "Base/Time/Timers.h"

#include "Base/ThirdParty/stb/stb_image_resize2.h"
#include "Base/ThirdParty/stb/stb_image_write.h"

#include <ctt.h>

struct SurfaceRGBA
{
    uint8_t*    m_pSurfaceData = nullptr;
    int32_t     m_width = 0;
    int32_t     m_height = 0;
    int32_t     m_strideInBytes = 0;
};

//-------------------------------------------------------------------------

namespace EE::Render
{
    template<typename T>
    void SwizzleChannels( SurfaceRGBA& sourceSurface, uint8_t const* pSourceImageData, TArrayView<size_t> srcOffsets, TArrayView<size_t> dstOffsets )
    {
        struct PixelType
        {
            T m_channels[4];
        };

        for ( int32_t y = 0; y < sourceSurface.m_height; ++y )
        {
            for ( int32_t x = 0; x < sourceSurface.m_width; ++x )
            {
                size_t           baseOffset = ( y * sourceSurface.m_width + x );
                PixelType*       pDstMemory = reinterpret_cast<PixelType*>( sourceSurface.m_pSurfaceData ) + baseOffset;
                PixelType const* pSrcMemory = reinterpret_cast<PixelType const*>( pSourceImageData ) + baseOffset;

                for ( size_t channelIndex = 0; channelIndex < srcOffsets.size(); ++channelIndex )
                {
                    pDstMemory->m_channels[dstOffsets[channelIndex]] = pSrcMemory->m_channels[srcOffsets[channelIndex]];
                }
            }
        }
    }

    template <typename T>
    void InvertNormalMap( SurfaceRGBA& sourceSurface, T maxValue )
    {
        struct PixelType
        {
            T m_channels[4];
        };

        for ( int32_t y = 0; y < sourceSurface.m_height; ++y )
        {
            for ( int32_t x = 0; x < sourceSurface.m_width; ++x )
            {
                size_t           baseOffset = ( y * sourceSurface.m_width + x );
                PixelType*       pDstMemory = reinterpret_cast<PixelType*>( sourceSurface.m_pSurfaceData ) + baseOffset;

                pDstMemory->m_channels[1] = maxValue - pDstMemory->m_channels[1];
            }
        }
    }

    TextureCompiler::TextureCompiler()
        : Resource::Compiler( "TextureCompiler" )
    {
        RegisterOutput<TextureResource>();
    }

    Resource::CompilationResult TextureCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        if ( ctx.m_resourceID.GetResourceTypeID() == TextureResource::GetStaticResourceTypeID() )
        {
            return CompileTexture( ctx );
        }
        return Resource::CompilationResult::Failure;
    }

    Resource::CompilationResult TextureCompiler::CompileTexture( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<TextureResourceDescriptor>();
        EE_ASSERT( pResourceDescriptor != nullptr );

        auto pTextureGroup = ctx.GetDataFile<TextureGroup>( pResourceDescriptor->m_textureGroup );
        EE_ASSERT( pTextureGroup != nullptr );

        // Swizzle texture
        //-------------------------------------------------------------------------

        Import::ReaderContext readerCtx =
        {
            [&ctx]( char const* pString ) { ctx.LogWarning( pString ); },
            [&ctx] ( char const* pString ) { ctx.LogError( pString ); },
        };

        TVector<TUniquePtr<uint8_t[]>>  sourceImageDatas;
        TVector<SurfaceRGBA>            sourceSurfaces;

        bool                            isHDR = false;
        bool                            isSharedExponent = false;

        if ( pResourceDescriptor->m_sourcePaths.size() != pTextureGroup->m_fileMasks.size() )
        {
            return ctx.LogError( "Amount of input textures does not match file masks in the group %s", ctx.GetInputPath().c_str() );
        }

        for ( size_t textureIndex = 0; textureIndex < pResourceDescriptor->m_sourcePaths.size(); ++textureIndex )
        {
            DataPath const&             sourceTexturePath = pResourceDescriptor->m_sourcePaths[textureIndex];
            TextureGroupFileMask const& fileMask = pTextureGroup->m_fileMasks[textureIndex];

            if ( sourceTexturePath.IsValid() && fileMask.m_channelName.empty() )
            {
                return ctx.LogError( "Invalid file mask for %s", sourceTexturePath.c_str() );
            }

            if ( !sourceTexturePath.IsValid() )
            {
                continue;
            }

            FileSystem::Path const textureFilePath = sourceTexturePath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );
            Import::Source const fileSource( textureFilePath, ctx.GetRawData( sourceTexturePath ) );
            TUniquePtr<Import::Image> importedImage = Import::Importer::ReadImage( readerCtx, fileSource );

            int32_t numDepthSlices = fileMask.m_depthSlice + importedImage->GetDepth();
            if ( numDepthSlices >= sourceSurfaces.size() )
            {
                sourceSurfaces.resize( numDepthSlices );
                sourceImageDatas.resize( numDepthSlices );
            }

            if ( ( ( importedImage->GetWidth() % 4 ) != 0 ) || ( ( importedImage->GetHeight() % 4 ) != 0 ) )
            {
                return ctx.LogError( "Texture dimensions should be a multiple of 4 for %s", sourceTexturePath.c_str() );
            }

            uint8_t const* pImageData = importedImage->GetImageData();

            for ( int32_t depthSlice = 0; depthSlice < importedImage->GetDepth(); ++depthSlice )
            {
                SurfaceRGBA& sourceSurface = sourceSurfaces[fileMask.m_depthSlice + depthSlice];
                TUniquePtr<uint8_t[]>& sourceImageData = sourceImageDatas[fileMask.m_depthSlice + depthSlice];

                if ( !sourceImageData )
                {
                    sourceSurface.m_width = importedImage->GetWidth();
                    sourceSurface.m_height = importedImage->GetHeight();
                    sourceSurface.m_strideInBytes = importedImage->GetStride();

                    sourceImageData.reset( EE::NewArray<uint8_t>( sourceSurface.m_height * importedImage->GetStride() ) );
                    sourceSurface.m_pSurfaceData = sourceImageData.get();

                    isHDR = importedImage->IsHDR();
                    isSharedExponent = importedImage->IsSharedExponent();

                    // Validate that HDR images are in the correct texture group
                    if ( pTextureGroup->m_compressionSettings.m_textureType == TextureGroupCompressionSettings::TextureType::FloatingPoint3Channel )
                    {
                        if ( !isHDR )
                        {
                            return ctx.LogError( "Image is not an HDR image: %s", textureFilePath.c_str() );
                        }
                    }
                    else
                    {
                        if ( isHDR )
                        {
                            return ctx.LogError( "Image is an HDR image: %s", textureFilePath.c_str() );
                        }
                    }
                }
                else
                {
                    if ( sourceSurface.m_width != importedImage->GetWidth() || sourceSurface.m_height != importedImage->GetHeight() || sourceSurface.m_strideInBytes != importedImage->GetStride() || isHDR != importedImage->IsHDR() )
                    {
                        stbir_pixel_layout pixelLayout = STBIR_RGBA;
                        uint8_t* pNewImageData = static_cast<uint8_t*>( EE::Alloc( sourceSurface.m_height * sourceSurface.m_strideInBytes ) );

                        switch ( pTextureGroup->m_compressionSettings.m_textureType )
                        {
                            case TextureGroupCompressionSettings::TextureType::UncompressedSRGB:
                            case TextureGroupCompressionSettings::TextureType::ColorSRGB:
                            {
                                stbir_resize_uint8_srgb
                                (
                                    pImageData, importedImage->GetWidth(), importedImage->GetHeight(), importedImage->GetStride(),
                                    pNewImageData, sourceSurface.m_width, sourceSurface.m_height, sourceSurface.m_strideInBytes,
                                    pixelLayout
                                );
                            }
                            break;

                            case TextureGroupCompressionSettings::TextureType::FloatingPoint3Channel:
                            {
                                stbir_resize_float_linear
                                (
                                    reinterpret_cast<float const*>( pImageData ), importedImage->GetWidth(), importedImage->GetHeight(), importedImage->GetStride(),
                                    reinterpret_cast<float*>( pNewImageData ), sourceSurface.m_width, sourceSurface.m_height, sourceSurface.m_strideInBytes,
                                    pixelLayout
                                );
                            }
                            break;

                            default:
                            {
                                stbir_resize_uint8_linear
                                (
                                    pImageData, importedImage->GetWidth(), importedImage->GetHeight(), importedImage->GetStride(),
                                    pNewImageData, sourceSurface.m_width, sourceSurface.m_height, sourceSurface.m_strideInBytes,
                                    pixelLayout
                                );
                            }
                            break;
                        }

                        EE::Free( pImageData );
                        pImageData = pNewImageData;
                        importedImage->ReplaceImageData( pNewImageData );
                    }
                }

                if ( isSharedExponent || ( importedImage->GetNumChannels() == 1 ) )
                {
                    // Special case - no swizzle, masks are ignored
                    memcpy( sourceSurface.m_pSurfaceData, pImageData, sourceSurface.m_height * sourceSurface.m_strideInBytes );
                }
                else
                {
                    // HACK: ISPC expects always 4 channel data, imported image seems to also always have 4 channel data.
                    size_t const bytesPerPixel = sourceSurface.m_strideInBytes / sourceSurface.m_width;
                    EE_ASSERT( ( bytesPerPixel % 4 ) == 0 );

                    TInlineVector<size_t, 4> srcOffsets;
                    TInlineVector<size_t, 4> dstOffsets;

                    if ( fileMask.m_sourceMask.IsFlagSet( TextureGroupFileMask::ChannelMask::R ) ) { srcOffsets.push_back( 0 ); }
                    if ( fileMask.m_sourceMask.IsFlagSet( TextureGroupFileMask::ChannelMask::G ) ) { srcOffsets.push_back( 1 ); }
                    if ( fileMask.m_sourceMask.IsFlagSet( TextureGroupFileMask::ChannelMask::B ) ) { srcOffsets.push_back( 2 ); }
                    if ( fileMask.m_sourceMask.IsFlagSet( TextureGroupFileMask::ChannelMask::A ) ) { srcOffsets.push_back( 3 ); }

                    if ( fileMask.m_destinationMask.IsFlagSet( TextureGroupFileMask::ChannelMask::R ) ) { dstOffsets.push_back( 0 ); }
                    if ( fileMask.m_destinationMask.IsFlagSet( TextureGroupFileMask::ChannelMask::G ) ) { dstOffsets.push_back( 1 ); }
                    if ( fileMask.m_destinationMask.IsFlagSet( TextureGroupFileMask::ChannelMask::B ) ) { dstOffsets.push_back( 2 ); }
                    if ( fileMask.m_destinationMask.IsFlagSet( TextureGroupFileMask::ChannelMask::A ) ) { dstOffsets.push_back( 3 ); }

                    if ( srcOffsets.size() != dstOffsets.size() )
                    {
                        return ctx.LogError( "Invalid file mask setup, the amount of source channels does not match the amount of destination channels in %s", ctx.GetInputPath().c_str() );
                    }

                    // Additional sanity checks
                    for ( size_t offset : srcOffsets )
                    {
                        if ( offset > size_t( importedImage->GetNumChannels() ) * bytesPerPixel )
                        {
                            return ctx.LogError( "Texture does not have enough channels for provided mask setup: %s", ctx.GetInputPath().c_str() );
                        }
                    }
                    for ( size_t offset : dstOffsets )
                    {
                        if ( offset > size_t( importedImage->GetNumChannels() ) * bytesPerPixel )
                        {
                            return ctx.LogError( "Texture does not have enough channels for provided mask setup: %s", ctx.GetInputPath().c_str() );
                        }
                    }

                    // Swizzle
                    switch ( bytesPerPixel )
                    {
                        case 4:
                        {
                            SwizzleChannels<uint8_t>( sourceSurface, pImageData, srcOffsets, dstOffsets );
                        }
                        break;

                        case 16:
                        {
                            SwizzleChannels<float>( sourceSurface, pImageData, srcOffsets, dstOffsets );
                        }
                        break;

                        default:
                        {
                            return ctx.LogError( "Unsupported bytes per pixel" );
                        }
                    }

                    // Invert normal map if needed
                    if ( pResourceDescriptor->m_invertNormalMap )
                    {
                        switch ( bytesPerPixel )
                        {
                            case 4:
                            {
                                InvertNormalMap<uint8_t>( sourceSurface, 255 );
                            }
                            break;

                            case 16:
                            {
                                InvertNormalMap<float>( sourceSurface, 1.0F );
                            }
                            break;

                            default:
                            {
                                return ctx.LogError( "Unsupported bytes per pixel" );
                            }
                            break;
                        }
                    }
                }

                pImageData += sourceSurface.m_height * sourceSurface.m_strideInBytes;
            }
        }

        if ( sourceSurfaces.empty() )
        {
            return ctx.LogError( "No image data imported!", ctx.GetInputPath().c_str() );
        }

        // Set ctt to use 4 threads by default
        //-------------------------------------------------------------------------

        ctt_set_thread_count( 4 );

        // Run texture compression
        //-------------------------------------------------------------------------

        uint32_t textureMipLevels = RHI::ComputeTextureMipLevels( sourceSurfaces[0].m_width, sourceSurfaces[0].m_height, uint32_t( sourceSurfaces.size() ) );
        if ( !pTextureGroup->m_mipSettings.m_generateMipmaps )
        {
            textureMipLevels = 1;
        }

        TVector<uint8_t> compressedData;
        RHI::DataFormat compressedFormat = RHI::DataFormat::Undefined;

        Milliseconds compressionTime;
        {
            ScopedTimer<PlatformClock, Milliseconds> tt( compressionTime );

            uint32_t pixelStride = ( sourceSurfaces[0].m_strideInBytes / sourceSurfaces[0].m_width );
            ctt_format cttSourceFormat = CTT_FORMAT_R8G8B8A8_UNORM;

            if ( pixelStride == 1 )
            {
                cttSourceFormat = CTT_FORMAT_R8_UNORM;
            }
            else if ( pixelStride == 2 )
            {
                cttSourceFormat = CTT_FORMAT_R8G8_UNORM;
            }
            else if ( pixelStride == 4 )
            {
                cttSourceFormat = CTT_FORMAT_R8G8B8A8_UNORM;
            }
            else
            {
                if ( isHDR )
                {
                    if ( pixelStride != 16 )
                    {
                        return ctx.LogError( "Expected 16 bytes pixel stride, got %d", pixelStride );
                    }

                    cttSourceFormat = CTT_FORMAT_R32G32B32A32_SFLOAT;
                }
                else
                {
                    EE_ASSERT( false );
                    cttSourceFormat = CTT_FORMAT_R8G8B8A8_UNORM;
                }
            }

            bool isCompressed = true;

            ctt_color_space cttSourceColorSpace = CTT_COLOR_SPACE_LINEAR;
            ctt_format cttTargetFormat = CTT_FORMAT_R8G8B8A8_UNORM;
            ctt_alpha_mode cttAlphaMode = CTT_ALPHA_MODE_STRAIGHT;

            switch ( pTextureGroup->m_compressionSettings.m_textureType )
            {
                case TextureGroupCompressionSettings::TextureType::Uncompressed:
                {
                    isCompressed = false;

                    cttSourceColorSpace = CTT_COLOR_SPACE_LINEAR;
                    cttAlphaMode = CTT_ALPHA_MODE_STRAIGHT;

                    if ( isSharedExponent )
                    {
                        cttTargetFormat = CTT_FORMAT_B8G8R8A8_UNORM; // TODO: Waiting for ctt, pass as 32 bit unorm for now and don't do any processing.
                        compressedFormat = RHI::DataFormat::RGB9_E5_UFloat;
                    }
                    else
                    {
                        if ( pixelStride == 1 )
                        {
                            cttTargetFormat = CTT_FORMAT_R8_UNORM;
                            compressedFormat = RHI::DataFormat::R8_UNorm;
                        }
                        else if ( pixelStride == 2 )
                        {
                            cttTargetFormat = CTT_FORMAT_R8G8_UNORM;
                            compressedFormat = RHI::DataFormat::RG8_UNorm;
                        }
                        else if ( pixelStride == 4 )
                        {
                            cttTargetFormat = CTT_FORMAT_R8G8B8A8_UNORM;
                            compressedFormat = RHI::DataFormat::RGBA8_UNorm;
                        }
                        else
                        {
                            if ( isHDR )
                            {
                                if ( pixelStride != 16 )
                                {
                                    return ctx.LogError( "Expected 16 bytes pixel stride, got %d", pixelStride );
                                }

                                cttTargetFormat = CTT_FORMAT_R32G32B32A32_SFLOAT;
                                compressedFormat = RHI::DataFormat::RGBA32_SFloat;
                            }
                            else
                            {
                                EE_ASSERT( false );
                                cttTargetFormat = CTT_FORMAT_R8G8B8A8_UNORM;
                                compressedFormat = RHI::DataFormat::RGBA8_UNorm;
                            }
                        }
                    }
                }
                break;

                case TextureGroupCompressionSettings::TextureType::UncompressedSRGB:
                {
                    isCompressed = false;

                    cttSourceColorSpace = CTT_COLOR_SPACE_SRGB;
                    cttAlphaMode = CTT_ALPHA_MODE_STRAIGHT;
                    cttTargetFormat = CTT_FORMAT_R8G8B8A8_SRGB;
                    compressedFormat = RHI::DataFormat::RGBA8_sRGB;
                }
                break;

                case TextureGroupCompressionSettings::TextureType::Color:
                {
                    cttSourceColorSpace = CTT_COLOR_SPACE_LINEAR;
                    cttAlphaMode = CTT_ALPHA_MODE_STRAIGHT;
                    cttTargetFormat = CTT_FORMAT_BC7_UNORM_BLOCK;
                    compressedFormat = RHI::DataFormat::DXBC7_UNorm;
                }
                break;

                case TextureGroupCompressionSettings::TextureType::ColorSRGB:
                {
                    cttSourceColorSpace = CTT_COLOR_SPACE_SRGB;
                    cttAlphaMode = CTT_ALPHA_MODE_STRAIGHT;
                    cttTargetFormat = CTT_FORMAT_BC7_UNORM_BLOCK;
                    compressedFormat = RHI::DataFormat::DXBC7_sRGB;
                }
                break;

                case TextureGroupCompressionSettings::TextureType::Grayscale1Channel:
                {
                    cttSourceColorSpace = CTT_COLOR_SPACE_LINEAR;
                    cttAlphaMode = CTT_ALPHA_MODE_OPAQUE;
                    cttTargetFormat = CTT_FORMAT_BC4_UNORM_BLOCK;
                    compressedFormat = RHI::DataFormat::DXBC4_UNorm;
                }
                break;

                case TextureGroupCompressionSettings::TextureType::Grayscale2Channel:
                {
                    cttSourceColorSpace = CTT_COLOR_SPACE_LINEAR;
                    cttAlphaMode = CTT_ALPHA_MODE_OPAQUE;
                    cttTargetFormat = CTT_FORMAT_BC5_UNORM_BLOCK;
                    compressedFormat = RHI::DataFormat::DXBC5_UNorm;
                }
                break;

                case TextureGroupCompressionSettings::TextureType::FloatingPoint3Channel:
                {
                    cttSourceColorSpace = CTT_COLOR_SPACE_LINEAR;
                    cttAlphaMode = CTT_ALPHA_MODE_OPAQUE;
                    cttTargetFormat = CTT_FORMAT_BC6H_UFLOAT_BLOCK;
                    compressedFormat = RHI::DataFormat::DXBC6H_UFloat;
                }
                break;
            }

            if ( !isCompressed && textureMipLevels == 1 )
            {
                // No need to run CTT, just memcpy the data directly
                for ( SurfaceRGBA const& sourceSurface : sourceSurfaces )
                {
                    size_t surfaceSizeInBytes = size_t( sourceSurface.m_strideInBytes ) * size_t( sourceSurface.m_height );

                    size_t compressedDataOffset = compressedData.size();
                    compressedData.resize( compressedData.size() + surfaceSizeInBytes );

                    std::memcpy( compressedData.data() + compressedDataOffset, sourceSurface.m_pSurfaceData, surfaceSizeInBytes );
                }
            }
            else
            {
                ctt_image* pImage = ctt_image_create( sourceSurfaces.size() == 1 ? CTT_TEXTURE_KIND_TEXTURE2D : CTT_TEXTURE_KIND_TEXTURE3D );
                for ( SurfaceRGBA const& sourceSurface : sourceSurfaces )
                {
                    size_t layer = 0;
                    ctt_image_add_layer( pImage, &layer );

                    uint32_t surfaceRowStride = sourceSurface.m_width * pixelStride;
                    uint32_t surfaceSliceStride = sourceSurface.m_height * surfaceRowStride;

                    ctt_surface* pSurface = ctt_surface_create
                    (
                        sourceSurface.m_pSurfaceData, surfaceSliceStride,
                        sourceSurface.m_width, sourceSurface.m_height, 1, surfaceRowStride, surfaceSliceStride,
                        cttSourceFormat, cttSourceColorSpace, cttAlphaMode
                    );

                    ctt_image_push_mip( pImage, layer, pSurface );
                }

                ctt_convert_settings convertSettings = ctt_convert_settings_default();
                convertSettings.container.tag = CTT_CONTAINER_RAW;
                if ( isCompressed )
                {
                    convertSettings.format.tag = CTT_TARGET_FORMAT_COMPRESSED;
                    convertSettings.format.compressed.format = cttTargetFormat;
                    convertSettings.format.compressed.encoder.tag = CTT_ENCODER_INTEL;
                    convertSettings.format.compressed.encoder.intel.bc7_alpha = CTT_INTEL_BC7_ALPHA_AUTO;
                }
                else
                {
                    convertSettings.format.tag = CTT_TARGET_FORMAT_UNCOMPRESSED;
                    convertSettings.format.uncompressed = cttTargetFormat;
                }
                convertSettings.mipmap = textureMipLevels > 0;
                convertSettings.mipmap_count.present = true;
                convertSettings.mipmap_count.value = textureMipLevels;
                convertSettings.mipmap_filter = CTT_MIPMAP_FILTER_LANCZOS3;
                convertSettings.quality = CTT_QUALITY_BASIC;

                ctt_pipeline_output* pOutput = nullptr;
                ctt_status status = ctt_convert( pImage, &convertSettings, &pOutput );

                if ( status != CTT_STATUS_OK )
                {
                    char const* pErrorMessage = "unknown error";
                    switch ( status )
                    {
                        case CTT_STATUS_OK: pErrorMessage = "OK"; break;
                        case CTT_STATUS_INVALID_DIMENSIONS: pErrorMessage = "INVALID_DIMENSIONS"; break;
                        case CTT_STATUS_UNSUPPORTED_FORMAT: pErrorMessage = "UNSUPPORTED_FORMAT"; break;
                        case CTT_STATUS_INVALID_SWIZZLE: pErrorMessage = "INVALID_SWIZZLE"; break;
                        case CTT_STATUS_CUBEMAP_FACE_COUNT: pErrorMessage = "CUBEMAP_FACE_COUNT"; break;
                        case CTT_STATUS_CUBEMAP_NON_UNIFORM_FACES: pErrorMessage = "CUBEMAP_NON_UNIFORM_FACES"; break;
                        case CTT_STATUS_COMPRESSION: pErrorMessage = "COMPRESSION"; break;
                        case CTT_STATUS_OUTPUT_ENCODING: pErrorMessage = "OUTPUT_ENCODING"; break;
                        case CTT_STATUS_INPUT_DECODING: pErrorMessage = "INPUT_DECODING"; break;
                        case CTT_STATUS_DATA_LENGTH_MISMATCH: pErrorMessage = "DATA_LENGTH_MISMATCH"; break;
                        case CTT_STATUS_UNSUPPORTED_CONVERSION: pErrorMessage = "UNSUPPORTED_CONVERSION"; break;
                        case CTT_STATUS_LOSSY_CONVERSION: pErrorMessage = "LOSSY_CONVERSION"; break;
                        case CTT_STATUS_INVALID_IMAGE: pErrorMessage = "INVALID_IMAGE"; break;
                        case CTT_STATUS_NULL_POINTER: pErrorMessage = "NULL_POINTER"; break;
                        case CTT_STATUS_ENCODER_NOT_COMPILED_IN: pErrorMessage = "ENCODER_NOT_COMPILED_IN"; break;
                        case CTT_STATUS_INVALID_ARGUMENT: pErrorMessage = "INVALID_ARGUMENT"; break;
                        case CTT_STATUS_INTERNAL: pErrorMessage = "INTERNAL"; break;
                        case CTT_STATUS_THREAD_POOL_ALREADY_INITIALIZED: pErrorMessage = "THREAD_POOL_ALREADY_INITIALIZED"; break;
                    };

                    return ctx.LogError( "Failed to convert texture: error code: %s error message: \"%s\"", pErrorMessage, ctt_last_error_message() );
                }

                ctt_image* pResultImage = ctt_pipeline_output_take_image( pOutput );

                size_t numLayers = ctt_image_layer_count( pResultImage );
                size_t numMips = ctt_image_mip_count( pResultImage, 0 );

                for ( size_t mipIndex = 0; mipIndex < numMips; ++mipIndex )
                {
                    for ( size_t layerIndex = 0; layerIndex < numLayers; ++layerIndex )
                    {
                        EE_ASSERT( numMips == ctt_image_mip_count( pResultImage, layerIndex ) ); // Sanity check

                        uint8_t const* pImageData = ctt_image_surface_data( pResultImage, layerIndex, mipIndex );
                        size_t imageDataSize = ctt_image_surface_data_len( pResultImage, layerIndex, mipIndex );

                        size_t compressedDataOffset = compressedData.size();
                        compressedData.resize( compressedData.size() + imageDataSize );

                        std::memcpy( compressedData.data() + compressedDataOffset, pImageData, imageDataSize );
                    }
                }

                ctt_image_destroy( pResultImage );
                ctt_pipeline_output_destroy( pOutput );
            }
        }

        ctx.LogMessage( "Compression Time: %.1fms", compressionTime.ToFloat() );

        // Write additional data file
        //-------------------------------------------------------------------------
        TextureResource texture = {};
        texture.m_width = sourceSurfaces[0].m_width;
        texture.m_height = sourceSurfaces[0].m_height;
        texture.m_depth = uint32_t( sourceSurfaces.size() );
        texture.m_arrayLayers = 1;
        texture.m_mipLevels = textureMipLevels;
        texture.m_rhiDataFormat = uint32_t( compressedFormat );

        if ( !FileSystem::WriteBinaryFile( ctx.GetAdditionalOutputPath().c_str(), compressedData.data(), compressedData.size() ) )
        {
            return ctx.LogError( "Failed to create additional data file: %s", ctx.GetAdditionalOutputPath().c_str() );
        }

        // Write resource file
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( TextureResource::s_version, TextureResource::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        Serialization::BinaryOutputArchive archive;
        archive << hdr << texture;

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            return Resource::CompilationResult::Success;
        }
        return Resource::CompilationResult::Failure;
    }
}

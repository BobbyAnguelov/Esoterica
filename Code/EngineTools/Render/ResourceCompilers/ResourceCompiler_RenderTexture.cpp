#include "ResourceCompiler_RenderTexture.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Import/Importer.h"
#include "EngineTools/Import/ImportedImage.h"
#include "Base/Render/RenderTexture.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

#include <DirectXTex.h>
#include <ispc_texcomp.h>
#include <dxgiformat.h>

//-------------------------------------------------------------------------

namespace EE::Render
{
    TextureCompiler::TextureCompiler()
        : Resource::Compiler( "TextureCompiler", s_version )
    {
        m_outputTypes.push_back( Texture::GetStaticResourceTypeID() );
        m_outputTypes.push_back( CubemapTexture::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult TextureCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        if ( ctx.m_resourceID.GetResourceTypeID() == Texture::GetStaticResourceTypeID() )
        {
            return CompileTexture( ctx );
        }
        else if ( ctx.m_resourceID.GetResourceTypeID() == CubemapTexture::GetStaticResourceTypeID() )
        {
            return CompileCubemapTexture( ctx );
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }

    Resource::CompilationResult TextureCompiler::CompileTexture( Resource::CompileContext const& ctx ) const
    {
        TextureResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        //-------------------------------------------------------------------------

        FileSystem::Path textureFilePath;
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_path, textureFilePath ) )
        {
            return Error( "Invalid texture data path: %s", resourceDescriptor.m_path.c_str() );
        }

        // Try to load the texture file
        //-------------------------------------------------------------------------

        Import::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        TUniquePtr<Import::ImportedImage> importedImage = Import::ReadImage( readerCtx, textureFilePath );

        rgba_surface surface;
        surface.ptr = const_cast<uint8_t*>( importedImage->GetImageData() );
        surface.width = importedImage->GetWidth();
        surface.height = importedImage->GetHeight();
        surface.stride = importedImage->GetStride();

        // Run texture compression
        //-------------------------------------------------------------------------

        auto GetBytesPerBlock = [] ( DXGI_FORMAT format )
        {
            switch ( format )
            {
                case DXGI_FORMAT_BC1_UNORM_SRGB:
                case DXGI_FORMAT_BC4_UNORM:
                return 8;

                case DXGI_FORMAT_BC3_UNORM_SRGB:
                case DXGI_FORMAT_BC5_UNORM:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                case DXGI_FORMAT_BC6H_UF16:
                return 16;

                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return 0;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return 0;
                }
            }
        };

        auto IntegerDivideCeiling = [] ( int n, int d ) 
        {
            return ( n + d - 1 ) / d;
        };

        int32_t const blockSize = 4;

        TVector<uint8_t> compressedData;

        int32_t const numCols = IntegerDivideCeiling( surface.width, blockSize );
        int32_t const numRows = IntegerDivideCeiling( surface.height, blockSize );
        int32_t const numBlocks = numCols * numRows;
        int32_t bytesPerBlock = 0;

        DXGI_FORMAT compressedFormat;
        switch ( resourceDescriptor.m_type )
        {
            case TextureType::AmbientOcclusion:
            {
                compressedFormat = DXGI_FORMAT_BC4_UNORM;
                bytesPerBlock = GetBytesPerBlock( compressedFormat );
                compressedData.resize( numBlocks * bytesPerBlock );

                CompressBlocksBC4( &surface, compressedData.data() );
            }
            break;

            case TextureType::TangentSpaceNormals:
            {
                compressedFormat = DXGI_FORMAT_BC5_UNORM;
                bytesPerBlock = GetBytesPerBlock( compressedFormat );
                compressedData.resize( numBlocks * bytesPerBlock );

                CompressBlocksBC5( &surface, compressedData.data() );
            }
            break;

            case TextureType::Uncompressed:
            {
                compressedFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                size_t const requiredSize = 4 * surface.width * surface.height;
                compressedData.resize( 4 * surface.width * surface.height );
                memcpy( compressedData.data(), surface.ptr, requiredSize );
                
            }
            break;

            default:
            {
                compressedFormat = DXGI_FORMAT_BC7_UNORM;
                bytesPerBlock = GetBytesPerBlock( compressedFormat );
                compressedData.resize( numBlocks * bytesPerBlock );

                bc7_enc_settings encoderSettings;
                GetProfile_alpha_veryfast( &encoderSettings );
                CompressBlocksBC7( &surface, compressedData.data(), &encoderSettings );
            }
            break;
        }

        // Create DDS container and store it in the texture
        //-------------------------------------------------------------------------

        DirectX::TexMetadata texMetaData;
        texMetaData.width = surface.width;
        texMetaData.height = surface.height;
        texMetaData.format = compressedFormat;
        texMetaData.arraySize = 1;
        texMetaData.miscFlags = 0;
        texMetaData.miscFlags2 = 0;
        texMetaData.depth = 1;
        texMetaData.mipLevels = 1;
        texMetaData.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

        DirectX::Image image;
        image.width = surface.width;
        image.height = surface.height;
        image.format = compressedFormat;
        image.pixels = compressedData.data();
        DirectX::ComputePitch( compressedFormat, surface.width, surface.height, image.rowPitch, image.slicePitch );

        DirectX::Blob ddsData;
        if ( FAILED( DirectX::SaveToDDSMemory( &image, 1, texMetaData, DirectX::DDS_FLAGS_NONE, ddsData ) ) )
        {
            return Error( "Failed to create DDS container: %s", "ERROR" );
        }

        Texture texture;
        texture.m_format = TextureFormat::DDS;
        texture.m_rawData.resize( ddsData.GetBufferSize() );
        memcpy( texture.m_rawData.data(), ddsData.GetBufferPointer(), ddsData.GetBufferSize() );

        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, Texture::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        
        Serialization::BinaryOutputArchive archive;
        archive << hdr << texture;
        
        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    Resource::CompilationResult TextureCompiler::CompileCubemapTexture( Resource::CompileContext const& ctx ) const
    {
        CubemapTextureResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Create cubemap
        //-------------------------------------------------------------------------

        CubemapTexture texture;
        texture.m_format = TextureFormat::DDS;

        FileSystem::Path const sourceTexturePath = resourceDescriptor.m_path.ToFileSystemPath( m_rawResourceDirectoryPath );
        if ( !FileSystem::Exists( sourceTexturePath ) )
        {
            return Error( "Failed to open specified source file: %s", sourceTexturePath.c_str() );
        }

        if ( !FileSystem::LoadFile( sourceTexturePath, texture.m_rawData ) )
        {
            return Error( "Failed to read specified source file: %s", sourceTexturePath.c_str() );
        }

        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, Texture::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << texture;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }
}
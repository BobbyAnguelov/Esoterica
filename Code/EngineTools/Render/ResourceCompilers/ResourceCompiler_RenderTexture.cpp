#include "ResourceCompiler_RenderTexture.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Render/TextureTools/TextureTools.h"
#include "Base/Render/RenderTexture.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

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

        Texture texture;
        texture.m_format = TextureFormat::DDS;

        if ( !ConvertTexture( textureFilePath, resourceDescriptor.m_type, texture.m_rawData ) )
        {
            return Error( "Failed to convert texture!" );
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
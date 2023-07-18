#include "ResourceCompiler_RenderMaterial.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMaterial.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    MaterialCompiler::MaterialCompiler()
        : Resource::Compiler( "MaterialCompiler", s_version )
    {
        m_outputTypes.push_back( Material::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult MaterialCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        MaterialResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        if ( !resourceDescriptor.IsValid() )
        {
            return Error( "Incomplete or invalid material descriptor" );
        }

        // Create Material
        //-------------------------------------------------------------------------

        Material material;
        material.m_pAlbedoTexture = resourceDescriptor.m_albedoTexture;
        material.m_pMetalnessTexture = resourceDescriptor.m_metalnessTexture;
        material.m_pRoughnessTexture = resourceDescriptor.m_roughnessTexture;
        material.m_pNormalMapTexture = resourceDescriptor.m_normalMapTexture;
        material.m_pAOTexture = resourceDescriptor.m_aoTexture;
        material.m_albedo = resourceDescriptor.m_albedo;
        material.m_metalness = Math::Clamp( resourceDescriptor.m_metalness, 0.0f, 1.0f );
        material.m_roughness = Math::Clamp( resourceDescriptor.m_roughness, 0.0f, 1.0f );
        material.m_normalScaler = resourceDescriptor.m_normalScaler;

        // Install dependencies
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, Material::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        hdr.m_installDependencies.push_back( material.m_pAlbedoTexture.GetResourceID() );
        
        if ( material.HasMetalnessTexture() )
        {
            hdr.m_installDependencies.push_back( material.m_pMetalnessTexture.GetResourceID() );
        }

        if ( material.HasRoughnessTexture() )
        {
            hdr.m_installDependencies.push_back( material.m_pRoughnessTexture.GetResourceID() );
        }

        if ( material.HasNormalMapTexture() )
        {
            hdr.m_installDependencies.push_back( material.m_pNormalMapTexture.GetResourceID() );
        }

        if ( material.HasAOTexture() )
        {
            hdr.m_installDependencies.push_back( material.m_pAOTexture.GetResourceID() );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << hdr << material;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    bool MaterialCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        FileSystem::Path const filePath = resourceID.GetResourcePath().ToFileSystemPath( m_rawResourceDirectoryPath );
        MaterialResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, filePath, resourceDescriptor ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( resourceDescriptor.m_albedoTexture.IsSet() )
        {
            VectorEmplaceBackUnique( outReferencedResources, resourceDescriptor.m_albedoTexture.GetResourceID() );
        }

        if ( resourceDescriptor.m_metalnessTexture.IsSet() )
        {
            VectorEmplaceBackUnique( outReferencedResources, resourceDescriptor.m_metalnessTexture.GetResourceID() );
        }

        if ( resourceDescriptor.m_roughnessTexture.IsSet() )
        {
            VectorEmplaceBackUnique( outReferencedResources, resourceDescriptor.m_roughnessTexture.GetResourceID() );
        }

        if ( resourceDescriptor.m_normalMapTexture.IsSet() )
        {
            VectorEmplaceBackUnique( outReferencedResources, resourceDescriptor.m_normalMapTexture.GetResourceID() );
        }

        if ( resourceDescriptor.m_aoTexture.IsSet() )
        {
            VectorEmplaceBackUnique( outReferencedResources, resourceDescriptor.m_aoTexture.GetResourceID() );
        }

        return true;
    }
}
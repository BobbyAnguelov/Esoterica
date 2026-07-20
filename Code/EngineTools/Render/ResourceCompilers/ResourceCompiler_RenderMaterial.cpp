#include "ResourceCompiler_RenderMaterial.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMaterial.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Engine/Render/RenderMaterial.h"
#include "Base/TypeSystem/CoreTypeIDs.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    MaterialCompiler::MaterialCompiler()
        : Resource::Compiler( "MaterialCompiler" )
    {
        RegisterOutput<Material>();
    }

    Resource::CompilationResult MaterialCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<MaterialResourceDescriptor>();

        // Create Material
        //-------------------------------------------------------------------------

        Material material;
        material.m_shaderID = pResourceDescriptor->m_shader;

        // Copy parameters from the type instance
        for ( TypeSystem::PropertyInfo const& property : pResourceDescriptor->m_shaderParameters.GetInstanceTypeInfo()->m_properties )
        {
            // Resources
            if ( property.IsResourcePtrProperty() )
            {
                Resource::ResourcePtr const* pResource = property.GetPropertyAddress<Resource::ResourcePtr>( pResourceDescriptor->m_shaderParameters.Get() );
                if ( pResource->IsSet() && pResource->GetResourceTypeID() == TextureResource::GetStaticResourceTypeID() )
                {
                    material.m_parameterStorage.m_textures.emplace_back( property.m_ID, pResource->GetResourceID() );
                }
            }
            // Scalars
            else if ( property.m_typeID == TypeSystem::GetCoreTypeID<uint32_t>() ||
                      property.m_typeID == TypeSystem::GetCoreTypeID<int32_t>() ||
                      property.m_typeID == TypeSystem::GetCoreTypeID<float>() ||
                      property.m_typeID == TypeSystem::GetCoreTypeID<BitFlags>() ||
                      property.m_typeID == TypeSystem::GetCoreTypeID<TBitFlags>() )
            {
                uint32_t parameterBits = 0;
                std::memcpy( &parameterBits, property.GetPropertyAddress( pResourceDescriptor->m_shaderParameters.Get() ), sizeof( parameterBits ) );
                material.m_parameterStorage.m_scalars.emplace_back( property.m_ID, parameterBits );
            }
            // Colors
            else if ( property.m_typeID == TypeSystem::GetCoreTypeID<Color>() )
            {
                // Convention is: everything in the pipeline is sRGB, everything in shaders is linear.
                // We do the conversion during material loading or when setting parameter values on the CPU.
                Color linearColor = property.GetPropertyAddress<Color>( pResourceDescriptor->m_shaderParameters.Get() )->ToLinear();

                uint32_t parameterBits = 0;
                std::memcpy( &parameterBits, &linearColor, sizeof( parameterBits ) );
                material.m_parameterStorage.m_scalars.emplace_back( property.m_ID, parameterBits );
            }
            // Vectors2
            else if ( property.m_typeID == TypeSystem::GetCoreTypeID<Int2>() ||
                      property.m_typeID == TypeSystem::GetCoreTypeID<Float2>() )
            {
                Int2 parameterBits = {};
                std::memcpy( &parameterBits, property.GetPropertyAddress( pResourceDescriptor->m_shaderParameters.Get() ), sizeof( parameterBits ) );
                material.m_parameterStorage.m_vectors2.emplace_back( property.m_ID, parameterBits );
            }
            // Vectors4
            else if ( property.m_typeID == TypeSystem::GetCoreTypeID<Int4>() ||
                      property.m_typeID == TypeSystem::GetCoreTypeID<Float4>() )
            {
                Int4 parameterBits = {};
                std::memcpy( &parameterBits, property.GetPropertyAddress( pResourceDescriptor->m_shaderParameters.Get() ), sizeof( parameterBits ) );
                material.m_parameterStorage.m_vectors4.emplace_back( property.m_ID, parameterBits );
            }
            // Matrices
            else if ( property.m_typeID == TypeSystem::GetCoreTypeID<Matrix>() )
            {
                Matrix parameterBits = Matrix::Identity;
                std::memcpy( &parameterBits, property.GetPropertyAddress( pResourceDescriptor->m_shaderParameters.Get() ), sizeof( parameterBits ) );
                material.m_parameterStorage.m_matrices.emplace_back( property.m_ID, parameterBits );
            }
            // Invalid or unsupported type
            else
            {
                return ctx.LogError( "Invalid shader parameter property type: %s %s ", property.m_typeID.c_str(), property.m_ID.c_str() );
            }
        }

        // Install dependencies
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( Material::s_version, Material::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        pResourceDescriptor->ForEachDependency( [&hdr] ( Resource::ResourcePtr const* pResource )
        {
            hdr.m_installDependencies.push_back( pResource->GetResourceID() );
        } );

        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << hdr << material;

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            return Resource::CompilationResult::Success;
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }
}

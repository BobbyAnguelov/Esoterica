#include "ResourceCompiler_DecalMaterial.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_DecalMaterial.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Engine/Render/RenderDecalMaterial.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    DecalMaterialCompiler::DecalMaterialCompiler()
        : Resource::Compiler( "DecalMaterialCompiler" )
    {
        RegisterOutput<DecalMaterial>();
    }

    Resource::CompilationResult DecalMaterialCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<DecalMaterialResourceDescriptor>();

        //-------------------------------------------------------------------------

        DecalMaterial decalMaterial;
        decalMaterial.m_albedoTexture = pResourceDescriptor->m_albedoTexture;
        decalMaterial.m_pbrTexture = pResourceDescriptor->m_pbrTexture;
        decalMaterial.m_normalTexture = pResourceDescriptor->m_normalTexture;
        decalMaterial.m_emissiveTexture = pResourceDescriptor->m_emissiveTexture;
        decalMaterial.m_parallaxTexture = pResourceDescriptor->m_parallaxTexture;
        decalMaterial.m_tint = pResourceDescriptor->m_tint;
        decalMaterial.m_opacity = pResourceDescriptor->m_opacity;
        decalMaterial.m_tiling = pResourceDescriptor->m_tiling;
        decalMaterial.m_shapeRadius = pResourceDescriptor->m_shapeRadius;
        decalMaterial.m_heightOffset = pResourceDescriptor->m_heightOffset;
        decalMaterial.m_emissiveColor = pResourceDescriptor->m_emissiveColor;
        decalMaterial.m_flags = pResourceDescriptor->m_flags;

        // Install dependencies
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( DecalMaterial::s_version, DecalMaterial::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        pResourceDescriptor->ForEachDependency( [&hdr] ( Resource::ResourcePtr const* pResource )
        {
            hdr.AddInstallDependency( pResource->GetResourceID() );
        } );

        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << hdr << decalMaterial;

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

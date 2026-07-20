#include "ResourceCompiler_Hitbox.h"
#include "EngineTools/Game/ResourceDescriptors/ResourceDescriptor_Hitbox.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    HitboxCompiler::HitboxCompiler()
        : Resource::Compiler( "HitboxCompiler" )
    {
        RegisterOutput<HitboxDefinition>();
    }

    Resource::CompilationResult HitboxCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<HitboxResourceDescriptor>();
        if ( !pResourceDescriptor->IsValid() )
        {
            return ctx.LogError( "Invalid hitbox definition!" );
        }

        // Transfer Data
        //-------------------------------------------------------------------------

        HitboxDefinition definition;
        definition.m_shapes = pResourceDescriptor->m_shapes;

        // Generate Creation Data
        //-------------------------------------------------------------------------

        uint32_t currentShapeMemoryOffset = 0;

        auto AddShape = [&] ( uint32_t alignment, uint32_t size )
        {
            definition.m_instanceRequiredAlignment = Math::Max( definition.m_instanceRequiredAlignment, alignment );
            uint32_t const requiredShapePadding = (uint32_t) Memory::CalculatePaddingForAlignment( currentShapeMemoryOffset, alignment );

            // Set current node offset
            definition.m_instanceShapesStartOffsets.emplace_back( currentShapeMemoryOffset + requiredShapePadding );

            // Shift memory offset to take into account the current node size
            currentShapeMemoryOffset += uint32_t( size + requiredShapePadding );
        };

        for ( auto const& shape : definition.m_shapes )
        {
            switch ( shape.m_type )
            {
                case HitboxShape::Type::Box:
                {
                    AddRuntimeShape<Hitbox::Box>( definition, currentShapeMemoryOffset );
                }
                break;

                case HitboxShape::Type::Sphere:
                {
                    AddRuntimeShape<Hitbox::Sphere>( definition, currentShapeMemoryOffset );
                }
                break;

                case HitboxShape::Type::Capsule:
                {
                    AddRuntimeShape<Hitbox::Capsule>( definition, currentShapeMemoryOffset );
                }
                break;
            }
        }

        definition.m_instanceRequiredMemory = currentShapeMemoryOffset;

        EE_ASSERT( definition.m_instanceShapesStartOffsets.size() == definition.m_shapes.size() );
        EE_ASSERT( definition.m_instanceRequiredMemory > 0 );
        EE_ASSERT( definition.m_instanceRequiredAlignment > 0 );

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( HitboxDefinition::s_version, HitboxDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << definition;

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
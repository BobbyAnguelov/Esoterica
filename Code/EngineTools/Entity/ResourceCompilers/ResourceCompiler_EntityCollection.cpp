#include "ResourceCompiler_EntityCollection.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Entity/ResourceDescriptors/ResourceDescriptor_EntityCollection.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionCompiler::EntityCollectionCompiler()
        : Resource::Compiler( "EntityCollectionCompiler" )
    {
        RegisterOutput<EntityCollection>();
    }

    Resource::CompilationResult EntityCollectionCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pCollectionResourceDescriptor = ctx.GetDescriptor<EntityCollectionResourceDescriptor>();

        //-------------------------------------------------------------------------
        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( EntityCollection::s_version, EntityCollection::GetStaticResourceTypeID(), ctx.m_sourceResourceHash ) << pCollectionResourceDescriptor->m_collection;
        
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
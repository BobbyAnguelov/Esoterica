#include "ResourceCompiler_PhysicsRagdoll.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Base/Serialization/TypeSerialization.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    RagdollCompiler::RagdollCompiler()
        : Resource::Compiler( "PhysicsRagdollCompiler" )
    {
        AddOutputType<RagdollDefinition>();
    }

    Resource::CompilationResult RagdollCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        RagdollResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Transfer Data
        //-------------------------------------------------------------------------

        RagdollDefinition definition;
        definition.m_skeleton = resourceDescriptor.m_skeleton;
        definition.m_bodies = resourceDescriptor.m_bodies;
        definition.m_profiles = resourceDescriptor.m_profiles;

        if ( !definition.IsValid() )
        {
            return Error( "Invalid ragdoll definition: %s", ctx.m_inputFilePath.c_str() );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( RagdollDefinition::s_version, RagdollDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );
        hdr.AddInstallDependency( definition.m_skeleton.GetResourceID() );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << definition;

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
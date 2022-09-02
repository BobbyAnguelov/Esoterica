#include "ResourceCompiler_PhysicsRagdoll.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "System/Serialization/TypeSerialization.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    RagdollCompiler::RagdollCompiler()
        : Resource::Compiler( "PhysicsRagdollCompiler", s_version )
    {
        m_outputTypes.push_back( RagdollDefinition::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult RagdollCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        RagdollResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        if ( !resourceDescriptor.m_definition.IsValid() )
        {
            return Error( "Invalid ragdoll definition: %s", ctx.m_inputFilePath.c_str() );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, RagdollDefinition::GetStaticResourceTypeID() );
        hdr.AddInstallDependency( resourceDescriptor.m_definition.m_skeleton.GetResourceID() );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << resourceDescriptor.m_definition;

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
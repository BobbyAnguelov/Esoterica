#include "ResourceCompiler_IKRig.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_IKRig.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    IKRigCompiler::IKRigCompiler() 
        : Resource::Compiler( "IKRigCompiler" )
    {
        AddOutputType<IKRigDefinition>();
    }

    Resource::CompilationResult IKRigCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        IKRigResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Transfer Data
        //-------------------------------------------------------------------------

        IKRigDefinition definition;
        definition.m_skeleton = resourceDescriptor.m_skeleton;
        definition.m_links = resourceDescriptor.m_links;
        definition.m_iterations = resourceDescriptor.m_iterations;

        if ( !definition.IsValid() )
        {
            return Error( "Invalid IK rig definition: %s", ctx.m_inputFilePath.c_str() );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( IKRigDefinition::s_version, IKRigDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );
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
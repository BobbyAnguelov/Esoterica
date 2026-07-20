#include "ResourceCompiler_AnimationGraph.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_VariationData.h"
#include "Base/TypeSystem/TypeDescriptors.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationGraphCompiler::AnimationGraphCompiler()
        : Resource::Compiler( "GraphCompiler" )
    {
        RegisterOutput<GraphDefinition>();
    }

    Resource::CompilationResult AnimationGraphCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        StringID variationID;
        ResourceID const graphResourceID = Variation::GetGraphResourceID( ctx.m_resourceID, &variationID );
        EE_ASSERT( graphResourceID.IsValid() );

        //-------------------------------------------------------------------------

        auto pGraphDescriptor = ctx.GetDescriptor<GraphResourceDescriptor>();
        variationID = pGraphDescriptor->m_graphDefinition.GetVariationHierarchy().TryGetCaseCorrectVariationID( variationID );
        if ( !pGraphDescriptor->m_graphDefinition.IsValidVariation( variationID ) )
        {
            return ctx.LogError( "Invalid graph variation '%s for graph '%s'", variationID.IsValid() ? variationID.c_str() : "", graphResourceID.c_str());
        }

        // Compile
        //-------------------------------------------------------------------------

        GraphDefinitionCompiler definitionCompiler( ctx.m_typeRegistry, ctx.m_sourceResourceDirectoryPath );
        if ( !definitionCompiler.CompileGraph( pGraphDescriptor->m_graphDefinition, variationID ) )
        {
            // Dump log
            for ( auto const& logEntry : definitionCompiler.GetLog() )
            {
                if ( logEntry.m_severity == Severity::Error )
                {
                    ctx.LogError( "Failed to compile graph: %s", logEntry.m_message.c_str() );
                }
                else if ( logEntry.m_severity == Severity::Warning )
                {
                    ctx.LogWarning( "Failed to compile graph: %s", logEntry.m_message.c_str() );
                }
                else if ( logEntry.m_severity == Severity::Info )
                {
                    ctx.LogMessage( "Failed to compile graph: %s", logEntry.m_message.c_str() );
                }
            }

            return ctx.LogError( "Graph compilation failed!" );
        }

        auto pRuntimeGraph = definitionCompiler.GetCompiledGraph();

        // Create header
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( GraphDefinition::s_version, GraphDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        hdr.AddInstallDependency( pRuntimeGraph->m_skeleton.GetResourceID() );

        // Add node data install dependencies
        for ( size_t i = 0; i < pRuntimeGraph->m_resources.size(); i++ )
        {
            ResourceID const& resourceID = pRuntimeGraph->m_resources[i].GetResourceID();
            EE_ASSERT( resourceID.IsValid() );

            // Ensure that we dont reference ourselves as a child-graph
            if ( resourceID == ctx.m_resourceID )
            {
                return ctx.LogError( "Cyclic resource dependency detected!" );
            }

            hdr.AddInstallDependency( resourceID );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << hdr;
        archive << *pRuntimeGraph;

        // Serialize node paths only in dev builds
        if ( ctx.IsCompilingForDevelopmentBuild() )
        {
            archive << pRuntimeGraph->m_nodePaths;
        }

        // Node types
        TypeSystem::TypeDescriptorCollection definitionTypeDescriptors;
        for ( auto pSettings : pRuntimeGraph->m_nodeDefinitions )
        {
            definitionTypeDescriptors.m_descriptors.emplace_back( TypeSystem::TypeDescriptor( ctx.m_typeRegistry, pSettings ) );
        }
        archive << definitionTypeDescriptors;

        // Node definition data
        for ( auto pSettings : pRuntimeGraph->m_nodeDefinitions )
        {
            pSettings->Save( archive );
        }

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
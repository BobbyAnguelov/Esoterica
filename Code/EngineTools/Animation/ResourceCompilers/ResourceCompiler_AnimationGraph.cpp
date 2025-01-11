#include "ResourceCompiler_AnimationGraph.h"
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
        AddOutputType<GraphDefinition>();
    }

    Resource::CompilationResult AnimationGraphCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        StringID variationID;
        ResourceID const graphResourceID = Variation::GetGraphResourceID( ctx.m_resourceID, &variationID );
        EE_ASSERT( graphResourceID.IsValid() );

        FileSystem::Path graphFilePath;
        if ( !GetFilePathForResourceID( graphResourceID, graphFilePath ) )
        {
            return Error( "Invalid graph data path: '%s'", graphResourceID.c_str() );
        }

        GraphResourceDescriptor graphDescriptor;
        if ( !TryLoadResourceDescriptor( graphFilePath, graphDescriptor ) )
        {
            return Error( "Failed to load graph definition: '%s'", graphResourceID.c_str() );
        }

        //-------------------------------------------------------------------------

        variationID = graphDescriptor.m_graphDefinition.GetVariationHierarchy().TryGetCaseCorrectVariationID( variationID );
        if ( !graphDescriptor.m_graphDefinition.IsValidVariation( variationID ) )
        {
            return Error( "Invalid graph variation '%s for graph '%s'", variationID.IsValid() ? variationID.c_str() : "", graphResourceID.c_str());
        }

        // Compile
        //-------------------------------------------------------------------------

        GraphDefinitionCompiler definitionCompiler;
        if ( !definitionCompiler.CompileGraph( graphDescriptor.m_graphDefinition, variationID ) )
        {
            // Dump log
            for ( auto const& logEntry : definitionCompiler.GetLog() )
            {
                if ( logEntry.m_severity == Severity::Error )
                {
                    Error( "Failed to compile graph: %s", logEntry.m_message.c_str() );
                }
                else if ( logEntry.m_severity == Severity::Warning )
                {
                    Warning( "Failed to compile graph: %s", logEntry.m_message.c_str() );
                }
                else if ( logEntry.m_severity == Severity::Info )
                {
                    Message( "Failed to compile graph: %s", logEntry.m_message.c_str() );
                }
            }

            return Error( "Graph compilation failed!" );
        }

        auto pRuntimeGraph = definitionCompiler.GetCompiledGraph();

        // Create header
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( GraphDefinition::s_version, GraphDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );
        hdr.AddInstallDependency( pRuntimeGraph->m_skeleton.GetResourceID() );

        // Add node data install dependencies
        for ( size_t i = 0; i < pRuntimeGraph->m_resources.size(); i++ )
        {
            ResourceID const& resourceID = pRuntimeGraph->m_resources[i].GetResourceID();
            EE_ASSERT( resourceID.IsValid() );

            // Ensure that we dont reference ourselves as a child-graph
            if ( resourceID == ctx.m_resourceID )
            {
                return Error( "Cyclic resource dependency detected!" );
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
            definitionTypeDescriptors.m_descriptors.emplace_back( TypeSystem::TypeDescriptor( *m_pTypeRegistry, pSettings ) );
        }
        archive << definitionTypeDescriptors;

        // Node definition data
        for ( auto pSettings : pRuntimeGraph->m_nodeDefinitions )
        {
            pSettings->Save( archive );
        }

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationGraphCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        StringID variationID;
        ResourceID const graphResourceID = Variation::GetGraphResourceID( resourceID, &variationID );
        EE_ASSERT( graphResourceID.IsValid() );

        FileSystem::Path graphFilePath;
        if ( !GetFilePathForResourceID( graphResourceID, graphFilePath ) )
        {
            return false;
        }

        GraphResourceDescriptor graphDescriptor;
        if ( !TryLoadResourceDescriptor( graphFilePath, graphDescriptor ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( !graphDescriptor.m_graphDefinition.IsValidVariation( variationID ) )
        {
            return false;
        }

        GraphDefinitionCompiler definitionCompiler;
        if ( !definitionCompiler.CompileGraph( graphDescriptor.m_graphDefinition, variationID ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto pRuntimeGraph = definitionCompiler.GetCompiledGraph();

        outReferencedResources.emplace_back( pRuntimeGraph->m_skeleton.GetResourceID() );

        for ( size_t i = 0; i < pRuntimeGraph->m_resources.size(); i++ )
        {
            ResourceID const& referencedResourceID = pRuntimeGraph->m_resources[i].GetResourceID();
            EE_ASSERT( referencedResourceID.IsValid() );
            outReferencedResources.emplace_back( referencedResourceID );
        }

        return true;
    }
}
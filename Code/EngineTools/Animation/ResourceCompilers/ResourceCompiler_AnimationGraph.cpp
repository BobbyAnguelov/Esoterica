#include "ResourceCompiler_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationGraphCompiler::AnimationGraphCompiler()
        : Resource::Compiler( "GraphCompiler", s_version )
    {
        m_outputTypes.push_back( GraphDefinition::GetStaticResourceTypeID() );
        m_outputTypes.push_back( GraphVariation::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult AnimationGraphCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto const resourceTypeID = ctx.m_resourceID.GetResourceTypeID();
        if ( resourceTypeID == GraphDefinition::GetStaticResourceTypeID() )
        {
            return CompileGraphDefinition( ctx );
        }
        else if ( resourceTypeID == GraphVariation::GetStaticResourceTypeID() )
        {
            return CompileGraphVariation( ctx );
        }

        EE_UNREACHABLE_CODE();
        return CompilationFailed( ctx );
    }

    bool AnimationGraphCompiler::LoadAndCompileGraph( FileSystem::Path const& graphFilePath, ToolsGraphDefinition& editorGraph, GraphDefinitionCompiler& definitionCompiler ) const
    {
        Serialization::JsonArchiveReader archive;
        if ( !archive.ReadFromFile( graphFilePath ) )
        {
            Error( "Failed to read animation graph file: %s", graphFilePath.c_str() );
            return false;
        }

        if ( !editorGraph.LoadFromJson( *m_pTypeRegistry, archive.GetDocument() ) )
        {
            Error( "Malformed animation graph file: %s", graphFilePath.c_str() );
            return false;
        }

        // Compile
        //-------------------------------------------------------------------------

        if ( !definitionCompiler.CompileGraph( editorGraph ) )
        {
            // Dump log
            for ( auto const& logEntry : definitionCompiler.GetLog() )
            {
                if ( logEntry.m_severity == Log::Severity::Error )
                {
                    Error( "%s", logEntry.m_message.c_str() );
                }
                else if ( logEntry.m_severity == Log::Severity::Warning )
                {
                    Warning( "%s", logEntry.m_message.c_str() );
                }
                else if ( logEntry.m_severity == Log::Severity::Info )
                {
                    Message( "%s", logEntry.m_message.c_str() );
                }
            }

            return false;
        }

        return true;
    }

    Resource::CompilationResult AnimationGraphCompiler::CompileGraphDefinition( Resource::CompileContext const& ctx ) const
    {
        ToolsGraphDefinition editorGraph;
        GraphDefinitionCompiler definitionCompiler;
        if ( !LoadAndCompileGraph( ctx.m_inputFilePath, editorGraph, definitionCompiler ) )
        {
            return CompilationFailed( ctx );
        }

        // Serialize
        //-------------------------------------------------------------------------

        auto pRuntimeGraph = definitionCompiler.GetCompiledGraph();

        Serialization::BinaryOutputArchive archive;

        archive << Resource::ResourceHeader( s_version, GraphDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        archive << *pRuntimeGraph;

        // Serialize node paths only in dev builds
        if ( ctx.IsCompilingForDevelopmentBuild() )
        {
            archive << pRuntimeGraph->m_nodePaths;
        }

        // Node definitions
        TypeSystem::TypeDescriptorCollection definitionTypeDescriptors;
        for ( auto pSettings : pRuntimeGraph->m_nodeDefinitions )
        {
            definitionTypeDescriptors.m_descriptors.emplace_back( TypeSystem::TypeDescriptor( *m_pTypeRegistry, pSettings ) );
        }
        archive << definitionTypeDescriptors;

        // Node settings data
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

    Resource::CompilationResult AnimationGraphCompiler::CompileGraphVariation( Resource::CompileContext const& ctx ) const
    {
        // Load anim graph
        //-------------------------------------------------------------------------

        StringID variationID;
        ResourceID const graphResourceID = Variation::GetGraphResourceID( ctx.m_resourceID, &variationID );
        EE_ASSERT( graphResourceID.IsValid() );

        FileSystem::Path graphFilePath;
        if ( !ConvertResourcePathToFilePath( graphResourceID.GetResourcePath(), graphFilePath ) )
        {
            return Error( "invalid graph data path: %s", graphResourceID.c_str() );
        }

        ToolsGraphDefinition editorGraph;
        GraphDefinitionCompiler definitionCompiler;
        if ( !LoadAndCompileGraph( graphFilePath, editorGraph, definitionCompiler ) )
        {
            return CompilationFailed( ctx );
        }

        // Check variation ID
        //-------------------------------------------------------------------------

        variationID = variationID.IsValid() ? variationID : Variation::s_defaultVariationID;
        variationID = editorGraph.GetVariationHierarchy().TryGetCaseCorrectVariationID( variationID );
        if ( !editorGraph.IsValidVariation( variationID ) )
        {
            return Error( "Invalid variation requested: %s", variationID.c_str() );
        }

        // Create variation resource and data set
        //-------------------------------------------------------------------------

        GraphVariation variation;
        variation.m_pGraphDefinition = graphResourceID;
        variation.m_dataSet.m_variationID = variationID;

        if ( !GenerateDataSet( ctx, editorGraph, definitionCompiler.GetRegisteredDataSlots(), variation.m_dataSet ) )
        {
            return Error( "Failed to create data set for variation: %s", variationID.c_str() );
        }

        // Create header
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, GraphVariation::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        hdr.AddInstallDependency( variation.m_pGraphDefinition.GetResourceID() );

        // Add data set install dependencies
        hdr.AddInstallDependency( variation.m_dataSet.m_skeleton.GetResourceID() );
        for ( Resource::ResourcePtr const& resourcePtr : variation.m_dataSet.m_resources )
        {
            // Skip invalid (empty) resource ID
            if ( resourcePtr.IsSet() )
            {
                // Ensure that we dont reference ourselves as a child-graph
                if ( resourcePtr.GetResourceID() == ctx.m_resourceID )
                {
                    return Error( "Cyclic resource dependency detected!" );
                }

                hdr.AddInstallDependency( resourcePtr.GetResourceID() );
            }
        }

        // Serialize variation
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << hdr;
        archive << variation;

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
        if ( resourceID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() )
        {
            StringID variationID;
            ResourceID const graphResourceID = Variation::GetGraphResourceID( resourceID, &variationID );
            EE_ASSERT( graphResourceID.IsValid() );

            FileSystem::Path graphFilePath;
            if ( !ConvertResourcePathToFilePath( graphResourceID.GetResourcePath(), graphFilePath ) )
            {
                Error( "invalid graph data path: %s", graphResourceID.c_str() );
                return false;
            }

            ToolsGraphDefinition editorGraph;
            GraphDefinitionCompiler definitionCompiler;
            if ( !LoadAndCompileGraph( graphFilePath, editorGraph, definitionCompiler ) )
            {
                return false;
            }

            // Check variation ID
            //-------------------------------------------------------------------------

            variationID = variationID.IsValid() ? variationID : Variation::s_defaultVariationID;
            variationID = editorGraph.GetVariationHierarchy().TryGetCaseCorrectVariationID( variationID );

            if ( !editorGraph.IsValidVariation( variationID ) )
            {
                Error( "Invalid variation requested: %s", variationID.c_str() );
                return false;
            }

            // Add graph definition
            //-------------------------------------------------------------------------

            VectorEmplaceBackUnique( outReferencedResources, graphResourceID );

            // Add skeleton
            //-------------------------------------------------------------------------

            auto const pVariation = editorGraph.GetVariation( variationID );
            EE_ASSERT( pVariation != nullptr );

            if ( pVariation->m_skeleton.GetResourceID().IsValid() )
            {
                VectorEmplaceBackUnique( outReferencedResources, pVariation->m_skeleton.GetResourceID() );
            }

            // Add data resources
            //-------------------------------------------------------------------------

            THashMap<UUID, GraphNodes::DataSlotToolsNode const*> dataSlotLookupMap;
            auto pRootGraph = editorGraph.GetRootGraph();
            auto const& dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
            for ( auto pSlotNode : dataSlotNodes )
            {
                dataSlotLookupMap.insert( TPair<UUID, GraphNodes::DataSlotToolsNode const*>( pSlotNode->GetID(), pSlotNode ) );
            }

            auto const& registeredDataSlots = definitionCompiler.GetRegisteredDataSlots();
            for ( auto const& dataSlotID : registeredDataSlots )
            {
                auto iter = dataSlotLookupMap.find( dataSlotID );
                if ( iter == dataSlotLookupMap.end() )
                {
                    Error( "Unknown data slot encountered (%s) when generating data set", dataSlotID.ToString().c_str() );
                    return false;
                }

                auto const dataSlotResourceID = iter->second->GetResourceID( editorGraph.GetVariationHierarchy(), variationID );
                if ( dataSlotResourceID.IsValid() )
                {
                    VectorEmplaceBackUnique( outReferencedResources, dataSlotResourceID );
                }
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    //-------------------------------------------------------------------------

    bool AnimationGraphCompiler::GenerateDataSet( Resource::CompileContext const& ctx, ToolsGraphDefinition const& editorGraph, TVector<UUID> const& registeredDataSlots, GraphDataSet& dataSet ) const
    {
        EE_ASSERT( dataSet.m_variationID.IsValid() );
        EE_ASSERT( editorGraph.IsValidVariation( dataSet.m_variationID ) );

        //-------------------------------------------------------------------------
        // Get skeleton for variation
        //-------------------------------------------------------------------------

        auto const pVariation = editorGraph.GetVariation( dataSet.m_variationID );
        EE_ASSERT( pVariation != nullptr ); 
        if ( !pVariation->m_skeleton.IsSet() )
        {
            Error( "Skeleton not set for variation: %s", dataSet.m_variationID.c_str() );
            return false;
        }

        dataSet.m_skeleton = pVariation->m_skeleton;

        //-------------------------------------------------------------------------
        // Fill data slots
        //-------------------------------------------------------------------------

        THashMap<UUID, GraphNodes::DataSlotToolsNode const*> dataSlotLookupMap;
        auto pRootGraph = editorGraph.GetRootGraph();
        auto const& dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        for ( auto pSlotNode : dataSlotNodes )
        {
            dataSlotLookupMap.insert( TPair<UUID, GraphNodes::DataSlotToolsNode const*>( pSlotNode->GetID(), pSlotNode ) );
        }

        dataSet.m_resources.reserve( registeredDataSlots.size() );

        for ( auto const& dataSlotID : registeredDataSlots )
        {
            auto iter = dataSlotLookupMap.find( dataSlotID );
            if ( iter == dataSlotLookupMap.end() )
            {
                Error( "Unknown data slot encountered (%s) when generating data set", dataSlotID.ToString().c_str() );
                return false;
            }

            auto const pDataSlotNode = iter->second;
            auto const dataSlotResourceID = pDataSlotNode->GetResourceID( editorGraph.GetVariationHierarchy(), dataSet.m_variationID );
            dataSet.m_resources.emplace_back( dataSlotResourceID );
        }

        return true;
    }
}
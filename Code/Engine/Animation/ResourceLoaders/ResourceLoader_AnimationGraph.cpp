#include "ResourceLoader_AnimationGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/TypeSystem/TypeDescriptors.h"


//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphLoader::GraphLoader()
    {
        m_loadableTypes.push_back( GraphDefinition::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( GraphVariation::GetStaticResourceTypeID() );
    }

    bool GraphLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        auto const resourceTypeID = resourceID.GetResourceTypeID();

        //-------------------------------------------------------------------------
        // Graph Definition
        //-------------------------------------------------------------------------

        if ( resourceTypeID == GraphDefinition::GetStaticResourceTypeID() )
        {
            auto* pGraphDef = EE::New<GraphDefinition>();
            archive << *pGraphDef;
            pResourceRecord->SetResourceData( pGraphDef );

            // Create parameter lookup map
            //-------------------------------------------------------------------------

            auto const numControlParameters = (int32_t) pGraphDef->m_controlParameterIDs.size();
            for ( int16_t i = 0; i < numControlParameters; i++ )
            {
                pGraphDef->m_parameterLookupMap.insert( TPair<StringID, int16_t>( pGraphDef->m_controlParameterIDs[i], i ) );
            }

            auto const numVirtualParameters = (int32_t) pGraphDef->m_virtualParameterNodeIndices.size();
            for ( int16_t i = 0; i < numVirtualParameters; i++ )
            {
                pGraphDef->m_parameterLookupMap.insert( TPair<StringID, int16_t>( pGraphDef->m_virtualParameterIDs[i], pGraphDef->m_virtualParameterNodeIndices[i] ) );
            }

            // Deserialize debug node paths
            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            archive << pGraphDef->m_nodePaths;
            #endif

            // Create Settings
            //-------------------------------------------------------------------------

            TypeSystem::TypeDescriptorCollection typeDescriptors;
            archive << typeDescriptors;

            typeDescriptors.CalculateCollectionRequirements( *m_pTypeRegistry );
            TypeSystem::TypeDescriptorCollection::InstantiateStaticCollection( *m_pTypeRegistry, typeDescriptors, pGraphDef->m_nodeDefinitions );

            for ( auto pNodeDefinition : pGraphDef->m_nodeDefinitions )
            {
                pNodeDefinition->Load( archive );
            }

            //-------------------------------------------------------------------------

            return pGraphDef->IsValid();
        }

        //-------------------------------------------------------------------------
        // Graph Variation
        //-------------------------------------------------------------------------

        else if ( resourceTypeID == GraphVariation::GetStaticResourceTypeID() )
        {
            GraphVariation* pGraphVariation = EE::New<GraphVariation>();
            archive << *pGraphVariation;
            pResourceRecord->SetResourceData( pGraphVariation );
            return pGraphVariation->m_pGraphDefinition.IsSet() && pGraphVariation->m_dataSet.m_variationID.IsValid() && pGraphVariation->m_dataSet.m_skeleton.IsSet();
        }

        EE_UNREACHABLE_CODE();
        return false;
    }

    Resource::InstallResult GraphLoader::Install( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto const resourceTypeID = resourceID.GetResourceTypeID();
        if ( resourceTypeID == GraphVariation::GetStaticResourceTypeID() )
        {
            // Graph definition
            //-------------------------------------------------------------------------

            auto pGraphVariation = pResourceRecord->GetResourceData<GraphVariation>();
            pGraphVariation->m_pGraphDefinition = GetInstallDependency( installDependencies, pGraphVariation->m_pGraphDefinition.GetResourceID() );

            // Get DataSet Skeleton
            //-------------------------------------------------------------------------

            GraphDataSet& dataSet = pGraphVariation->m_dataSet;
            EE_ASSERT( dataSet.m_skeleton.GetResourceID().IsValid() );
            dataSet.m_skeleton = GetInstallDependency( installDependencies, dataSet.m_skeleton.GetResourceID() );

            if ( !dataSet.IsValid() )
            {
                EE_LOG_ERROR( "Animation", "Graph Loader", "Failed to install skeleton for graph data set resource: %s", resourceID.ToString().c_str() );
                return Resource::InstallResult::Failed;
            }

            // Fill Slots
            //-------------------------------------------------------------------------

            int32_t const numDataSlots = (int32_t) dataSet.m_resources.size();
            for ( int32_t i = 0; i < numDataSlots; i++ )
            {
                if ( dataSet.m_resources[i].GetResourceID().IsValid() )
                {
                    dataSet.m_resources[i] = GetInstallDependency( installDependencies, dataSet.m_resources[i].GetResourceID() );

                    // Add to LUT
                    dataSet.m_resourceLUT.insert( TPair<uint32_t, Resource::ResourcePtr>( dataSet.m_resources[i].GetResourceID().GetPathID(), dataSet.m_resources[i] ) );

                    // Add child graph resources to the LUT
                    if ( dataSet.m_resources[i].GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() )
                    {
                        auto pChildGraphDataSet = dataSet.m_resources[i].GetPtr<GraphVariation>();
                        for ( auto const& iter : pChildGraphDataSet->m_dataSet.m_resourceLUT )
                        {
                            dataSet.m_resourceLUT.insert( TPair<uint32_t, Resource::ResourcePtr>( iter.second.GetResourceID().GetPathID(), iter.second ) );
                        }
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        ResourceLoader::Install( resourceID, resourcePath, installDependencies, pResourceRecord );
        return Resource::InstallResult::Succeeded;
    }

    void GraphLoader::UnloadInternal( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto const resourceTypeID = resourceID.GetResourceTypeID();

        if ( resourceTypeID == GraphDefinition::GetStaticResourceTypeID() )
        {
            // Release settings memory
            auto pGraphDef = pResourceRecord->GetResourceData<GraphDefinition>();
            if ( pGraphDef != nullptr )
            {
                TypeSystem::TypeDescriptorCollection::DestroyStaticCollection( pGraphDef->m_nodeDefinitions );
            }
        }

        ResourceLoader::UnloadInternal( resourceID, pResourceRecord );
    }
}
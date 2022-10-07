#include "ResourceLoader_AnimationGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/TypeSystem/TypeDescriptors.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphLoader::GraphLoader()
    {
        m_loadableTypes.push_back( GraphDefinition::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( GraphVariation::GetStaticResourceTypeID() );
    }

    bool GraphLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        auto const resourceTypeID = resID.GetResourceTypeID();

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
            TypeSystem::TypeDescriptorCollection::InstantiateStaticCollection( *m_pTypeRegistry, typeDescriptors, pGraphDef->m_nodeSettings );

            for ( auto pSettings : pGraphDef->m_nodeSettings )
            {
                pSettings->Load( archive );
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

    Resource::InstallResult GraphLoader::Install( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const
    {
        auto const resourceTypeID = resID.GetResourceTypeID();
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
                EE_LOG_ERROR( "Animation", "Graph Loader", "Failed to install skeleton for graph data set resource: %s", resID.ToString().c_str() );
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
                }
            }
        }

        //-------------------------------------------------------------------------

        ResourceLoader::Install( resID, pResourceRecord, installDependencies );
        return Resource::InstallResult::Succeeded;
    }

    void GraphLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto const resourceTypeID = resID.GetResourceTypeID();

        if ( resourceTypeID == GraphDefinition::GetStaticResourceTypeID() )
        {
            // Release settings memory
            auto pGraphDef = pResourceRecord->GetResourceData<GraphDefinition>();
            if ( pGraphDef != nullptr )
            {
                TypeSystem::TypeDescriptorCollection::DestroyStaticCollection( pGraphDef->m_nodeSettings );
            }
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}
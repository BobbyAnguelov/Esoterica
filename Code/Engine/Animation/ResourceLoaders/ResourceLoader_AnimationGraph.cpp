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
    }

    Resource::ResourceLoader::LoadResult GraphLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        auto const resourceTypeID = resourceID.GetResourceTypeID();

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

        // Create node definitions
        //-------------------------------------------------------------------------

        TypeSystem::TypeDescriptorCollection typeDescriptors;
        archive << typeDescriptors;

        typeDescriptors.CalculateCollectionRequirements( *m_pTypeRegistry );
        TypeSystem::TypeDescriptorCollection::InstantiateStaticCollection( *m_pTypeRegistry, typeDescriptors, pGraphDef->m_nodeDefinitions );

        for ( auto pNodeDefinition : pGraphDef->m_nodeDefinitions )
        {
            pNodeDefinition->Load( archive );
        }

        bool const loadSucceeded = pGraphDef->m_variationID.IsValid() && pGraphDef->m_skeleton.IsSet();
        return loadSucceeded ? Resource::ResourceLoader::LoadResult::Succeeded : Resource::ResourceLoader::LoadResult::Failed;
    }

    Resource::ResourceLoader::LoadResult GraphLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pGraphDefinition = pResourceRecord->GetResourceData<GraphDefinition>();

        // Skeleton
        //-------------------------------------------------------------------------

        EE_ASSERT( pGraphDefinition->m_skeleton.GetResourceID().IsValid() );
        pGraphDefinition->m_skeleton = GetInstallDependency( installDependencies, pGraphDefinition->m_skeleton.GetResourceID() );

        if ( !pGraphDefinition->m_skeleton.IsLoaded() )
        {
            EE_LOG_ERROR( "Animation", "Graph Loader", "Failed to install skeleton for graph defintion: %s", resourceID.ToString().c_str() );
            return ResourceLoader::LoadResult::Failed;
        }

        // Fill Slots
        //-------------------------------------------------------------------------

        int32_t const numDataSlots = (int32_t) pGraphDefinition->m_resources.size();
        for ( int32_t i = 0; i < numDataSlots; i++ )
        {
            if ( pGraphDefinition->m_resources[i].IsSet() )
            {
                pGraphDefinition->m_resources[i] = GetInstallDependency( installDependencies, pGraphDefinition->m_resources[i].GetResourceID() );
            }
        }

        // Generate LUT
        //-------------------------------------------------------------------------

        for ( Resource::ResourcePtr const& installDependency : installDependencies )
        {
            // Add to LUT
            pGraphDefinition->m_resourceLUT.insert( TPair<uint32_t, Resource::ResourcePtr>( installDependency.GetResourceID().GetPathID(), installDependency ) );

            // Add referenced graph resources to the LUT
            if ( installDependency.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() )
            {
                GraphDefinition const* pReferencedGraphDef = installDependency.GetPtr<GraphDefinition>();
                for ( auto const& iter : pReferencedGraphDef->m_resourceLUT )
                {
                    pGraphDefinition->m_resourceLUT.insert( TPair<uint32_t, Resource::ResourcePtr>( iter.second.GetResourceID().GetPathID(), iter.second ) );
                }
            }
        }

        //-------------------------------------------------------------------------

        return ResourceLoader::Install( resourceID, installDependencies, pResourceRecord );
    }

    void GraphLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto const resourceTypeID = resourceID.GetResourceTypeID();

        // Release settings memory
        auto pGraphDef = pResourceRecord->GetResourceData<GraphDefinition>();
        if ( pGraphDef != nullptr )
        {
            TypeSystem::TypeDescriptorCollection::DestroyStaticCollection( pGraphDef->m_nodeDefinitions );
        }

        ResourceLoader::Unload( resourceID, pResourceRecord );
    }
}
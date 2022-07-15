#include "AnimationGraphLoader.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/TypeSystem/TypeDescriptors.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphLoader::GraphLoader()
    {
        m_loadableTypes.push_back( GraphDataSet::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( GraphDefinition::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( GraphVariation::GetStaticResourceTypeID() );
    }

    bool GraphLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        auto const resourceTypeID = resID.GetResourceTypeID();

        if ( resourceTypeID == GraphDefinition::GetStaticResourceTypeID() )
        {
            auto* pGraphDef = EE::New<GraphDefinition>();
            archive << *pGraphDef;
            pResourceRecord->SetResourceData( pGraphDef );

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
        else if ( resourceTypeID == GraphDataSet::GetStaticResourceTypeID() )
        {
            GraphDataSet* pDataSet = EE::New<GraphDataSet>();
            archive << *pDataSet;
            EE_ASSERT( pDataSet->m_variationID.IsValid() && pDataSet->m_pSkeleton.IsValid() );
            pResourceRecord->SetResourceData( pDataSet );
            return true;
        }
        else if ( resourceTypeID == GraphVariation::GetStaticResourceTypeID() )
        {
            GraphVariation* pGraphVariation = EE::New<GraphVariation>();
            archive << *pGraphVariation;
            pResourceRecord->SetResourceData( pGraphVariation );
            return true;
        }

        EE_UNREACHABLE_CODE();
        return false;
    }

    Resource::InstallResult GraphLoader::Install( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const
    {
        auto const resourceTypeID = resID.GetResourceTypeID();

        if ( resourceTypeID == GraphDataSet::GetStaticResourceTypeID() )
        {
            auto pDataSet = pResourceRecord->GetResourceData<GraphDataSet>();

            EE_ASSERT( pDataSet->m_pSkeleton.GetResourceID().IsValid() );
            pDataSet->m_pSkeleton = GetInstallDependency( installDependencies, pDataSet->m_pSkeleton.GetResourceID() );

            if ( !pDataSet->IsValid() )
            {
                EE_LOG_ERROR( "Animation", "Failed to install skeleton for graph data set resource: %s", resID.ToString().c_str() );
                return Resource::InstallResult::Failed;
            }

            //-------------------------------------------------------------------------

            int32_t const numInstallDependencies = (int32_t) installDependencies.size();
            for ( auto i = 1; i < numInstallDependencies; i++ )
            {
                pDataSet->m_resources[i - 1] = GetInstallDependency( installDependencies, pDataSet->m_resources[i - 1].GetResourceID() );
            }
        }
        else if ( resourceTypeID == GraphVariation::GetStaticResourceTypeID() )
        {
            auto pGraphVariation = pResourceRecord->GetResourceData<GraphVariation>();
            pGraphVariation->m_pDataSet = GetInstallDependency( installDependencies, pGraphVariation->m_pDataSet.GetResourceID() );
            pGraphVariation->m_pGraphDefinition = GetInstallDependency( installDependencies, pGraphVariation->m_pGraphDefinition.GetResourceID() );
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
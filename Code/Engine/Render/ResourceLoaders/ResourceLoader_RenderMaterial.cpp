#include "ResourceLoader_RenderMaterial.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    MaterialLoader::MaterialLoader()
    {
        m_loadableTypes.push_back( Material::GetStaticResourceTypeID() );
    }

    bool MaterialLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        // Create mesh resource
        Material* pMaterial = EE::New<Material>();
        archive << *pMaterial;

        pResourceRecord->SetResourceData( pMaterial );
        return true;
    }

    Resource::InstallResult MaterialLoader::Install( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const
    {
        Material* pMaterial = pResourceRecord->GetResourceData<Material>();

        EE_ASSERT( pMaterial->m_pAlbedoTexture.GetResourceID().IsValid() );
        pMaterial->m_pAlbedoTexture = GetInstallDependency( installDependencies, pMaterial->m_pAlbedoTexture.GetResourceID() );

        // Optional textures
        //-------------------------------------------------------------------------

        if ( pMaterial->HasMetalnessTexture() )
        {
            pMaterial->m_pMetalnessTexture = GetInstallDependency( installDependencies, pMaterial->m_pMetalnessTexture.GetResourceID() );
        }
        
        if ( pMaterial->HasRoughnessTexture() )
        {
            pMaterial->m_pRoughnessTexture = GetInstallDependency( installDependencies, pMaterial->m_pRoughnessTexture.GetResourceID() );
        }

        if ( pMaterial->HasNormalMapTexture() )
        {
            pMaterial->m_pNormalMapTexture = GetInstallDependency( installDependencies, pMaterial->m_pNormalMapTexture.GetResourceID() );
        }

        //-------------------------------------------------------------------------

        if ( !pMaterial->IsValid() )
        {
            EE_LOG_ERROR( "Resource", "Failed to install resource: %s", resID.ToString().c_str() );
            return Resource::InstallResult::Failed;
        }
        else
        {
            ResourceLoader::Install( resID, pResourceRecord, installDependencies );
        }

        return Resource::InstallResult::Succeeded;
    }
}
#include "ResourceLoader_RenderMaterial.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Base/Serialization/BinarySerialization.h"


//-------------------------------------------------------------------------

namespace EE::Render
{
    MaterialLoader::MaterialLoader()
    {
        m_loadableTypes.push_back( Material::GetStaticResourceTypeID() );
    }

    Resource::ResourceLoader::LoadResult MaterialLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        // Create mesh resource
        Material* pMaterial = EE::New<Material>();
        archive << *pMaterial;

        pResourceRecord->SetResourceData( pMaterial );
        return Resource::ResourceLoader::LoadResult::Succeeded;
    }

    Resource::ResourceLoader::LoadResult MaterialLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
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
            EE_LOG_ERROR( "Render", "Material Loader", "Failed to install resource: %s", resourceID.ToString().c_str());
            return ResourceLoader::LoadResult::Failed;
        }
        else
        {
            ResourceLoader::Install( resourceID, installDependencies, pResourceRecord );
        }

        return Resource::ResourceLoader::LoadResult::Succeeded;
    }
}
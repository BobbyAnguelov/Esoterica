#include "ResourceLoader_DecalMaterial.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    DecalMaterialLoader::DecalMaterialLoader()
    {
        m_loadableTypes.push_back( DecalMaterial::GetStaticResourceTypeID() );
    }

    Resource::LoadResult DecalMaterialLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        DecalMaterial* pDecalMaterial = EE::New<DecalMaterial>();
        ( *pArchive ) << *pDecalMaterial;

        pResourceRecord->SetResourceData( pDecalMaterial );

        return Resource::LoadResult::Complete;
    }

    Resource::LoadResult DecalMaterialLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        DecalMaterial* pDecalMaterial = pResourceRecord->GetResourceData<DecalMaterial>();
        EE_ASSERT( pDecalMaterial != nullptr );

        //-------------------------------------------------------------------------

        auto ResolveTexture = [&installDependencies] ( TResourcePtr<TextureResource>& textureResourcePtr )
        {
            if ( !textureResourcePtr.IsSet() )
            {
                return;
            }

            textureResourcePtr = GetInstallDependency( installDependencies, textureResourcePtr.GetResourceID() );
        };

        ResolveTexture( pDecalMaterial->m_albedoTexture );
        ResolveTexture( pDecalMaterial->m_pbrTexture );
        ResolveTexture( pDecalMaterial->m_normalTexture );
        ResolveTexture( pDecalMaterial->m_emissiveTexture );
        ResolveTexture( pDecalMaterial->m_parallaxTexture );

        return Resource::LoadResult::Complete;
    }
}

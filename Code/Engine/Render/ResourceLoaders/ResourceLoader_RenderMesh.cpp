#include "ResourceLoader_RenderMesh.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Serialization/BinarySerialization.h"


//-------------------------------------------------------------------------

namespace EE::Render
{
    MeshLoader::MeshLoader()
    {
        m_loadableTypes.push_back( StaticMesh::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( SkeletalMesh::GetStaticResourceTypeID() );
    }

    Resource::ResourceLoader::LoadResult MeshLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        Mesh* pMeshResource = nullptr;

        // Static Mesh
        if ( resourceID.GetResourceTypeID() == StaticMesh::GetStaticResourceTypeID() )
        {
            StaticMesh* pStaticMesh = EE::New<StaticMesh>();
            archive << *pStaticMesh;
            pMeshResource = pStaticMesh;
        }
        else // Skeletal Mesh
        {
            SkeletalMesh* pSkeletalMesh = EE::New<SkeletalMesh>();
            archive << *pSkeletalMesh;
            pMeshResource = pSkeletalMesh;
        }

        EE_ASSERT( !pMeshResource->m_vertices.empty() );
        EE_ASSERT( !pMeshResource->m_indices.empty() );

        //-------------------------------------------------------------------------

        pResourceRecord->SetResourceData( pMeshResource );

        return Resource::ResourceLoader::LoadResult::Succeeded;
    }

    Resource::ResourceLoader::LoadResult MeshLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pMesh = pResourceRecord->GetResourceData<Mesh>();

        // Create GPU buffers
        //-------------------------------------------------------------------------

        m_pRenderDevice->LockDevice();
        {
            m_pRenderDevice->CreateBuffer( pMesh->m_vertexBuffer, pMesh->m_vertices.data() );
            EE_ASSERT( pMesh->m_vertexBuffer.IsValid() );

            m_pRenderDevice->CreateBuffer( pMesh->m_indexBuffer, pMesh->m_indices.data() );
            EE_ASSERT( pMesh->m_indexBuffer.IsValid() );
        }
        m_pRenderDevice->UnlockDevice();

        // Set materials
        //-------------------------------------------------------------------------

        for ( auto& pMaterial : pMesh->m_materials )
        {
            // Default materials are allowed to be unset
            if ( !pMaterial.GetResourceID().IsValid() )
            {
                continue;
            }

            pMaterial = GetInstallDependency( installDependencies, pMaterial.GetResourceID() );
        }

        //-------------------------------------------------------------------------

        ResourceLoader::Install( resourceID, installDependencies, pResourceRecord );
        return Resource::ResourceLoader::LoadResult::Succeeded;
    }

    void MeshLoader::Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pMesh = pResourceRecord->GetResourceData<Mesh>();
        if ( pMesh != nullptr )
        {
            m_pRenderDevice->LockDevice();
            m_pRenderDevice->DestroyBuffer( pMesh->m_vertexBuffer );
            m_pRenderDevice->DestroyBuffer( pMesh->m_indexBuffer );
            m_pRenderDevice->UnlockDevice();
        }
    }
}
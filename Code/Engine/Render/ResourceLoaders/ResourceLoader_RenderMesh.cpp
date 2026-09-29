#include "ResourceLoader_RenderMesh.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/Shaders/MeshData.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    MeshLoader::MeshLoader()
    {
        m_loadableTypes.push_back( StaticMesh::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( SkeletalMesh::GetStaticResourceTypeID() );
    }

    Resource::LoadResult MeshLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        Mesh* pMeshResource = pResourceRecord->GetResourceData<Mesh>();

        // Create Mesh
        //-------------------------------------------------------------------------

        if ( pMeshResource == nullptr )
        {
            // Static Mesh
            if ( resourceID.GetResourceTypeID() == StaticMesh::GetStaticResourceTypeID() )
            {
                StaticMesh* pStaticMesh = EE::New<StaticMesh>();
                ( *pArchive ) << *pStaticMesh;
                pMeshResource = pStaticMesh;
            }
            else // Skeletal Mesh
            {
                SkeletalMesh* pSkeletalMesh = EE::New<SkeletalMesh>();
                ( *pArchive ) << *pSkeletalMesh;
                pMeshResource = pSkeletalMesh;
            }

            EE_ASSERT( pMeshResource->IsValid() );

            //-------------------------------------------------------------------------

            EE_ASSERT( pMeshResource->m_meshBuffersState.size() == 0 );

            pMeshResource->m_meshBuffers.clear();
            pMeshResource->m_meshBuffersState.clear();

            pMeshResource->m_meshBuffers.resize( pMeshResource->GetGeometry().size() );
            pMeshResource->m_meshBuffersState.reserve( pMeshResource->GetGeometry().size() );

            for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->m_geometry.size(); ++geometryIdx )
            {
                Geometry& geometry = pMeshResource->m_geometry[geometryIdx];

                RHI::BufferParameters meshBufferParameters = {};
                meshBufferParameters.m_bufferSize = geometry.GetMeshData().size();
                meshBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::Raw };
                meshBufferParameters.m_debugName.sprintf( "MeshData %s", resourceID.c_str() );

                pMeshResource->m_meshBuffersState.emplace_back( Mesh::ResourceUpdateState{ m_pRenderSystem->CreateBufferAsync( meshBufferParameters ) } );
            }

            //-------------------------------------------------------------------------

            pResourceRecord->SetResourceData( pMeshResource );
        }

        EE_ASSERT( pMeshResource != nullptr );
        EE_ASSERT( !pMeshResource->m_meshBuffersState.empty() );

        // Wait for buffers
        //-------------------------------------------------------------------------

        bool everythingLoaded = true;

        for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
        {
            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
            Mesh::ResourceUpdateState& meshUpdateState = pMeshResource->m_meshBuffersState[geometryIdx];

            if ( meshUpdateState.m_pMeshBufferUpdate != nullptr )
            {
                AsyncResourceUpdateState updateState = meshUpdateState.m_pMeshBufferUpdate->m_updateState.load();

                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        pMeshResource->m_meshBuffers[geometryIdx] = meshUpdateState.m_pMeshBufferUpdate->m_pDstBuffer;

                        Memory::CopyToWriteCombined( meshUpdateState.m_pMeshBufferUpdate->m_pDstMemory_WriteCombined, geometry.GetMeshData().data(), meshUpdateState.m_pMeshBufferUpdate->m_dstSize );

                        meshUpdateState.m_pMeshBufferUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );
                        everythingLoaded = false;
                    }
                    break;

                    case AsyncResourceUpdateState::CompletePending:
                    {
                        // Loaded
                    }
                    break;

                    default:
                    {
                        everythingLoaded = false;
                    }
                    break;
                }
            }
        }

        if ( everythingLoaded )
        {
            // Release buffer updates
            //-------------------------------------------------------------------------
            for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
            {
                Mesh::ResourceUpdateState& bufferUpdateState = pMeshResource->m_meshBuffersState[geometryIdx];

                if ( bufferUpdateState.m_pMeshBufferUpdate != nullptr )
                {
                    bufferUpdateState.m_pMeshBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                    bufferUpdateState.m_pMeshBufferUpdate = nullptr;
                }
            }

            return Resource::LoadResult::Complete;
        }

        return Resource::LoadResult::InProgress;
    }

    Resource::UnloadResult MeshLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        bool isEverythingUnloaded = true;

        //-------------------------------------------------------------------------

        auto pMeshResource = pResourceRecord->GetResourceData<Mesh>();
        if ( pMeshResource != nullptr )
        {
            auto UnloadBuffer = [this, &isEverythingUnloaded] ( AsyncBufferUpdate*& pAsyncBufferUpdate, RHI::Buffer*& pResourceBuffer )
            {
                // Cancel buffer allocation in-flight
                if ( pAsyncBufferUpdate != nullptr )
                {
                    AsyncResourceUpdateState updateState = pAsyncBufferUpdate->m_updateState.load();
                    switch ( updateState )
                    {
                        case AsyncResourceUpdateState::UpdatePending:
                        {
                            EE_ASSERT( pResourceBuffer == nullptr );
                            EE_ASSERT( pAsyncBufferUpdate->m_pDstBuffer != nullptr );
                            m_pRenderSystem->QueueResourceDelete( eastl::move( pAsyncBufferUpdate->m_pDstBuffer ) );
                            pAsyncBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                            pAsyncBufferUpdate = nullptr;
                        }
                        break;

                        case AsyncResourceUpdateState::CompletePending:
                        {
                            EE_ASSERT( pResourceBuffer != nullptr );
                            m_pRenderSystem->QueueResourceDelete( eastl::move( pResourceBuffer ) );
                            pAsyncBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                            pAsyncBufferUpdate = nullptr;
                        }
                        break;

                        default:
                        {
                            isEverythingUnloaded = false;
                        }
                        break;
                    }
                }
                else // Delete allocated buffer
                {
                    if ( pResourceBuffer != nullptr )
                    {
                        m_pRenderSystem->QueueResourceDelete( eastl::move( pResourceBuffer ) );
                        EE_ASSERT( pResourceBuffer == nullptr );
                    }
                }
            };

            //-------------------------------------------------------------------------

            for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
            {
                Mesh::ResourceUpdateState& meshUpdateState = pMeshResource->m_meshBuffersState[geometryIdx];

                UnloadBuffer( meshUpdateState.m_pMeshBufferUpdate, pMeshResource->m_meshBuffers[geometryIdx] );
            }
        }

        //-------------------------------------------------------------------------

        if ( !isEverythingUnloaded )
        {
            return Resource::UnloadResult::InProgress;
        }

        return Resource::UnloadResult::Complete;
    }

    Resource::LoadResult MeshLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        Mesh* pMeshResource = pResourceRecord->GetResourceData<Mesh>();

        // Set materials
        //-------------------------------------------------------------------------

        for ( Mesh::Submesh& submesh : pMeshResource->m_submeshes )
        {
            if ( !submesh.m_material.IsSet() )
            {
                continue;
            }

            submesh.m_material = GetInstallDependency( installDependencies, submesh.m_material.GetResourceID() );
        }

        return Resource::LoadResult::Complete;
    }
}

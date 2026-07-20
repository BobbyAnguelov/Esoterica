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

            EE_ASSERT( pMeshResource->m_clusterBuffersState.size() == 0 );

            pMeshResource->m_clusterVertexBuffers.clear();
            pMeshResource->m_clusterTriangleBuffers.clear();
            pMeshResource->m_clusterBuffersState.clear();

            pMeshResource->m_clusterVertexBuffers.resize( pMeshResource->GetGeometry().size() );
            pMeshResource->m_clusterTriangleBuffers.resize( pMeshResource->GetGeometry().size() );
            pMeshResource->m_clusterBuffersState.reserve( pMeshResource->GetGeometry().size() );

            size_t numMeshes = 0;
            size_t numClustersForAllMeshes = 0;

            for ( Geometry const& geo : pMeshResource->GetGeometry() )
            {
                RHI::BufferParameters clusterVertexBufferParameters = {};
                clusterVertexBufferParameters.m_bufferSize = geo.GetNumClusterVertices() * geo.GetClusterVertexStride();
                clusterVertexBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::Raw );
                clusterVertexBufferParameters.m_debugName.sprintf( "Vertices %s", resourceID.c_str() );

                RHI::BufferParameters clusterTriangleBufferParameters = {};
                clusterTriangleBufferParameters.m_bufferSize = geo.GetNumClusterTriangles() * sizeof( uint32_t );
                clusterTriangleBufferParameters.m_bufferStride = sizeof( uint32_t );
                clusterTriangleBufferParameters.m_debugName.sprintf( "Triangles %s", resourceID.c_str() );

                pMeshResource->m_clusterBuffersState.emplace_back
                (
                     m_pRenderSystem->CreateBufferAsync( clusterVertexBufferParameters ),
                     m_pRenderSystem->CreateBufferAsync( clusterTriangleBufferParameters )
                );

                numMeshes++;
                numClustersForAllMeshes += geo.GetNumClusters();
            }

            EE_ASSERT( pMeshResource->m_pMeshUpdate == nullptr );
            pMeshResource->m_pMeshUpdate = m_pRenderSystem->CreateMeshAsync( numMeshes, numClustersForAllMeshes );

            //-------------------------------------------------------------------------

            pResourceRecord->SetResourceData( pMeshResource );
        }

        EE_ASSERT( pMeshResource != nullptr );
        EE_ASSERT( pMeshResource->m_pMeshUpdate != nullptr );
        EE_ASSERT( !pMeshResource->m_clusterBuffersState.empty() );

        // Wait for buffers
        //-------------------------------------------------------------------------

        bool everythingLoaded = true;

        for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
        {
            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
            Mesh::ResourceUpdateState& meshUpdateState = pMeshResource->m_clusterBuffersState[geometryIdx];

            if ( meshUpdateState.m_pVertexBufferUpdate != nullptr )
            {
                AsyncResourceUpdateState updateState = meshUpdateState.m_pVertexBufferUpdate->m_updateState.load();

                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        pMeshResource->m_clusterVertexBuffers[geometryIdx] = meshUpdateState.m_pVertexBufferUpdate->m_pDstBuffer;

                        Memory::CopyToWriteCombined( meshUpdateState.m_pVertexBufferUpdate->m_pDstMemory_WriteCombined, geometry.GetClusterVertices().data(), meshUpdateState.m_pVertexBufferUpdate->m_dstSize );

                        meshUpdateState.m_pVertexBufferUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );
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

            if ( meshUpdateState.m_pTriangleBufferUpdate != nullptr )
            {
                AsyncResourceUpdateState updateState = meshUpdateState.m_pTriangleBufferUpdate->m_updateState.load();

                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        pMeshResource->m_clusterTriangleBuffers[geometryIdx] = meshUpdateState.m_pTriangleBufferUpdate->m_pDstBuffer;

                        Memory::CopyToWriteCombined( meshUpdateState.m_pTriangleBufferUpdate->m_pDstMemory_WriteCombined, geometry.GetClusterTriangles().data(), meshUpdateState.m_pTriangleBufferUpdate->m_dstSize );

                        meshUpdateState.m_pTriangleBufferUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );
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

        // Wait for mesh update
        //-------------------------------------------------------------------------

        AsyncResourceUpdateState updateState = pMeshResource->m_pMeshUpdate->m_updateState.load();
        switch ( updateState )
        {
            case AsyncResourceUpdateState::UpdatePending:
            {
                if ( everythingLoaded )
                {
                    // Update device mesh data
                    //-------------------------------------------------------------------------
                    MeshUpdate& meshUpdate = pMeshResource->m_pMeshUpdate->m_meshUpdate;

                    pMeshResource->m_meshHandle = meshUpdate.m_meshHandle;
                    pMeshResource->m_clustersHandle = meshUpdate.m_clustersHandle;

                    uint32_t dstClusterIndex = 0;
                    for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->m_geometry.size(); ++geometryIdx )
                    {
                        Geometry const& geometry = pMeshResource->m_geometry[geometryIdx];

                        meshUpdate.m_deviceMeshes[geometryIdx].m_clusterVertexBuffer = RHI::GetBufferHandle( pMeshResource->m_clusterVertexBuffers[geometryIdx], RHI::DescriptorTypeFlags::Buffer );
                        meshUpdate.m_deviceMeshes[geometryIdx].m_clusterTriangleBuffer = RHI::GetBufferHandle( pMeshResource->m_clusterTriangleBuffers[geometryIdx], RHI::DescriptorTypeFlags::Buffer );

                        if ( pMeshResource->GetResourceTypeID() == SkeletalMesh::GetStaticResourceTypeID() )
                        {
                            SkeletalMesh const* pSkeletalMesh = static_cast<SkeletalMesh const*>( pMeshResource );
                            meshUpdate.m_deviceMeshes[geometryIdx].m_numBones = pSkeletalMesh->GetNumBones();
                        }

                        m_pRenderSystem->WriteCommonMeshData( meshUpdate, geometryIdx, dstClusterIndex, geometry );
                        dstClusterIndex += geometry.GetNumClusters();
                    }
                    EE_ASSERT( dstClusterIndex == pMeshResource->m_pMeshUpdate->m_numClustersForAllMeshes );

                    // Release buffer updates
                    //-------------------------------------------------------------------------
                    for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
                    {
                        Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
                        Mesh::ResourceUpdateState& bufferUpdateState = pMeshResource->m_clusterBuffersState[geometryIdx];

                        if ( bufferUpdateState.m_pVertexBufferUpdate != nullptr )
                        {
                            bufferUpdateState.m_pVertexBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                            bufferUpdateState.m_pVertexBufferUpdate = nullptr;
                        }

                        if ( bufferUpdateState.m_pTriangleBufferUpdate != nullptr )
                        {
                            bufferUpdateState.m_pTriangleBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                            bufferUpdateState.m_pTriangleBufferUpdate = nullptr;
                        }

                    }

                    // Signal that we're done
                    //-------------------------------------------------------------------------
                    pMeshResource->m_pMeshUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );
                }

                return Resource::LoadResult::InProgress;
            }
            break;

            case AsyncResourceUpdateState::CompletePending:
            {
                pMeshResource->m_pMeshUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                pMeshResource->m_pMeshUpdate = nullptr;
                return Resource::LoadResult::Complete;
            }
            break;

            default:
            break;
        };

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
                    if( pResourceBuffer != nullptr )
                    {
                        m_pRenderSystem->QueueResourceDelete( eastl::move( pResourceBuffer ) );
                        EE_ASSERT( pResourceBuffer == nullptr );
                    }
                }
            };

            //-------------------------------------------------------------------------

            for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
            {
                Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];

                Mesh::ResourceUpdateState& meshUpdateState = pMeshResource->m_clusterBuffersState[geometryIdx];

                UnloadBuffer( meshUpdateState.m_pVertexBufferUpdate, pMeshResource->m_clusterVertexBuffers[geometryIdx] );
                UnloadBuffer( meshUpdateState.m_pTriangleBufferUpdate, pMeshResource->m_clusterTriangleBuffers[geometryIdx] );
            }

            // Wait for mesh update
            //-------------------------------------------------------------------------

            if ( pMeshResource->m_pMeshUpdate != nullptr )
            {
                AsyncResourceUpdateState updateState = pMeshResource->m_pMeshUpdate->m_updateState.load();
                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        EE_ASSERT( !pMeshResource->m_meshHandle.m_handle.IsValid() );
                        EE_ASSERT( !pMeshResource->m_clustersHandle.m_handle.IsValid() );

                        m_pRenderSystem->QueueResourceDelete( TPair{ eastl::move( pMeshResource->m_pMeshUpdate->m_meshUpdate.m_meshHandle ), eastl::move( pMeshResource->m_pMeshUpdate->m_meshUpdate.m_clustersHandle ) } );

                        pMeshResource->m_pMeshUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                        pMeshResource->m_pMeshUpdate = nullptr;
                    }
                    break;

                    case AsyncResourceUpdateState::CompletePending:
                    {
                        EE_ASSERT( pMeshResource->m_meshHandle.m_handle.IsValid() );
                        EE_ASSERT( pMeshResource->m_clustersHandle.m_handle.IsValid() );

                        pMeshResource->m_pMeshUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                        pMeshResource->m_pMeshUpdate = nullptr;

                        m_pRenderSystem->QueueResourceDelete( TPair{ eastl::move( pMeshResource->m_meshHandle ), eastl::move( pMeshResource->m_clustersHandle ) } );
                    }
                    break;

                    default:
                    {
                        isEverythingUnloaded = false;
                    }
                    break;
                };
            }
            else
            {
                if ( pMeshResource->m_meshHandle.m_handle.IsValid() )
                {
                    EE_ASSERT( pMeshResource->m_clustersHandle.m_handle.IsValid() );
                    m_pRenderSystem->QueueResourceDelete( TPair{ eastl::move( pMeshResource->m_meshHandle ), eastl::move( pMeshResource->m_clustersHandle ) } );
                }
                else
                {
                    EE_ASSERT( !pMeshResource->m_clustersHandle.m_handle.IsValid() );
                }
            }
        }

        //-------------------------------------------------------------------------

        if( !isEverythingUnloaded )
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

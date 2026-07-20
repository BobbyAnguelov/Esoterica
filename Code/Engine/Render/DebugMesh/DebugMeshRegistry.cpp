#include "DebugMeshRegistry.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderGeometryBuilder.h"
#include "Engine/Render/Shaders/Debug/DebugDrawMesh.esf"
#include "Base/Drawing/DebugDrawing.h"
#include <atomic>

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    static std::atomic<int64_t> g_debugMeshUniqueID = 1;

    static GeometryBuilder CreateGeometryBuilder()
    {
        GeometryBuilder geometryBuilder = {};
        geometryBuilder.SetNumColorAttributes( 1 );
        geometryBuilder.InitializeVertexFormat();
        return geometryBuilder;
    }

    //-------------------------------------------------------------------------

    DebugMeshRegistry::~DebugMeshRegistry()
    {
        EE_ASSERT( m_registeredMeshes.empty() );
    }

    void DebugMeshRegistry::Initialize( RenderSystem* pRenderSystem )
    {
        EE_ASSERT( m_pRenderSystem == nullptr && pRenderSystem != nullptr );
        m_pRenderSystem = pRenderSystem;

        // Register Drawing System Meshes
        //-------------------------------------------------------------------------
        // Note: Order MUST match the order in MeshIDs

        DebugMesh mesh;

        DebugMesh::CreateBox( Quaternion::Identity, Vector::Zero, Float3::One, mesh );
        int64_t const boxMeshID = RegisterMesh( "DebugBox", mesh );
        EE_ASSERT( boxMeshID == (uint64_t) DebugMeshID::Box );

        DebugMesh::CreateSphere( Vector::Zero, 1.0f, mesh );
        int64_t const sphereMeshID = RegisterMesh( "DebugSphere", mesh );
        EE_ASSERT( sphereMeshID == (uint64_t) DebugMeshID::Sphere );

        DebugMesh::CreateHemisphere( Quaternion::Identity, Vector::Zero, 1.0f, mesh );
        int64_t const hemisphereMeshID = RegisterMesh( "DebugHemisphere", mesh );
        EE_ASSERT( hemisphereMeshID == (uint64_t) DebugMeshID::Hemisphere );

        DebugMesh::CreateCylinder( Quaternion::Identity, Vector::Zero, 1.0f, 1.0f, mesh );
        int64_t const cylinderMeshID = RegisterMesh( "DebugCylinder", mesh );
        EE_ASSERT( cylinderMeshID == (uint64_t) DebugMeshID::Cylinder );

        DebugMesh::CreateOpenCylinder( Quaternion::Identity, Vector::Zero, 1.0f, 1.0f, mesh );
        int64_t const openCylinderMeshID = RegisterMesh( "DebugOpenCylinder", mesh );
        EE_ASSERT( openCylinderMeshID == (uint64_t) DebugMeshID::OpenCylinder );
    }

    void DebugMeshRegistry::Shutdown()
    {
        // Unregister Drawing System Meshes
        //-------------------------------------------------------------------------

        UnregisterMesh( (uint64_t) DebugMeshID::OpenCylinder );
        UnregisterMesh( (uint64_t) DebugMeshID::Cylinder );
        UnregisterMesh( (uint64_t) DebugMeshID::Hemisphere );
        UnregisterMesh( (uint64_t) DebugMeshID::Sphere );
        UnregisterMesh( (uint64_t) DebugMeshID::Box );

        m_pRenderSystem = nullptr;
    }

    uint64_t DebugMeshRegistry::RegisterMesh( InlineString const& name, DebugMesh const& mesh )
    {
        EE_ASSERT( mesh.IsValid() );

        int64_t const meshID = g_debugMeshUniqueID;
        g_debugMeshUniqueID++;

        RegisteredMesh* pRegisteredMesh = nullptr;
        {
            Threading::ScopeLockWrite sl( m_mutex );
            pRegisteredMesh = &m_registeredMeshes.try_emplace( meshID, RegisteredMesh( meshID, name.c_str(), mesh ) ).first->second;
        }

        return pRegisteredMesh->m_meshID;
    }

    uint64_t DebugMeshRegistry::RegisterMesh( InlineString const& name, DebugMesh&& mesh )
    {
        EE_ASSERT( mesh.IsValid() );

        int64_t const meshID = g_debugMeshUniqueID;
        g_debugMeshUniqueID++;

        RegisteredMesh* pRegisteredMesh = nullptr;
        {
            Threading::ScopeLockWrite sl( m_mutex );
            pRegisteredMesh = &m_registeredMeshes.try_emplace( meshID, RegisteredMesh( meshID, name.c_str(), eastl::move( mesh ) ) ).first->second;
        }

        return pRegisteredMesh->m_meshID;
    }

    void DebugMeshRegistry::UnregisterMesh( uint64_t meshID )
    {
        EE_ASSERT( meshID > 0 );

        auto iter = m_registeredMeshes.find( meshID );
        EE_ASSERT( iter != m_registeredMeshes.end() );

        {
            Threading::ScopeLockWrite sl( m_mutex );
            if ( iter->second.m_meshHandle.m_handle.IsValid() )
            {
                DestroyRenderMesh( iter->second );
            }
            m_registeredMeshes.erase( iter );
        }
    }

    void DebugMeshRegistry::UpdateDeviceResources()
    {
        for ( auto& registeredMesh : m_registeredMeshes )
        {
            if ( !registeredMesh.second.m_pClusterVertexBuffer )
            {
                CreateRenderMesh( registeredMesh.second );
            }
        }
    }

    void DebugMeshRegistry::CreateRenderMesh( RegisteredMesh& registeredMesh )
    {
        GeometryBuilder geometryBuilder = CreateGeometryBuilder();

        if ( registeredMesh.m_mesh.HasIndices() )
        {
            geometryBuilder.SetIndices( registeredMesh.m_mesh.m_indices, false );
        }
        geometryBuilder.SetNumVertices( registeredMesh.m_mesh.m_vertices.size() );

        for ( size_t vertexIndex = 0; vertexIndex < registeredMesh.m_mesh.m_vertices.size(); ++vertexIndex )
        {
            DebugMesh::Vertex const& vertex = registeredMesh.m_mesh.m_vertices[vertexIndex];

            geometryBuilder.SetPositionAttribute( vertexIndex, { vertex.m_pos, vertex.m_normal } );
            geometryBuilder.SetColorAttribute( vertexIndex, 0, vertex.m_color );
        }

        geometryBuilder.Optimize();

        //---------------------------------------------------------------------

        Geometry geometry = {};
        geometry.SetVertexStride( sizeof( StaticMeshVertex ) );
        geometry.SetBounds( OBB( geometryBuilder.ComputeAABB() ) );
        geometryBuilder.BuildAndAppendGeometry( geometry );

        //---------------------------------------------------------------------
        auto CopyClustersMemory = [&geometry] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
        {
            Memory::CopyToWriteCombined( pDstMemory_WriteCombined, geometry.GetClusterVertices().data(), dstSize );
        };

        RHI::BufferParameters clusterVertexBufferParameters = {};
        clusterVertexBufferParameters.m_bufferSize = geometry.GetNumClusterVertices() * geometry.GetClusterVertexStride();
        clusterVertexBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::Raw );
        clusterVertexBufferParameters.m_debugName.sprintf( "Vertices %s", registeredMesh.m_name.c_str() );

        registeredMesh.m_pClusterVertexBuffer = m_pRenderSystem->QueueBufferCreate( CopyClustersMemory, clusterVertexBufferParameters );

        //---------------------------------------------------------------------
        auto CopyTrianglesMemory = [&geometry] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
        {
            Memory::CopyToWriteCombined( pDstMemory_WriteCombined, geometry.GetClusterTriangles().data(), dstSize );
        };

        RHI::BufferParameters clusterTriangleBufferParameters = {};
        clusterTriangleBufferParameters.m_bufferSize = geometry.GetNumClusterTriangles() * sizeof( uint32_t );
        clusterTriangleBufferParameters.m_bufferStride = sizeof( uint32_t );
        clusterTriangleBufferParameters.m_debugName.sprintf( "Triangles %s", registeredMesh.m_name.c_str() );

        registeredMesh.m_pClusterTriangleBuffer = m_pRenderSystem->QueueBufferCreate( CopyTrianglesMemory, clusterTriangleBufferParameters );

        //---------------------------------------------------------------------
        MeshUpdate meshUpdate = m_pRenderSystem->CreateMesh( 1, geometry.GetNumClusters() );

        registeredMesh.m_meshHandle = meshUpdate.m_meshHandle;
        registeredMesh.m_clustersHandle = meshUpdate.m_clustersHandle;

        meshUpdate.m_deviceMeshes[0].m_clusterVertexBuffer = RHI::GetBufferHandle( registeredMesh.m_pClusterVertexBuffer, RHI::DescriptorTypeFlags::Buffer );
        meshUpdate.m_deviceMeshes[0].m_clusterTriangleBuffer = RHI::GetBufferHandle( registeredMesh.m_pClusterTriangleBuffer, RHI::DescriptorTypeFlags::Buffer );

        meshUpdate.m_deviceMeshes[0].m_numBones = 0;

        m_pRenderSystem->WriteCommonMeshData( meshUpdate, 0, 0, geometry );
        m_pRenderSystem->QueueMeshUpdate( meshUpdate.m_meshHandle, meshUpdate.m_clustersHandle );
    }

    void DebugMeshRegistry::DestroyRenderMesh( RegisteredMesh& registeredMesh )
    {
        m_pRenderSystem->QueueResourceDelete
        (
            eastl::move( registeredMesh.m_pClusterVertexBuffer ), eastl::move( registeredMesh.m_pClusterTriangleBuffer ),
            TPair{ eastl::move( registeredMesh.m_meshHandle ), eastl::move( registeredMesh.m_clustersHandle ) }
        );
    }
}
#endif

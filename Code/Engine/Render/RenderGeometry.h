#pragma once

#include "Engine/Render/RenderMaterial.h" // TODO: Decouple
#include "Base/Math/Math.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------

    struct StaticMeshVertex final
    {
        uint16_t                        m_compressedPositionX = 0;
        uint16_t                        m_compressedPositionY = 0;
        uint16_t                        m_compressedPositionZ = 0;
        int16_t                         m_compressedNormalX = 0;
        int16_t                         m_compressedNormalY = 0;
        int16_t                         m_compressedNormalZ = 0;
        Float2                          m_uv0 = Float2::Zero;
        Float2                          m_uv1 = Float2::Zero;
        uint32_t                        m_packedColor = 0;

        inline void Initialize( Int3 compressedPosition, Float3 normal )
        {
            EE_ASSERT( compressedPosition.m_x >= 0 && compressedPosition.m_x <= UINT16_MAX );
            EE_ASSERT( compressedPosition.m_y >= 0 && compressedPosition.m_y <= UINT16_MAX );
            EE_ASSERT( compressedPosition.m_z >= 0 && compressedPosition.m_z <= UINT16_MAX );

            m_compressedPositionX = uint16_t( compressedPosition.m_x );
            m_compressedPositionY = uint16_t( compressedPosition.m_y );
            m_compressedPositionZ = uint16_t( compressedPosition.m_z );

            m_compressedNormalX = Math::FloatToSNorm16( normal.m_x );
            m_compressedNormalY = Math::FloatToSNorm16( normal.m_y );
            m_compressedNormalZ = Math::FloatToSNorm16( normal.m_z );
        }

        inline Float3 GetPosition( Int3 anchor, int32_t exponent ) const
        {
            Int3 decompressedPosition = anchor + Int3( m_compressedPositionX, m_compressedPositionY, m_compressedPositionZ );

            return Float3
            (
                ldexpf( float( decompressedPosition.m_x ), exponent ),
                ldexpf( float( decompressedPosition.m_y ), exponent ),
                ldexpf( float( decompressedPosition.m_z ), exponent )
            );
        }

        inline Float3 GetNormal() const
        {
            return Float3( Math::SNorm16ToFloat( m_compressedNormalX ),
                           Math::SNorm16ToFloat( m_compressedNormalY ),
                           Math::SNorm16ToFloat( m_compressedNormalZ ) );
        }


    };

    //-------------------------------------------------------------------------

    struct SkeletalMeshVertex final
    {
        StaticMeshVertex                m_vertex;
        Int4                            m_boneIndices;
        Float4                          m_boneWeights;
    };

    //-------------------------------------------------------------------------

    struct MeshCluster final
    {
        uint16_t                        m_boundingSphere[4] = {};       // W is unused
        uint32_t                        m_vertexOffset = 0;
        uint32_t                        m_triangleOffset = 0;
        int32_t                         m_anchorX : 24 = 0;
        uint32_t                        m_numVertices : 8 = 0;          // only 6 bits needed
        int32_t                         m_anchorY : 24 = 0;
        uint32_t                        m_numTriangles : 8 = 0;         // only 7 bits needed
        int32_t                         m_anchorZ : 24 = 0;
        int32_t                         m_sharedExponent : 8 = 0;
        float                           m_boundingSphereRadius = 0.0F;  // TODO: a whole float is kinda wasteful, can compress this to get more bits for other stuff. Not needed right now.

        inline void SetAnchorAndExponent( Int3 anchor, int32_t exponent )
        {
            // We want to be within the DXR2 spec with this compression so that the vertex data is compatible without conversion.
            // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing2.html#compressed1-position-encoding
            EE_ASSERT( exponent >= -126 && exponent <= 105 );

            m_anchorX = anchor.m_x;
            m_anchorY = anchor.m_y;
            m_anchorZ = anchor.m_z;
            m_sharedExponent = exponent;

            EE_ASSERT( m_anchorX == anchor.m_x );
            EE_ASSERT( m_anchorY == anchor.m_y );
            EE_ASSERT( m_anchorZ == anchor.m_z );
        }

        inline void SetBoundingSphere( Int3 center, float radius )
        {
            Int3 compressedCenter = center - Int3( m_anchorX, m_anchorY, m_anchorZ );

            EE_ASSERT( compressedCenter.m_x >= 0 && compressedCenter.m_x <= UINT16_MAX );
            EE_ASSERT( compressedCenter.m_y >= 0 && compressedCenter.m_y <= UINT16_MAX );
            EE_ASSERT( compressedCenter.m_z >= 0 && compressedCenter.m_z <= UINT16_MAX );

            m_boundingSphere[0] = uint16_t( compressedCenter.m_x );
            m_boundingSphere[1] = uint16_t( compressedCenter.m_y );
            m_boundingSphere[2] = uint16_t( compressedCenter.m_z );
            m_boundingSphereRadius = radius;
        }

        inline Float4 GetBoundingSphere() const
        {
            Int3 decompressedCenter = Int3( m_anchorX, m_anchorY, m_anchorZ ) + Int3( m_boundingSphere[0], m_boundingSphere[1], m_boundingSphere[2] );

            return Float4
            (
                ldexpf( float( decompressedCenter.m_x ), m_sharedExponent ),
                ldexpf( float( decompressedCenter.m_y ), m_sharedExponent ),
                ldexpf( float( decompressedCenter.m_z ), m_sharedExponent ),
                m_boundingSphereRadius
            );
        }

        inline static uint32_t PackTriangle( uint16_t vertex0, uint16_t vertex1, uint16_t vertex2 )
        {
            return Pack3x10( vertex0, vertex1, vertex2 );
        }

        inline static void UnpackTriangle( uint32_t packed, uint16_t& outIndex0, uint16_t& outIndex1, uint16_t& outIndex2 )
        {
            Unpack3x10( packed, outIndex0, outIndex1, outIndex2 );
        }

    private:

        EE_FORCE_INLINE static uint32_t Pack3x10( uint32_t in_10bit_0, uint16_t in_10bit_1, uint16_t in_10bit_2 )
        {
            union
            {
                struct
                {
                    uint32_t m_10bit_0 : 10;
                    uint32_t m_10bit_1 : 10;
                    uint32_t m_10bit_2 : 10;
                    uint32_t m_padding : 2;
                };
                uint32_t m_uint;
            } pack;

            static_assert( sizeof( pack ) == sizeof( uint32_t ) );

            pack.m_10bit_0 = in_10bit_0;
            pack.m_10bit_1 = in_10bit_1;
            pack.m_10bit_2 = in_10bit_2;
            pack.m_padding = 0;

            return pack.m_uint;
        }

        EE_FORCE_INLINE static uint32_t Unpack3x10( uint32_t packed, uint16_t& out_10bit_0, uint16_t& out_10bit_1, uint16_t& out_10bit_2 )
        {
            union
            {
                struct
                {
                    uint32_t m_10bit_0 : 10;
                    uint32_t m_10bit_1 : 10;
                    uint32_t m_10bit_2 : 10;
                    uint32_t m_padding : 2;
                };
                uint32_t m_uint;
            } pack;

            static_assert( sizeof( pack ) == sizeof( uint32_t ) );

            pack.m_uint = packed;

            out_10bit_0 = pack.m_10bit_0;
            out_10bit_1 = pack.m_10bit_1;
            out_10bit_2 = pack.m_10bit_2;

            return pack.m_uint;
        }
    };

    static_assert( sizeof( MeshCluster ) == 32, "Please don't increase cluster structure size, this is performance critical!" );

    //-------------------------------------------------------------------------

    struct Geometry final
    {
    public:

        // Optimal cluster sizes: https://zeux.io/2023/01/16/meshlet-size-tradeoffs/
        // These have to match the corresponding macros in the shader code!
        static constexpr size_t         MaxClusterVertices = 64;
        static constexpr size_t         MaxClusterTriangles = 64;

        static constexpr size_t         MaxMeshVertices = UINT32_MAX;
        static constexpr size_t         MaxMeshTriangles = UINT32_MAX;

        //-------------------------------------------------------------------------

        EE_SERIALIZE( m_clusterVertices, m_vertexStride, m_clusterTriangles, m_clusters, m_bounds );

    public:

        // Bounds
        //-------------------------------------------------------------------------

        inline void SetBounds( OBB bounds ) { m_bounds = bounds; }
        inline OBB const& GetBounds() const { return m_bounds; }

        // Vertices
        //-------------------------------------------------------------------------

        inline void SetVertexStride( uint32_t vertexStride )
        {
            EE_ASSERT( vertexStride == sizeof( StaticMeshVertex ) || vertexStride == sizeof( SkeletalMeshVertex ) );
            m_vertexStride = vertexStride;
        }

        inline uint32_t GetVertexStride() const { return m_vertexStride; }

        inline Blob& GetClusterVertices() { return m_clusterVertices; }
        inline Blob const& GetClusterVertices() const { return m_clusterVertices; }
        inline uint32_t GetNumClusterVertices() const { return uint32_t( m_clusterVertices.size() ) / m_vertexStride; }
        inline uint32_t GetClusterVertexStride() const { return m_vertexStride; }

        template<typename T>
        inline T const& GetVertex( uint32_t index ) const
        {
            EE_ASSERT( sizeof( T ) <= m_vertexStride );
            EE_ASSERT( ( index * m_vertexStride ) <= m_clusterVertices.size() );
            return *reinterpret_cast<T const*>( m_clusterVertices.data() + index * m_vertexStride );
        }

        // Cluster triangles
        //-------------------------------------------------------------------------

        inline TAlignedVector<uint32_t>& GetClusterTriangles() { return m_clusterTriangles; }
        inline TAlignedVector<uint32_t> const& GetClusterTriangles() const { return m_clusterTriangles; }
        inline uint32_t GetNumClusterTriangles() const { return uint32_t( m_clusterTriangles.size() ); }

        // Clusters
        //-------------------------------------------------------------------------

        inline Blob& GetClusters() { return m_clusters; }
        inline Blob const& GetClusters() const { return m_clusters; }
        inline uint32_t GetNumClusters() const { return uint32_t( m_clusters.size() / sizeof( MeshCluster ) ); }

        // Statistics
        //-------------------------------------------------------------------------

        inline size_t GetMemoryFootprint() const
        {
            return m_clusterVertices.size() + m_clusterTriangles.size() * sizeof( uint32_t ) + m_clusters.size();
        }

        // Utils
        //-------------------------------------------------------------------------

        // Iterate all triangles in the mesh, fn will get vertex indices into the cluster vertex buffer.
        template<typename F>
        inline void IterateAllTriangles( F fn ) const
        {
            uint32_t           numClusters = GetNumClusters();
            MeshCluster const* pClusters = reinterpret_cast<MeshCluster const*>( m_clusters.data() );

            for ( uint32_t clusterIndex = 0; clusterIndex < numClusters; ++clusterIndex )
            {
                MeshCluster const& cluster = pClusters[clusterIndex];

                for ( uint32_t triangleIndex = 0; triangleIndex < cluster.m_numTriangles; ++triangleIndex )
                {
                    uint16_t triangleIndex0 = 0;
                    uint16_t triangleIndex1 = 0;
                    uint16_t triangleIndex2 = 0;
                    MeshCluster::UnpackTriangle
                    (
                        m_clusterTriangles[cluster.m_triangleOffset + triangleIndex],
                        triangleIndex0,
                        triangleIndex1,
                        triangleIndex2
                    );

                    fn(
                        Int3( cluster.m_anchorX, cluster.m_anchorY, cluster.m_anchorZ ),
                        cluster.m_sharedExponent,
                        cluster.m_vertexOffset + triangleIndex0,
                        cluster.m_vertexOffset + triangleIndex1,
                        cluster.m_vertexOffset + triangleIndex2
                    );
                }
            }
        }

    public:

        //-------------------------------------------------------------------------

        Blob                            m_clusterVertices;
        uint32_t                        m_vertexStride = 0;
        TAlignedVector<uint32_t>        m_clusterTriangles;
        Blob                            m_clusters;
        OBB                             m_bounds;
    };
}

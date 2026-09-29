#pragma once

#include "Engine/Render/RenderMaterial.h" // TODO: Decouple
#include "Base/Math/Math.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Types/Arrays.h"
#include <cstring>
#include <intrin.h>

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct MeshCluster final
    {
    public:

        // Optimal cluster sizes: https://zeux.io/2023/01/16/meshlet-size-tradeoffs/
        // These have to match the corresponding macros in the shader code!
        static constexpr uint32_t       s_maxVerticesPerCluster = 64;
        static constexpr uint32_t       s_maxTrianglesPerCluster = 64;

        static constexpr uint32_t       s_triangleIndexBits = 6;

        //-------------------------------------------------------------------------

        inline static void WriteBits( uint8_t* pData, uint32_t bitOffset, uint32_t numBits, uint32_t value )
        {
            EE_ASSERT( numBits >= 1 && numBits <= 16 );
            EE_ASSERT( value < ( 1U << numBits ) );

            uint32_t const byteAddress = ( bitOffset / 32 ) * sizeof( uint32_t );
            uint32_t const shift = bitOffset % 32;

            uint64_t bits = 0;
            memcpy( &bits, pData + byteAddress, sizeof( bits ) );

            uint64_t const mask = ( ( 1ULL << numBits ) - 1ULL ) << shift;
            bits = ( bits & ~mask ) | ( uint64_t( value ) << shift );

            memcpy( pData + byteAddress, &bits, sizeof( bits ) );
        }

        inline static uint32_t ReadBits( uint8_t const* pData, uint32_t bitOffset, uint32_t numBits )
        {
            EE_ASSERT( numBits >= 1 && numBits <= 16 );

            uint32_t const byteAddress = ( bitOffset / 32 ) * sizeof( uint32_t );
            uint32_t const shift = bitOffset % 32;

            uint64_t bits = 0;
            memcpy( &bits, pData + byteAddress, sizeof( bits ) );

            return uint32_t( bits >> shift ) & ( ( 1U << numBits ) - 1U );
        }

        inline static uint32_t GetNumBitsRequired( uint32_t value )
        {
            return 32 - _lzcnt_u32( value );
        }

        inline static void PackPositionBits( uint8_t* pDstPositionBits, uint32_t vertexIndex, Int3 value, uint32_t numBitsX, uint32_t numBitsY, uint32_t numBitsZ )
        {
            uint32_t const bitsPerVertex = numBitsX + numBitsY + numBitsZ;
            uint32_t const bitOffset = vertexIndex * bitsPerVertex;

            WriteBits( pDstPositionBits, bitOffset, numBitsX, uint32_t( value.m_x ) );
            WriteBits( pDstPositionBits, bitOffset + numBitsX, numBitsY, uint32_t( value.m_y ) );
            WriteBits( pDstPositionBits, bitOffset + numBitsX + numBitsY, numBitsZ, uint32_t( value.m_z ) );
        }

        inline static Int3 UnpackPositionBits( uint8_t const* pPositionBits, uint32_t vertexIndex, uint32_t numBitsX, uint32_t numBitsY, uint32_t numBitsZ )
        {
            uint32_t const bitsPerVertex = numBitsX + numBitsY + numBitsZ;
            uint32_t const bitOffset = vertexIndex * bitsPerVertex;

            Int3 value;
            value.m_x = int32_t( ReadBits( pPositionBits, bitOffset, numBitsX ) );
            value.m_y = int32_t( ReadBits( pPositionBits, bitOffset + numBitsX, numBitsY ) );
            value.m_z = int32_t( ReadBits( pPositionBits, bitOffset + numBitsX + numBitsY, numBitsZ ) );
            return value;
        }

        inline static uint32_t GetTriangleDataSize( uint32_t numTriangles )
        {
            EE_ASSERT( numTriangles >= 1 );

            uint32_t const dataSize = ( numTriangles * 3 * s_triangleIndexBits + 31 ) / 32 * sizeof( uint32_t );
            uint32_t const dataSizeWithPadding = ( ( numTriangles - 1 ) * 3 * s_triangleIndexBits / 32 ) * sizeof( uint32_t ) + sizeof( uint64_t );

            return Math::Max( dataSize, dataSizeWithPadding );
        }

        inline static void PackTriangleBits( uint8_t* pTriangleData, uint32_t triangleIndex, uint16_t vertex0, uint16_t vertex1, uint16_t vertex2 )
        {
            EE_ASSERT( vertex0 < s_maxVerticesPerCluster && vertex1 < s_maxVerticesPerCluster && vertex2 < s_maxVerticesPerCluster );

            uint32_t const packed = vertex0 | ( uint32_t( vertex1 ) << 6 ) | ( uint32_t( vertex2 ) << 12 );
            uint32_t const bitOffset = triangleIndex * 3 * s_triangleIndexBits;
            uint32_t const byteAddress = ( bitOffset / 32 ) * sizeof( uint32_t );
            uint32_t const shift = bitOffset % 32;

            uint64_t bits = 0;
            memcpy( &bits, pTriangleData + byteAddress, sizeof( bits ) );

            uint64_t const mask = ( ( 1ULL << ( 3 * s_triangleIndexBits ) ) - 1ULL ) << shift;
            bits = ( bits & ~mask ) | ( uint64_t( packed ) << shift );

            memcpy( pTriangleData + byteAddress, &bits, sizeof( bits ) );
        }

        inline static void UnpackTriangleBits( uint8_t const* pTriangleData, uint32_t triangleIndex, uint16_t& outIndex0, uint16_t& outIndex1, uint16_t& outIndex2 )
        {
            uint32_t const bitOffset = triangleIndex * 3 * s_triangleIndexBits;
            uint32_t const byteAddress = ( bitOffset / 32 ) * sizeof( uint32_t );
            uint32_t const shift = bitOffset % 32;

            uint64_t bits = 0;
            memcpy( &bits, pTriangleData + byteAddress, sizeof( bits ) );

            uint32_t const packed = uint32_t( bits >> shift );

            outIndex0 = uint16_t( packed & 0x3F );
            outIndex1 = uint16_t( ( packed >> 6 ) & 0x3F );
            outIndex2 = uint16_t( ( packed >> 12 ) & 0x3F );
        }

        //-------------------------------------------------------------------------

        struct VertexNormalAttribute final
        {
            int16_t                         m_compressedNormalX = 0;
            int16_t                         m_compressedNormalY = 0;
            int16_t                         m_compressedNormalZ = 0;

            inline void Initialize( Float3 normal )
            {
                m_compressedNormalX = Math::FloatToSNorm16( normal.m_x );
                m_compressedNormalY = Math::FloatToSNorm16( normal.m_y );
                m_compressedNormalZ = Math::FloatToSNorm16( normal.m_z );
            }

            inline Float3 GetNormal() const
            {
                return Float3
                (
                    Math::SNorm16ToFloat( m_compressedNormalX ),
                    Math::SNorm16ToFloat( m_compressedNormalY ),
                    Math::SNorm16ToFloat( m_compressedNormalZ )
                );
            }
        };

        static_assert( sizeof( VertexNormalAttribute ) == 6, "Expected to be exactly 6 bytes" );

        //-------------------------------------------------------------------------

        using TextureCoordinateAttribute = Float2;
        static_assert( sizeof( TextureCoordinateAttribute ) == 8, "Expected to be exactly 8 bytes" );

        //-------------------------------------------------------------------------

        using VertexColorAttribute = uint32_t;
        static_assert( sizeof( VertexColorAttribute ) == 4, "Expected to be exactly 4 bytes" );

        //-------------------------------------------------------------------------

        struct SkinningAttribute final
        {
            Int4                            m_boneIndices;
            Float4                          m_boneWeights;
        };

        static_assert( sizeof( SkinningAttribute ) == 32, "Expected to be exactly 32 bytes" );

    public:

        //-------------------------------------------------------------------------

        inline void SetNumVertices( uint32_t numVertices )
        {
            EE_ASSERT( numVertices >= 1 && numVertices <= s_maxVerticesPerCluster );
            m_numVertices = numVertices - 1;
        }

        inline uint32_t GetNumVertices() const { return m_numVertices + 1; }

        inline void SetNumTriangles( uint32_t numTriangles )
        {
            EE_ASSERT( numTriangles >= 1 && numTriangles <= s_maxTrianglesPerCluster );
            m_numTriangles = numTriangles - 1;
        }

        inline uint32_t GetNumTriangles() const { return m_numTriangles + 1; }

        inline void SetNumSkinningAttributes( uint32_t numAttributes )
        {
            EE_ASSERT( numAttributes <= 3 ); // 2-bit field
            m_numSkinningAttributes = numAttributes;
        }

        inline uint32_t GetNumSkinningAttributes() const { return m_numSkinningAttributes; }

        inline void SetNumTextureCoordinateAttributes( uint32_t numAttributes )
        {
            EE_ASSERT( numAttributes <= 3 ); // 2-bit field
            m_numTextureCoordinateAttributes = numAttributes;
        }

        inline uint32_t GetNumTextureCoordinateAttributes() const { return m_numTextureCoordinateAttributes; }

        inline void SetNumColorAttributes( uint32_t numAttributes )
        {
            EE_ASSERT( numAttributes <= 3 ); // 2-bit field
            m_numColorAttributes = numAttributes;
        }

        inline uint32_t GetNumColorAttributes() const { return m_numColorAttributes; }

        inline void SetNumPositionBitsX( uint32_t numBits )
        {
            EE_ASSERT( numBits >= 1 && numBits <= 16 ); // 4-bit field, stored as ( numBits - 1 )
            m_numPositionBitsX = numBits - 1;
        }

        inline uint32_t GetNumPositionBitsX() const { return m_numPositionBitsX + 1; }

        inline void SetNumPositionBitsY( uint32_t numBits )
        {
            EE_ASSERT( numBits >= 1 && numBits <= 16 ); // 4-bit field, stored as ( numBits - 1 )
            m_numPositionBitsY = numBits - 1;
        }

        inline uint32_t GetNumPositionBitsY() const { return m_numPositionBitsY + 1; }

        inline void SetNumPositionBitsZ( uint32_t numBits )
        {
            EE_ASSERT( numBits >= 1 && numBits <= 16 ); // 4-bit field, stored as ( numBits - 1 )
            m_numPositionBitsZ = numBits - 1;
        }

        inline uint32_t GetNumPositionBitsZ() const { return m_numPositionBitsZ + 1; }

        inline void SetDataOffsetIn8ByteBlocks( uint32_t dataOffsetIn8ByteBlocks )
        {
            m_dataOffsetIn8ByteBlocks = dataOffsetIn8ByteBlocks;
        }

        inline uint32_t GetDataOffsetIn8ByteBlocks() const { return m_dataOffsetIn8ByteBlocks; }

        inline uint32_t GetDataOffsetInBytes() const { return m_dataOffsetIn8ByteBlocks * 8; }

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

        inline Int3 GetAnchor() const { return Int3( m_anchorX, m_anchorY, m_anchorZ ); }

        inline int32_t GetSharedExponent() const { return m_sharedExponent; }

        // aabbMin/aabbMax are in the same compressed space as vertex positions
        inline void SetAABB( Int3 aabbMin, Int3 aabbMax )
        {
            Int3 const anchor = GetAnchor();

            Int3 const minOffsets = aabbMin - anchor;
            Int3 const maxOffsets = aabbMax - anchor;

            EE_ASSERT( minOffsets.m_x >= 0 && minOffsets.m_x <= UINT16_MAX );
            EE_ASSERT( minOffsets.m_y >= 0 && minOffsets.m_y <= UINT16_MAX );
            EE_ASSERT( minOffsets.m_z >= 0 && minOffsets.m_z <= UINT16_MAX );
            EE_ASSERT( maxOffsets.m_x >= 0 && maxOffsets.m_x <= UINT16_MAX );
            EE_ASSERT( maxOffsets.m_y >= 0 && maxOffsets.m_y <= UINT16_MAX );
            EE_ASSERT( maxOffsets.m_z >= 0 && maxOffsets.m_z <= UINT16_MAX );

            m_aabb[0] = uint16_t( minOffsets.m_x );
            m_aabb[1] = uint16_t( minOffsets.m_y );
            m_aabb[2] = uint16_t( minOffsets.m_z );
            m_aabb[3] = uint16_t( maxOffsets.m_x );
            m_aabb[4] = uint16_t( maxOffsets.m_y );
            m_aabb[5] = uint16_t( maxOffsets.m_z );
        }

        inline void GetAABB( Float3& outAABBMin, Float3& outAABBMax ) const
        {
            Int3 const anchor = GetAnchor();

            outAABBMin = Float3
            (
                ldexpf( float( anchor.m_x + m_aabb[0] ), m_sharedExponent ),
                ldexpf( float( anchor.m_y + m_aabb[1] ), m_sharedExponent ),
                ldexpf( float( anchor.m_z + m_aabb[2] ), m_sharedExponent )
            );

            outAABBMax = Float3
            (
                ldexpf( float( anchor.m_x + m_aabb[3] ), m_sharedExponent ),
                ldexpf( float( anchor.m_y + m_aabb[4] ), m_sharedExponent ),
                ldexpf( float( anchor.m_z + m_aabb[5] ), m_sharedExponent )
            );
        }

        inline uint32_t GetNumPositionBitsPerVertex() const
        {
            return GetNumPositionBitsX() + GetNumPositionBitsY() + GetNumPositionBitsZ();
        }

        inline uint32_t GetPositionBitsSize() const
        {
            // Positions are variable bit rate, rounded up to 4 bytes so that all following attribute sections stay 4-byte aligned
            return ( ( ( m_numVertices + 1 ) * GetNumPositionBitsPerVertex() + 31 ) & ~uint32_t( 31 ) ) / 8;
        }

        inline uint32_t GetNormalsOffset() const
        {
            return GetPositionBitsSize();
        }

        inline uint32_t GetUVsOffset() const
        {
            // Normals rounded up to 4-bytes so that all following attribute sections stay 4-byte aligned
            return GetNormalsOffset() + ( ( ( m_numVertices + 1 ) * sizeof( VertexNormalAttribute ) + 3 ) & ~uint32_t( 3 ) );
        }

        inline uint32_t GetColorsOffset() const
        {
            return GetUVsOffset() + m_numTextureCoordinateAttributes * ( m_numVertices + 1 ) * sizeof( TextureCoordinateAttribute );
        }

        inline uint32_t GetSkinningOffset() const
        {
            return GetColorsOffset() + m_numColorAttributes * ( m_numVertices + 1 ) * sizeof( VertexColorAttribute );
        }

        inline uint32_t GetTrianglesOffset() const
        {
            return GetSkinningOffset() + m_numSkinningAttributes * ( m_numVertices + 1 ) * uint32_t( sizeof( SkinningAttribute ) );
        }

    private:

        //-------------------------------------------------------------------------

        int32_t                         m_anchorX : 24 = 0;
        uint32_t                        m_numVertices : 6 = 0;                  // Stored as ( count - 1 ), max 64 vertices per cluster, hard limit
        uint32_t                        m_numSkinningAttributes : 2 = 0;

        int32_t                         m_anchorY : 24 = 0;
        uint32_t                        m_numTriangles : 6 = 0;                 // Stored as ( count - 1 ), max 64 triangles per cluster, hard limit
        uint32_t                        m_numTextureCoordinateAttributes : 2 = 0;

        int32_t                         m_anchorZ : 24 = 0;
        int32_t                         m_sharedExponent : 8 = 0;

        uint32_t                        m_numColorAttributes : 2 = 0;
        uint32_t                        m_numPositionBitsX : 4 = 0;             // Bits used to store ( position - anchor ) per axis, stored as ( numBits - 1 ), max 16 per axis
        uint32_t                        m_numPositionBitsY : 4 = 0;
        uint32_t                        m_numPositionBitsZ : 4 = 0;
        uint32_t                        m_reserved : 18 = 0;

        uint16_t                        m_aabb[6] = {};                         // Anchor-relative AABB min/max offsets

        uint32_t                        m_dataOffsetIn8ByteBlocks = 0;          // Offset to the cluster's tightly packed data, buffer-relative, in 8-byte blocks
    };

    static_assert( sizeof( MeshCluster ) == 32, "Mesh cluster must be exactly 32 bytes" );

    //-------------------------------------------------------------------------

    struct alignas( 8 ) MeshHeader final
    {
        float                           m_aabbCenterLocal[3] = {};
        float                           m_aabbHalfExtentsLocal[3] = {};

        uint32_t                        m_reserved = 0;
        uint32_t                        m_numClusters = 0;
    };

    static_assert( sizeof( MeshHeader ) == 32, "Mesh header must be exactly 32 bytes" );

    //-------------------------------------------------------------------------

    struct Geometry final
    {
    public:

        static constexpr size_t         s_maxMeshVertices = UINT32_MAX;
        static constexpr size_t         s_maxMeshTriangles = UINT32_MAX;

        //-------------------------------------------------------------------------

        EE_SERIALIZE( m_bounds, m_meshData );

        // Bounds
        //-------------------------------------------------------------------------

        inline OBB const& GetBounds() const { return m_bounds; }

        // Vertices
        //-------------------------------------------------------------------------

        inline MeshCluster::VertexNormalAttribute GetVertexNormal( uint32_t clusterIndex, uint32_t vertexIndex ) const
        {
            MeshCluster const& cluster = GetCluster( clusterIndex );
            EE_ASSERT( vertexIndex < cluster.GetNumVertices() );

            MeshCluster::VertexNormalAttribute const* pVertexNormalAttribute = reinterpret_cast<MeshCluster::VertexNormalAttribute const*>( m_meshData.data() + cluster.GetDataOffsetInBytes() + cluster.GetNormalsOffset() + vertexIndex * sizeof( MeshCluster::VertexNormalAttribute ) );
            return *pVertexNormalAttribute;
        }

        inline Float3 GetVertexPosition( uint32_t clusterIndex, uint32_t vertexIndex ) const
        {
            MeshCluster const& cluster = GetCluster( clusterIndex );
            EE_ASSERT( vertexIndex < cluster.GetNumVertices() );

            uint8_t const* pPositionBits = m_meshData.data() + cluster.GetDataOffsetInBytes();

            Int3 const decompressedPosition = MeshCluster::UnpackPositionBits
            (
                pPositionBits,
                vertexIndex,
                cluster.GetNumPositionBitsX(),
                cluster.GetNumPositionBitsY(),
                cluster.GetNumPositionBitsZ()
            ) + cluster.GetAnchor();

            return Float3
            (
                ldexpf( float( decompressedPosition.m_x ), cluster.GetSharedExponent() ),
                ldexpf( float( decompressedPosition.m_y ), cluster.GetSharedExponent() ),
                ldexpf( float( decompressedPosition.m_z ), cluster.GetSharedExponent() )
            );
        }

        // Clusters
        //-------------------------------------------------------------------------

        inline uint32_t GetNumClusters() const { return GetMeshHeader().m_numClusters; }

        inline uint32_t GetNumVertices() const
        {
            uint32_t numVertices = 0;
            for ( uint32_t clusterIndex = 0; clusterIndex < GetNumClusters(); ++clusterIndex )
            {
                numVertices += GetCluster( clusterIndex ).GetNumVertices();
            }
            return numVertices;
        }

        inline uint32_t GetNumTriangles() const
        {
            uint32_t numTriangles = 0;
            for ( uint32_t clusterIndex = 0; clusterIndex < GetNumClusters(); ++clusterIndex )
            {
                numTriangles += GetCluster( clusterIndex ).GetNumTriangles();
            }
            return numTriangles;
        }

        // Packed mesh buffer
        //-------------------------------------------------------------------------

        inline AlignedBlob& GetMeshData() { return m_meshData; }
        inline AlignedBlob const& GetMeshData() const { return m_meshData; }

        inline MeshHeader& GetMeshHeader()
        {
            EE_ASSERT( m_meshData.size() >= sizeof( MeshHeader ) );
            return *reinterpret_cast<MeshHeader*>( m_meshData.data() );
        }

        inline MeshHeader const& GetMeshHeader() const
        {
            EE_ASSERT( m_meshData.size() >= sizeof( MeshHeader ) );
            return *reinterpret_cast<MeshHeader const*>( m_meshData.data() );
        }

        inline uint32_t GetClusterAddress( uint32_t clusterIndex ) const
        {
            EE_ASSERT( clusterIndex < GetNumClusters() );
            return uint32_t( sizeof( MeshHeader ) ) + clusterIndex * uint32_t( sizeof( MeshCluster ) );
        }

        inline MeshCluster const& GetCluster( uint32_t clusterIndex ) const
        {
            EE_ASSERT( clusterIndex < GetNumClusters() );
            return *reinterpret_cast<MeshCluster const*>( m_meshData.data() + GetClusterAddress( clusterIndex ) );
        }

        // Statistics
        //-------------------------------------------------------------------------

        inline size_t GetMemoryFootprint() const
        {
            return m_meshData.size();
        }

        // Utils
        //-------------------------------------------------------------------------

        template<typename F>
        inline void IterateAllTriangles( F fn ) const
        {
            uint32_t const numClusters = GetNumClusters();

            for ( uint32_t clusterIndex = 0; clusterIndex < numClusters; ++clusterIndex )
            {
                MeshCluster const& cluster = GetCluster( clusterIndex );

                uint8_t const* pTriangleData = m_meshData.data() + cluster.GetDataOffsetInBytes() + cluster.GetTrianglesOffset();

                for ( uint32_t triangleIndex = 0; triangleIndex < cluster.GetNumTriangles(); ++triangleIndex )
                {
                    uint16_t triangleIndex0 = 0;
                    uint16_t triangleIndex1 = 0;
                    uint16_t triangleIndex2 = 0;
                    MeshCluster::UnpackTriangleBits( pTriangleData, triangleIndex, triangleIndex0, triangleIndex1, triangleIndex2 );

                    fn( clusterIndex, triangleIndex0, triangleIndex1, triangleIndex2 );
                }
            }
        }

        //-------------------------------------------------------------------------

        OBB                             m_bounds;
        AlignedBlob                     m_meshData;
    };
}

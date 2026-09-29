#include "RenderGeometryBuilder.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/ThirdParty/meshoptimizer/meshoptimizer_esoterica.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void GeometryBuilder::SetIndices( TArrayView<uint32_t const> indices, bool flipWinding )
    {
        m_indices = { indices.begin(), indices.end() };

        if ( flipWinding )
        {
            for ( size_t index = 0; index < m_indices.size(); index += 3 )
            {
                eastl::swap( m_indices[index], m_indices[index + 2] );
            }
        }
    }

    void GeometryBuilder::SetNumVertices( size_t numVertices )
    {
        EE_ASSERT( m_vertexStride );
        m_vertices.resize( numVertices * m_vertexStride );
    }

    //-------------------------------------------------------------------------

    void GeometryBuilder::SetPositionAttribute( size_t vertexIndex, PositionAttribute const& positionAttribute )
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        *reinterpret_cast<PositionAttribute*>( m_vertices.data() + vertexIndex * m_vertexStride ) = positionAttribute;
    }

    GeometryBuilder::PositionAttribute GeometryBuilder::GetPositionAttribute( size_t vertexIndex ) const
    {
        EE_ASSERT( m_vertexStride );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        return *reinterpret_cast<PositionAttribute const*>( m_vertices.data() + vertexIndex * m_vertexStride );
    }

    void GeometryBuilder::SetTextureCoordinateAttribute( size_t vertexIndex, size_t attributeIndex, TextureCoordinateAttribute const& textureCoordinateAttribute )
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( attributeIndex < m_numTextureCoordinateAttributes );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        size_t dataOffset = vertexIndex * m_vertexStride;
        dataOffset += GetTextureCoordinateAttributesOffset();
        dataOffset += attributeIndex * sizeof( TextureCoordinateAttribute );

        *reinterpret_cast<TextureCoordinateAttribute*>( m_vertices.data() + dataOffset ) = textureCoordinateAttribute;
    }

    GeometryBuilder::TextureCoordinateAttribute GeometryBuilder::GetTextureCoordinateAttribute( size_t vertexIndex, size_t attributeIndex ) const
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( attributeIndex < m_numTextureCoordinateAttributes );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        size_t dataOffset = vertexIndex * m_vertexStride;
        dataOffset += GetTextureCoordinateAttributesOffset();
        dataOffset += attributeIndex * sizeof( TextureCoordinateAttribute );

        return *reinterpret_cast<TextureCoordinateAttribute const*>( m_vertices.data() + dataOffset );
    }

    bool GeometryBuilder::GetTextureCoordinateAttribute( size_t vertexIndex, size_t attributeIndex, TextureCoordinateAttribute& textureCoordinate ) const
    {
        if ( attributeIndex < m_numTextureCoordinateAttributes )
        {
            textureCoordinate = GetTextureCoordinateAttribute( vertexIndex, attributeIndex );
            return true;
        }
        return false;
    }

    void GeometryBuilder::SetColorAttribute( size_t vertexIndex, size_t attributeIndex, ColorAttribute const& colorAttribute )
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( attributeIndex < m_numColorAttributes );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        size_t dataOffset = vertexIndex * m_vertexStride;
        dataOffset += GetColorAttributesOffset();
        dataOffset += attributeIndex * sizeof( ColorAttribute );

        *reinterpret_cast<ColorAttribute*>( m_vertices.data() + dataOffset ) = colorAttribute;
    }

    GeometryBuilder::ColorAttribute GeometryBuilder::GetColorAttribute( size_t vertexIndex, size_t attributeIndex ) const
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( attributeIndex < m_numColorAttributes );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        size_t dataOffset = vertexIndex * m_vertexStride;
        dataOffset += GetColorAttributesOffset();
        dataOffset += attributeIndex * sizeof( ColorAttribute );

        return *reinterpret_cast<ColorAttribute const*>( m_vertices.data() + dataOffset );
    }

    bool GeometryBuilder::GetColorAttribute( size_t vertexIndex, size_t attributeIndex, ColorAttribute& colorAttribute ) const
    {
        if ( attributeIndex < m_numColorAttributes )
        {
            colorAttribute = GetColorAttribute( vertexIndex, attributeIndex );
            return true;
        }
        return false;
    }

    void GeometryBuilder::SetSkinningAttribute( size_t vertexIndex, size_t attributeIndex, SkinningAttribute const& skinningAttribute )
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( attributeIndex < m_numSkinningAttributes );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        size_t dataOffset = vertexIndex * m_vertexStride;
        dataOffset += GetSkinningAttributesOffset();
        dataOffset += attributeIndex * sizeof( SkinningAttribute );

        *reinterpret_cast<SkinningAttribute*>( m_vertices.data() + dataOffset ) = skinningAttribute;
    }

    GeometryBuilder::SkinningAttribute GeometryBuilder::GetSkinningAttribute( size_t vertexIndex, size_t attributeIndex ) const
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( attributeIndex < m_numSkinningAttributes );
        EE_ASSERT( ( m_vertices.size() % m_vertexStride ) == 0 );

        size_t numVertices = m_vertices.size() / m_vertexStride;
        EE_ASSERT( vertexIndex < numVertices );

        size_t dataOffset = vertexIndex * m_vertexStride;
        dataOffset += GetSkinningAttributesOffset();
        dataOffset += attributeIndex * sizeof( SkinningAttribute );

        return *reinterpret_cast<SkinningAttribute const*>( m_vertices.data() + dataOffset );
    }

    bool GeometryBuilder::GetSkinningAttribute( size_t vertexIndex, size_t attributeIndex, SkinningAttribute& skinningAttribute ) const
    {
        if ( attributeIndex < m_numSkinningAttributes )
        {
            skinningAttribute = GetSkinningAttribute( vertexIndex, attributeIndex );
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------

    bool GeometryBuilder::Simplify( float normalsWeight, float uvWeight, float targetAttributeError, uint32_t targetTriangleCount, float targetTrianglePercentage )
    {
        EE_ASSERT( m_vertexStride );

        bool simplificationSuccess = false;

        uint32_t targetIndexCount = targetTriangleCount * 3;
        targetIndexCount = Math::Min( targetIndexCount, uint32_t( float( m_indices.size() ) * ( targetTrianglePercentage / 100.0F ) ) );

        float const attributeWeights[5] =
        {
            normalsWeight,
            normalsWeight,
            normalsWeight,

            uvWeight,
            uvWeight,
        };

        float  resultError = 0.0F;
        size_t newIndexCount = 0;

        newIndexCount = meshopt_simplifyWithUpdate
        (
            m_indices.data(),
            m_indices.size(),
            reinterpret_cast<float*>( m_vertices.data() ),
            m_vertices.size() / m_vertexStride,
            m_vertexStride,
            reinterpret_cast<float*>( m_vertices.data() + sizeof( Float3 ) ),
            m_vertexStride,
            attributeWeights,
            5,
            nullptr,
            targetIndexCount,
            targetAttributeError,
            meshopt_SimplifyPermissive,
            &resultError
        );

        if ( newIndexCount != 0 && newIndexCount != m_indices.size() )
        {
            m_indices.resize( newIndexCount );
            simplificationSuccess = true;
        }

        return simplificationSuccess;
    }

    void GeometryBuilder::Optimize( TBitFlags<GeometryOptimizeFlags> flags )
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( flags.IsAnyFlagSet() );

        TAlignedVector<uint32_t>    remapIndices;
        TAlignedVector<uint8_t>     remapVertices;
        TAlignedVector<uint32_t>    remapTable;

        size_t numVertices = m_vertices.size() / m_vertexStride;

        if ( flags.IsFlagSet( GeometryOptimizeFlags::VertexRemap ) || m_indices.empty() )
        {
            remapTable.resize( numVertices );

            size_t numIndices = m_indices.size();
            if ( !numIndices )
            {
                numIndices = numVertices;
            }

            size_t numRemappedVertices = meshopt_generateVertexRemap
            (
                remapTable.data(),
                m_indices.data(),
                numIndices,
                m_vertices.data(),
                numVertices,
                m_vertexStride
            );

            remapIndices.resize( numIndices );
            remapVertices.resize( numRemappedVertices * m_vertexStride );

            meshopt_remapIndexBuffer( remapIndices.data(), m_indices.data(), numIndices, remapTable.data() );
            meshopt_remapVertexBuffer( remapVertices.data(), m_vertices.data(), numVertices, m_vertexStride, remapTable.data() );

            m_indices = eastl::move( remapIndices );
            m_vertices = eastl::move( remapVertices );

            numVertices = numRemappedVertices;
        }

        if ( flags.IsFlagSet( GeometryOptimizeFlags::VertexCache ) )
        {
            meshopt_optimizeVertexCache( m_indices.data(), m_indices.data(), m_indices.size(), numVertices );
        }

        if ( flags.IsFlagSet( GeometryOptimizeFlags::VertexFetch ) )
        {
            size_t newVertexCount = meshopt_optimizeVertexFetch
            (
                m_vertices.data(),
                m_indices.data(),
                m_indices.size(),
                m_vertices.data(),
                numVertices,
                m_vertexStride
            );

            m_vertices.resize( newVertexCount * m_vertexStride );
        }
    }

    //-------------------------------------------------------------------------

    AABB GeometryBuilder::ComputeAABB() const
    {
        EE_ASSERT( m_vertexStride );

        AABB aabb = {};

        PositionAttribute position = GetPositionAttribute( 0 );
        aabb.m_center = position.m_position;
        aabb.m_halfExtents = Vector::Zero;

        size_t numVertices = m_vertices.size() / m_vertexStride;

        for ( size_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex )
        {
            position = GetPositionAttribute( vertexIndex );
            aabb.AddPoint( position.m_position );
        }

        return aabb;
    }

    uint32_t GeometryBuilder::BuildClusters( AlignedBlob& packedMeshData ) const
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( packedMeshData.size() >= sizeof( MeshHeader ) );

        TVector<meshopt_Meshlet> tempMeshlets;
        TVector<uint32_t>        tempMeshletVertices;
        TVector<uint8_t>         tempMeshletTriangles;

        uint32_t meshNumClusters = 0;

        // Generate clusters
        //-------------------------------------------------------------------------

        size_t const meshletUpperBound = meshopt_buildMeshletsBound( m_indices.size(), MeshCluster::s_maxVerticesPerCluster, MeshCluster::s_maxTrianglesPerCluster );

        tempMeshlets.resize( meshletUpperBound );
        tempMeshletVertices.resize( meshletUpperBound * MeshCluster::s_maxTrianglesPerCluster );
        tempMeshletTriangles.resize( meshletUpperBound * MeshCluster::s_maxTrianglesPerCluster * 3 );

        size_t const numMeshlets = meshopt_buildMeshletsSpatial
        (
            tempMeshlets.data(),
            tempMeshletVertices.data(),
            tempMeshletTriangles.data(),
            m_indices.data(),
            m_indices.size(),
            reinterpret_cast<float const*>( m_vertices.data() ),
            m_vertices.size() / m_vertexStride,
            m_vertexStride,
            MeshCluster::s_maxVerticesPerCluster,
            MeshCluster::s_maxTrianglesPerCluster,
            MeshCluster::s_maxTrianglesPerCluster,
            1.0F
        );

        EE_ASSERT( numMeshlets > 0 );
        if ( numMeshlets == 0 )
        {
            return 0;
        }

        meshopt_Meshlet const& lastMeshlet = tempMeshlets[numMeshlets - 1];

        uint32_t const sectionTotalNumIndices = lastMeshlet.vertex_offset + lastMeshlet.vertex_count;
        uint32_t const sectionTotalNumTriangleIndices = lastMeshlet.triangle_offset + ( ( lastMeshlet.triangle_count * 3 + 3 ) & ~3 );

        tempMeshlets.resize( numMeshlets );
        tempMeshletVertices.resize( sectionTotalNumIndices );
        tempMeshletTriangles.resize( sectionTotalNumTriangleIndices );

        size_t const clusterBaseOffset = packedMeshData.size();
        packedMeshData.resize( clusterBaseOffset + numMeshlets * sizeof( MeshCluster ) );
        memset( packedMeshData.data() + clusterBaseOffset, 0, numMeshlets * sizeof( MeshCluster ) );

        meshNumClusters += uint32_t( numMeshlets );

        size_t dataCursor = packedMeshData.size();
        EE_ASSERT( ( dataCursor % 8 ) == 0 );

        // Optimize meshlets and compute the shared exponent
        //-------------------------------------------------------------------------

        int32_t const MinExponent = -13;
        int32_t const MaxOffsetBits = 16;

        int32_t compressedVertexExponent = MinExponent;

        for ( size_t meshletIndex = 0; meshletIndex < numMeshlets; ++meshletIndex )
        {
            meshopt_Meshlet const& meshlet = tempMeshlets[meshletIndex];

            meshopt_optimizeMeshletLevel
            (
                tempMeshletVertices.data() + meshlet.vertex_offset, meshlet.vertex_count,
                tempMeshletTriangles.data() + meshlet.triangle_offset, meshlet.triangle_count,
                3
            );

            meshopt_Bounds meshletBounds = meshopt_computeMeshletBounds
            (
                tempMeshletVertices.data() + meshlet.vertex_offset,
                tempMeshletTriangles.data() + meshlet.triangle_offset,
                meshlet.triangle_count,
                reinterpret_cast<float const*>( m_vertices.data() ),
                m_vertices.size() / m_vertexStride,
                m_vertexStride
            );

            Vector meshletCenter = Vector( meshletBounds.center );
            Float3 meshletBoxMin = meshletCenter - Vector( meshletBounds.radius );
            Float3 meshletBoxMax = meshletCenter + Vector( meshletBounds.radius );

            int32_t const clusterExponent = meshopt_computePositionExponent( &meshletBoxMin.m_x, &meshletBoxMax.m_x, MinExponent, MaxOffsetBits );
            compressedVertexExponent = Math::Max( compressedVertexExponent, clusterExponent );
        }

        float const vertexScale = ldexpf( 1.0F, compressedVertexExponent );

        // Quantize and serialize clusters
        //-------------------------------------------------------------------------
        for ( size_t meshletIndex = 0; meshletIndex < numMeshlets; ++meshletIndex )
        {
            meshopt_Meshlet const& meshlet = tempMeshlets[meshletIndex];

            meshopt_Bounds meshletBounds = meshopt_computeMeshletBounds
            (
                tempMeshletVertices.data() + meshlet.vertex_offset,
                tempMeshletTriangles.data() + meshlet.triangle_offset,
                meshlet.triangle_count,
                reinterpret_cast<float const*>( m_vertices.data() ),
                m_vertices.size() / m_vertexStride,
                m_vertexStride
            );

            // Quantize vertices
            TArray<Int3, 256> compressedVertexPositions = {};
            Int3 compressedVertexAnchor = Int3( INT_MAX, INT_MAX, INT_MAX );

            for ( size_t clusterVertexIndex = 0; clusterVertexIndex < meshlet.vertex_count; ++clusterVertexIndex )
            {
                EE_ASSERT( meshlet.vertex_offset + clusterVertexIndex < tempMeshletVertices.size() );

                uint32_t sourceVertexIndex = tempMeshletVertices[meshlet.vertex_offset + clusterVertexIndex];
                PositionAttribute srcPositionAttribute = GetPositionAttribute( sourceVertexIndex );

                Int3& compressedPosition = compressedVertexPositions[clusterVertexIndex];
                compressedPosition.m_x = Math::RoundToInt32( srcPositionAttribute.m_position.m_x / vertexScale );
                compressedPosition.m_y = Math::RoundToInt32( srcPositionAttribute.m_position.m_y / vertexScale );
                compressedPosition.m_z = Math::RoundToInt32( srcPositionAttribute.m_position.m_z / vertexScale );

                compressedVertexAnchor.m_x = Math::Min( compressedVertexAnchor.m_x, compressedPosition.m_x );
                compressedVertexAnchor.m_y = Math::Min( compressedVertexAnchor.m_y, compressedPosition.m_y );
                compressedVertexAnchor.m_z = Math::Min( compressedVertexAnchor.m_z, compressedPosition.m_z );
            }

            // Quantize the cluster AABB, the anchor is the AABB min so all offsets are non-negative
            Int3 compressedAABBMin = {};
            Int3 compressedAABBMax = {};
            compressedAABBMin.m_x = Math::RoundToInt32( ( meshletBounds.center[0] - meshletBounds.radius ) / vertexScale );
            compressedAABBMin.m_y = Math::RoundToInt32( ( meshletBounds.center[1] - meshletBounds.radius ) / vertexScale );
            compressedAABBMin.m_z = Math::RoundToInt32( ( meshletBounds.center[2] - meshletBounds.radius ) / vertexScale );
            compressedAABBMax.m_x = Math::RoundToInt32( ( meshletBounds.center[0] + meshletBounds.radius ) / vertexScale );
            compressedAABBMax.m_y = Math::RoundToInt32( ( meshletBounds.center[1] + meshletBounds.radius ) / vertexScale );
            compressedAABBMax.m_z = Math::RoundToInt32( ( meshletBounds.center[2] + meshletBounds.radius ) / vertexScale );

            compressedVertexAnchor.m_x = Math::Min( compressedVertexAnchor.m_x, compressedAABBMin.m_x );
            compressedVertexAnchor.m_y = Math::Min( compressedVertexAnchor.m_y, compressedAABBMin.m_y );
            compressedVertexAnchor.m_z = Math::Min( compressedVertexAnchor.m_z, compressedAABBMin.m_z );

            // Compute the bits per axis needed to store ( position - anchor ), up to 16 bits per axis
            uint32_t numPositionBitsX = 1;
            uint32_t numPositionBitsY = 1;
            uint32_t nymPositionBitsZ = 1;
            for ( size_t clusterVertexIndex = 0; clusterVertexIndex < meshlet.vertex_count; ++clusterVertexIndex )
            {
                Int3 const offset = compressedVertexPositions[clusterVertexIndex] - compressedVertexAnchor;
                numPositionBitsX = Math::Max( numPositionBitsX, MeshCluster::GetNumBitsRequired( uint32_t( offset.m_x ) ) );
                numPositionBitsY = Math::Max( numPositionBitsY, MeshCluster::GetNumBitsRequired( uint32_t( offset.m_y ) ) );
                nymPositionBitsZ = Math::Max( nymPositionBitsZ, MeshCluster::GetNumBitsRequired( uint32_t( offset.m_z ) ) );
            }
            EE_ASSERT( numPositionBitsX <= 16 && numPositionBitsY <= 16 && nymPositionBitsZ <= 16 );

            EE_ASSERT( clusterBaseOffset + ( meshletIndex + 1 ) * sizeof( MeshCluster ) <= packedMeshData.size() );
            EE_ASSERT( meshlet.vertex_count > 0 && meshlet.vertex_count <= MeshCluster::s_maxVerticesPerCluster );
            EE_ASSERT( meshlet.triangle_count > 0 && meshlet.triangle_count <= MeshCluster::s_maxTrianglesPerCluster );
            EE_ASSERT( meshlet.vertex_offset + meshlet.vertex_count <= tempMeshletVertices.size() );
            EE_ASSERT( meshlet.triangle_offset + meshlet.triangle_count * 3 <= tempMeshletTriangles.size() );

            size_t const positionBitsSize = ( ( meshlet.vertex_count * ( numPositionBitsX + numPositionBitsY + nymPositionBitsZ ) + 31 ) & ~size_t( 31 ) ) / 8;
            size_t const normalsOffset = positionBitsSize;
            size_t const normalsSize = meshlet.vertex_count * sizeof( MeshCluster::VertexNormalAttribute );
            size_t const uvsOffset = normalsOffset + ( ( normalsSize + 3 ) & ~size_t( 3 ) );
            size_t const uvsSize = meshlet.vertex_count * m_numTextureCoordinateAttributes * sizeof( MeshCluster::TextureCoordinateAttribute );
            size_t const colorsOffset = uvsOffset + uvsSize;
            size_t const colorsSize = meshlet.vertex_count * m_numColorAttributes * sizeof( MeshCluster::VertexColorAttribute );
            size_t const skinningOffset = colorsOffset + colorsSize;
            size_t const skinningSize = meshlet.vertex_count * m_numSkinningAttributes * sizeof( MeshCluster::SkinningAttribute );
            size_t const triangleDataSize = MeshCluster::GetTriangleDataSize( meshlet.triangle_count );

            size_t const clusterDataSize = ( skinningOffset + skinningSize + triangleDataSize + 7 ) & ~size_t( 7 );
            EE_ASSERT( ( clusterDataSize % 8 ) == 0 );
            EE_ASSERT( dataCursor <= UINT32_MAX );
            size_t const requiredSize = dataCursor + clusterDataSize;
            if ( requiredSize > packedMeshData.size() )
            {
                packedMeshData.resize( requiredSize );
            }

            MeshCluster* pCluster = reinterpret_cast<MeshCluster*>( packedMeshData.data() + clusterBaseOffset + meshletIndex * sizeof( MeshCluster ) );

            uint8_t* const pClusterData = packedMeshData.data() + dataCursor;
            uint8_t* const pPositionBits = pClusterData;
            uint8_t* const pNormals = pClusterData + normalsOffset;
            uint8_t* const pUVs = pClusterData + uvsOffset;
            uint8_t* const pColors = pClusterData + colorsOffset;
            uint8_t* const pSkinning = pClusterData + skinningOffset;
            uint8_t* const pTriangleBits = pClusterData + skinningOffset + skinningSize;

            uint8_t const* const pBufferEnd = packedMeshData.data() + packedMeshData.size();
            EE_ASSERT( pPositionBits + positionBitsSize + sizeof( uint64_t ) <= pBufferEnd );
            EE_ASSERT( pTriangleBits + triangleDataSize <= pBufferEnd );
            EE_ASSERT( pNormals + normalsSize <= pBufferEnd );
            EE_ASSERT( pUVs + uvsSize <= pBufferEnd );
            EE_ASSERT( pColors + colorsSize <= pBufferEnd );
            EE_ASSERT( pSkinning + skinningSize <= pBufferEnd );

            pCluster->SetNumVertices( uint32_t( meshlet.vertex_count ) );
            pCluster->SetNumTriangles( uint32_t( meshlet.triangle_count ) );
            pCluster->SetNumSkinningAttributes( m_numSkinningAttributes );
            pCluster->SetNumTextureCoordinateAttributes( m_numTextureCoordinateAttributes );
            pCluster->SetNumColorAttributes( m_numColorAttributes );
            pCluster->SetNumPositionBitsX( numPositionBitsX );
            pCluster->SetNumPositionBitsY( numPositionBitsY );
            pCluster->SetNumPositionBitsZ( nymPositionBitsZ );
            pCluster->SetAnchorAndExponent( compressedVertexAnchor, compressedVertexExponent );
            pCluster->SetAABB( compressedAABBMin, compressedAABBMax );
            pCluster->SetDataOffsetIn8ByteBlocks( uint32_t( dataCursor / 8 ) );

            // Position bits
            //-------------------------------------------------------------------------

            for ( size_t clusterVertexIndex = 0; clusterVertexIndex < meshlet.vertex_count; ++clusterVertexIndex )
            {
                MeshCluster::PackPositionBits
                (
                    pPositionBits,
                    uint32_t( clusterVertexIndex ),
                    compressedVertexPositions[clusterVertexIndex] - compressedVertexAnchor,
                    numPositionBitsX,
                    numPositionBitsY,
                    nymPositionBitsZ
                );
            }

            // Triangle bits
            //-------------------------------------------------------------------------

            for ( uint32_t meshletTriangleIndex = 0; meshletTriangleIndex < meshlet.triangle_count; ++meshletTriangleIndex )
            {
                EE_ASSERT( meshlet.triangle_offset + meshletTriangleIndex * 3 + 2 < tempMeshletTriangles.size() );

                uint8_t srcVertex0 = tempMeshletTriangles[meshlet.triangle_offset + meshletTriangleIndex * 3];
                uint8_t srcVertex1 = tempMeshletTriangles[meshlet.triangle_offset + meshletTriangleIndex * 3 + 1];
                uint8_t srcVertex2 = tempMeshletTriangles[meshlet.triangle_offset + meshletTriangleIndex * 3 + 2];

                MeshCluster::PackTriangleBits( pTriangleBits, meshletTriangleIndex, srcVertex0, srcVertex1, srcVertex2 );

                // Validate the triangle compression
                //-------------------------------------------------------------------------

                uint16_t unpackedVertex0 = 0;
                uint16_t unpackedVertex1 = 0;
                uint16_t unpackedVertex2 = 0;
                MeshCluster::UnpackTriangleBits( pTriangleBits, meshletTriangleIndex, unpackedVertex0, unpackedVertex1, unpackedVertex2 );
                EE_ASSERT( unpackedVertex0 == srcVertex0 && unpackedVertex1 == srcVertex1 && unpackedVertex2 == srcVertex2 );
            }

            // Compress vertices
            //-------------------------------------------------------------------------

            for ( size_t clusterVertexIndex = 0; clusterVertexIndex < meshlet.vertex_count; ++clusterVertexIndex )
            {
                EE_ASSERT( meshlet.vertex_offset + clusterVertexIndex < tempMeshletVertices.size() );

                uint32_t sourceVertexIndex = tempMeshletVertices[meshlet.vertex_offset + clusterVertexIndex];
                PositionAttribute srcPositionAttribute = GetPositionAttribute( sourceVertexIndex );

                MeshCluster::VertexNormalAttribute* pDstNormal = reinterpret_cast<MeshCluster::VertexNormalAttribute*>( pNormals + clusterVertexIndex * sizeof( MeshCluster::VertexNormalAttribute ) );
                *pDstNormal = {};
                pDstNormal->Initialize( srcPositionAttribute.m_normal );

                // Texture coordinates
                for ( uint32_t attributeIndex = 0; attributeIndex < m_numTextureCoordinateAttributes; ++attributeIndex )
                {
                    TextureCoordinateAttribute uv;
                    if ( GetTextureCoordinateAttribute( sourceVertexIndex, attributeIndex, uv ) )
                    {
                        MeshCluster::TextureCoordinateAttribute* pDstUV = reinterpret_cast<MeshCluster::TextureCoordinateAttribute*>( pUVs + ( attributeIndex * meshlet.vertex_count + clusterVertexIndex ) * sizeof( MeshCluster::TextureCoordinateAttribute ) );
                        *pDstUV = uv;
                    }
                }

                // Vertex colors
                for ( uint32_t attributeIndex = 0; attributeIndex < m_numColorAttributes; ++attributeIndex )
                {
                    ColorAttribute color;
                    if ( GetColorAttribute( sourceVertexIndex, attributeIndex, color ) )
                    {
                        MeshCluster::VertexColorAttribute* pDstColor = reinterpret_cast<MeshCluster::VertexColorAttribute*>( pColors + ( attributeIndex * meshlet.vertex_count + clusterVertexIndex ) * sizeof( MeshCluster::VertexColorAttribute ) );
                        *pDstColor = color;
                    }
                }

                // Skinning attributes
                for ( uint32_t attributeIndex = 0; attributeIndex < m_numSkinningAttributes; ++attributeIndex )
                {
                    SkinningAttribute skinning;
                    if ( GetSkinningAttribute( sourceVertexIndex, attributeIndex, skinning ) )
                    {
                        MeshCluster::SkinningAttribute* pDstSkinning = reinterpret_cast<MeshCluster::SkinningAttribute*>( pSkinning + ( attributeIndex * meshlet.vertex_count + clusterVertexIndex ) * sizeof( MeshCluster::SkinningAttribute ) );
                        pDstSkinning->m_boneIndices = skinning.m_boneIndices;
                        pDstSkinning->m_boneWeights = skinning.m_boneWeights;
                    }
                }

                // Validate compression
                //-------------------------------------------------------------------------

                #ifdef EE_DEBUG
                float const validationTolerance = Math::Max( 0.5F, ldexpf( 0.5F, compressedVertexExponent ) );
                Int3 const decompressedPosition = MeshCluster::UnpackPositionBits
                (
                    pPositionBits,
                    uint32_t( clusterVertexIndex ),
                    numPositionBitsX,
                    numPositionBitsY,
                    nymPositionBitsZ
                ) + compressedVertexAnchor;
                EE_ASSERT( Math::Abs( ldexpf( float( decompressedPosition.m_x ), compressedVertexExponent ) - srcPositionAttribute.m_position.m_x ) < validationTolerance );
                EE_ASSERT( Math::Abs( ldexpf( float( decompressedPosition.m_y ), compressedVertexExponent ) - srcPositionAttribute.m_position.m_y ) < validationTolerance );
                EE_ASSERT( Math::Abs( ldexpf( float( decompressedPosition.m_z ), compressedVertexExponent ) - srcPositionAttribute.m_position.m_z ) < validationTolerance );
                #endif
            }

            // Validate the AABB compression

            #ifdef EE_DEBUG
            float const validationTolerance = Math::Max( 0.5F, ldexpf( 0.5F, compressedVertexExponent ) );
            Float3 decompressedAABBMin, decompressedAABBMax;
            pCluster->GetAABB( decompressedAABBMin, decompressedAABBMax );
            EE_ASSERT( Math::Abs( decompressedAABBMin.m_x - ( meshletBounds.center[0] - meshletBounds.radius ) ) <= validationTolerance );
            EE_ASSERT( Math::Abs( decompressedAABBMin.m_y - ( meshletBounds.center[1] - meshletBounds.radius ) ) <= validationTolerance );
            EE_ASSERT( Math::Abs( decompressedAABBMin.m_z - ( meshletBounds.center[2] - meshletBounds.radius ) ) <= validationTolerance );
            EE_ASSERT( Math::Abs( decompressedAABBMax.m_x - ( meshletBounds.center[0] + meshletBounds.radius ) ) <= validationTolerance );
            EE_ASSERT( Math::Abs( decompressedAABBMax.m_y - ( meshletBounds.center[1] + meshletBounds.radius ) ) <= validationTolerance );
            EE_ASSERT( Math::Abs( decompressedAABBMax.m_z - ( meshletBounds.center[2] + meshletBounds.radius ) ) <= validationTolerance );
            #endif

            dataCursor += clusterDataSize;
        }

        EE_ASSERT( packedMeshData.size() == dataCursor );
        return meshNumClusters;
    }

    Geometry GeometryBuilder::BuildGeometry() const
    {
        Geometry geometry = {};

        geometry.m_meshData.resize( sizeof( MeshHeader ) );

        AABB clustersAABB = AABB( ComputeAABB() ); // TODO: use real algorithm to find minimal bounding box, for now use AABB
        geometry.m_bounds = OBB( clustersAABB );

        uint32_t const meshNumClusters = BuildClusters( geometry.m_meshData );

        MeshHeader& meshHeader = geometry.GetMeshHeader();
        meshHeader = {};
        meshHeader.m_numClusters = meshNumClusters;
        clustersAABB.GetCenter().StoreFloat3( meshHeader.m_aabbCenterLocal );
        clustersAABB.GetExtents().StoreFloat3( meshHeader.m_aabbHalfExtentsLocal );

        EE_ASSERT( geometry.m_meshData.size() >= sizeof( MeshHeader ) + meshHeader.m_numClusters * sizeof( MeshCluster ) );
        EE_ASSERT( meshHeader.m_numClusters == 0 || ( geometry.m_meshData.size() % 8 ) == 0 );

        return geometry;
    }
}

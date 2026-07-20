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

    //-------------------------------------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------------------------------------

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

    uint32_t GeometryBuilder::BuildAndAppendClusters( Blob& clusterVertices, size_t clusterVertexStride, TAlignedVector<uint32_t>& clusterTriangles, Blob& clusters ) const
    {
        EE_ASSERT( m_vertexStride );
        EE_ASSERT( ( clusterVertices.size() % clusterVertexStride ) == 0 );

        TVector<meshopt_Meshlet> tempMeshlets;
        TVector<uint32_t>        tempMeshletVertices;
        TVector<uint8_t>         tempMeshletTriangles;

        size_t   meshClusterOffset = clusters.size() / sizeof( MeshCluster );
        uint32_t meshNumClusters = 0;

        uint32_t meshBaseVertex = uint32_t( clusterVertices.size() / clusterVertexStride );
        uint32_t sectionBaseVertex = 0;

        // Generate clusters
        //-------------------------------------------------------------------------------------------------------------

        size_t const meshletUpperBound = meshopt_buildMeshletsBound( m_indices.size(), Geometry::MaxClusterVertices, Geometry::MaxClusterTriangles );

        tempMeshlets.resize( meshletUpperBound );
        tempMeshletVertices.resize( meshletUpperBound * Geometry::MaxClusterVertices );
        tempMeshletTriangles.resize( meshletUpperBound * Geometry::MaxClusterTriangles * 3 );

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
            Geometry::MaxClusterVertices,
            Geometry::MaxClusterTriangles,
            Geometry::MaxClusterTriangles,
            1.0F
        );

        meshopt_Meshlet const& lastMeshlet = tempMeshlets[numMeshlets - 1];

        uint32_t const sectionTotalNumIndices = lastMeshlet.vertex_offset + lastMeshlet.vertex_count;
        uint32_t const sectionTotalNumTriangleIndices = lastMeshlet.triangle_offset + ( ( lastMeshlet.triangle_count * 3 + 3 ) & ~3 );

        tempMeshlets.resize( numMeshlets );
        tempMeshletVertices.resize( sectionTotalNumIndices );
        tempMeshletTriangles.resize( sectionTotalNumTriangleIndices );

        size_t const currentClusterOffset = clusters.size() / sizeof( MeshCluster );
        clusters.resize( ( currentClusterOffset + numMeshlets ) * sizeof( MeshCluster ) );

        meshNumClusters += uint32_t( numMeshlets );

        // Optimize and compute exponent
        //---------------------------------------------------------------------------------------------------------

        int32_t const MinExponent = -20;
        int32_t const OffsetBits = 16;

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

            int32_t currentExponent = meshopt_computePositionExponent( &meshletBoxMin.m_x, &meshletBoxMax.m_x, MinExponent, OffsetBits );
            compressedVertexExponent = Math::Max( compressedVertexExponent, currentExponent );
        }

        float const vertexScale = ldexpf( 1.0F, compressedVertexExponent );

        // Quantize and serialize clusters
        //---------------------------------------------------------------------------------------------------------
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

            // Quantize cluster center as well
            Int3 compressedClusterCenter = {};
            compressedClusterCenter.m_x = Math::RoundToInt32( meshletBounds.center[0] / vertexScale );
            compressedClusterCenter.m_y = Math::RoundToInt32( meshletBounds.center[1] / vertexScale );
            compressedClusterCenter.m_z = Math::RoundToInt32( meshletBounds.center[2] / vertexScale );

            compressedVertexAnchor.m_x = Math::Min( compressedVertexAnchor.m_x, compressedClusterCenter.m_x );
            compressedVertexAnchor.m_y = Math::Min( compressedVertexAnchor.m_y, compressedClusterCenter.m_y );
            compressedVertexAnchor.m_z = Math::Min( compressedVertexAnchor.m_z, compressedClusterCenter.m_z );

            // Triangle buffer
            uint32_t const currentClusterTriangleOffset = uint32_t( clusterTriangles.size() );
            for ( uint32_t meshletTriangleIndex = 0; meshletTriangleIndex < meshlet.triangle_count; ++meshletTriangleIndex )
            {
                uint8_t srcVertex0 = tempMeshletTriangles[meshlet.triangle_offset + meshletTriangleIndex * 3];
                uint8_t srcVertex1 = tempMeshletTriangles[meshlet.triangle_offset + meshletTriangleIndex * 3 + 1];
                uint8_t srcVertex2 = tempMeshletTriangles[meshlet.triangle_offset + meshletTriangleIndex * 3 + 2];

                clusterTriangles.push_back( MeshCluster::PackTriangle( srcVertex0, srcVertex1, srcVertex2 ) );
            }

            // Cluster itself
            MeshCluster& cluster = *reinterpret_cast<MeshCluster*>( &clusters[( currentClusterOffset + meshletIndex ) * sizeof( MeshCluster )] );
            cluster.m_numVertices = uint8_t( meshlet.vertex_count );
            cluster.m_numTriangles = uint8_t( meshlet.triangle_count );
            cluster.m_vertexOffset = meshBaseVertex + sectionBaseVertex + meshlet.vertex_offset;
            cluster.m_triangleOffset = currentClusterTriangleOffset;
            cluster.SetAnchorAndExponent( compressedVertexAnchor, compressedVertexExponent );
            cluster.SetBoundingSphere( compressedClusterCenter, meshletBounds.radius );

            // Append vertex data
            clusterVertices.resize( clusterVertices.size() + meshlet.vertex_count * clusterVertexStride );

            // Compress vertices
            for ( size_t clusterVertexIndex = 0; clusterVertexIndex < meshlet.vertex_count; ++clusterVertexIndex )
            {
                uint32_t sourceVertexIndex = tempMeshletVertices[meshlet.vertex_offset + clusterVertexIndex];
                PositionAttribute srcPositionAttribute = GetPositionAttribute( sourceVertexIndex );

                StaticMeshVertex* pDstMeshVertex = reinterpret_cast<StaticMeshVertex*>( clusterVertices.data() + ( cluster.m_vertexOffset + clusterVertexIndex ) * clusterVertexStride );
                EE_ASSERT( reinterpret_cast<uint8_t*>( pDstMeshVertex ) <= ( clusterVertices.data() + clusterVertices.size() ) );

                *pDstMeshVertex = {};
                pDstMeshVertex->Initialize( compressedVertexPositions[clusterVertexIndex] - compressedVertexAnchor, srcPositionAttribute.m_normal );

                if ( TextureCoordinateAttribute uv0; GetTextureCoordinateAttribute( sourceVertexIndex, 0, uv0 ) )
                {
                    pDstMeshVertex->m_uv0 = uv0;
                }

                if ( TextureCoordinateAttribute uv1; GetTextureCoordinateAttribute( sourceVertexIndex, 1, uv1 ) )
                {
                    pDstMeshVertex->m_uv1 = uv1;
                }

                if ( ColorAttribute color; GetColorAttribute( sourceVertexIndex, 0, color ) )
                {
                    pDstMeshVertex->m_packedColor = color;
                }

                if ( SkinningAttribute skinning; GetSkinningAttribute( sourceVertexIndex, 0, skinning ) )
                {
                    SkeletalMeshVertex* pDstSkeletalMeshVertex = reinterpret_cast<SkeletalMeshVertex*>( pDstMeshVertex );

                    pDstSkeletalMeshVertex->m_boneIndices = skinning.m_boneIndices;
                    pDstSkeletalMeshVertex->m_boneWeights = skinning.m_boneWeights;
                }

                // Validate compression
                Float3 decompressedPosition = pDstMeshVertex->GetPosition( compressedVertexAnchor, compressedVertexExponent );
                EE_ASSERT( Math::Abs( decompressedPosition.m_x - srcPositionAttribute.m_position.m_x ) < 0.01F );
                EE_ASSERT( Math::Abs( decompressedPosition.m_y - srcPositionAttribute.m_position.m_y ) < 0.01F );
                EE_ASSERT( Math::Abs( decompressedPosition.m_z - srcPositionAttribute.m_position.m_z ) < 0.01F );
            }

            // Validate compression again
            Float4 decompressedBoundingSphere = cluster.GetBoundingSphere();
            EE_ASSERT( Math::Abs( decompressedBoundingSphere.m_x - meshletBounds.center[0] ) < 0.01F );
            EE_ASSERT( Math::Abs( decompressedBoundingSphere.m_y - meshletBounds.center[1] ) < 0.01F );
            EE_ASSERT( Math::Abs( decompressedBoundingSphere.m_z - meshletBounds.center[2] ) < 0.01F );
            EE_ASSERT( Math::Abs( decompressedBoundingSphere.m_w - meshletBounds.radius ) < 0.01F );
        }

        return meshNumClusters;
    }

    void GeometryBuilder::BuildAndAppendGeometry( Geometry& geometry ) const
    {
        uint32_t meshNumClusters = BuildAndAppendClusters
        (
            geometry.GetClusterVertices(),
            geometry.GetClusterVertexStride(),
            geometry.GetClusterTriangles(),
            geometry.GetClusters()
        );

        EE_ASSERT( geometry.GetNumClusters() == meshNumClusters );
        EE_ASSERT( ( geometry.m_clusters.size() % sizeof( MeshCluster ) ) == 0 );
    }
}

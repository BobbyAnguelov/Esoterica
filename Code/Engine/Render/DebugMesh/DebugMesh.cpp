#include "DebugMesh.h"
#include "Base/Types/HashMap.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS

namespace EE
{
    struct EdgeKey
    {
        inline bool operator==( EdgeKey const& rhs ) const { return m_edgeIndices[0] == rhs.m_edgeIndices[0] && m_edgeIndices[1] == rhs.m_edgeIndices[1]; }

        int32_t m_edgeIndices[2];
    };
}

namespace eastl
{
    template <typename T> struct hash;

    template <>
    struct hash<EE::EdgeKey>
    {
        size_t operator()( EE::EdgeKey const& key ) const { return ( size_t( key.m_edgeIndices[0] ) << 32 ) + key.m_edgeIndices[1]; }
    };
}

namespace EE
{
    // Create unit ico sphere
    static void MakeIcoSphereMesh( int32_t numSubDivisions, TVector<Float3>& outVertices, TVector<Int3>& outIndices )
    {
        auto SubDivide = [] ( TVector<Float3>& vertices, TVector<Int3>& triangles )
        {
            auto GetVertexForEdge = [] ( THashMap<EdgeKey, int32_t>& lookup, TVector<Float3>& vertices, int32_t first, int32_t second )
            {
                EdgeKey key = { first, second };
                if ( key.m_edgeIndices[0] > key.m_edgeIndices[1] )
                {
                    eastl::swap( key.m_edgeIndices[0], key.m_edgeIndices[1] );
                }

                auto inserted = lookup.try_emplace( key, (int32_t) vertices.size() );
                if ( inserted.second )
                {
                    Vector const edge0 = vertices[first];
                    Vector const edge1 = vertices[second];
                    Vector const point = ( edge0 + edge1 ).GetNormalized3();
                    vertices.push_back( point );
                }

                return inserted.first->second;
            };

            //-------------------------------------------------------------------------

            THashMap<EdgeKey, int32_t> lookup;
            TVector<Int3> newTriangles;

            for ( auto const& tri : triangles )
            {
                int32_t mid[3];
                for ( int32_t edge = 0; edge < 3; ++edge )
                {
                    mid[edge] = GetVertexForEdge( lookup, vertices, tri[edge], tri[( edge + 1 ) % 3] );
                }

                newTriangles.emplace_back( tri[0], mid[0], mid[2] );
                newTriangles.emplace_back( tri[1], mid[1], mid[0] );
                newTriangles.emplace_back( tri[2], mid[2], mid[1] );
                newTriangles.emplace_back( mid[0], mid[1], mid[2] );
            }

            triangles.swap( newTriangles );
        };

        //-------------------------------------------------------------------------

        constexpr static float const X = .525731112119133606f;
        constexpr static float const Z = .850650808352039932f;
        constexpr static float const N = 0.f;

        outVertices =
        {
          Float3( -X,N,Z ), Float3( X,N,Z ), Float3( -X,N,-Z ), Float3( X,N,-Z ),
          Float3( N,Z,X ), Float3( N,Z,-X ), Float3( N,-Z,X ), Float3( N,-Z,-X ),
          Float3( Z,X,N ), Float3( -Z,X, N ), Float3( Z,-X,N ), Float3( -Z,-X, N )
        };

        outIndices =
        {
            Int3( 0,4,1 ), Int3( 0,9,4 ), Int3( 9,5,4 ), Int3( 4,5,8 ), Int3( 4,8,1 ),
            Int3( 8,10,1 ), Int3( 8,3,10 ), Int3( 5,3,8 ), Int3( 5,2,3 ), Int3( 2,7,3 ),
            Int3( 7,10,3 ), Int3( 7,6,10 ), Int3( 7,11,6 ), Int3( 11,0,6 ), Int3( 0,1,6 ),
            Int3( 6,1,10 ), Int3( 9,0,11 ), Int3( 9,11,2 ), Int3( 9,2,5 ), Int3( 7,2,11 )
        };

        for ( int32_t i = 0; i < numSubDivisions; ++i )
        {
            SubDivide( outVertices, outIndices );
        }
    }

    //-------------------------------------------------------------------------

    static void MakeCylinderMesh( int32_t resolution, float radius, float halfHeight, bool isClosedCylinder, TVector<Float3>& outVertices, TVector<uint32_t>& outIndices )
    {
        outVertices.clear();

        // Edges
        //-------------------------------------------------------------------------

        TVector<uint32_t> topEdge;
        TVector<uint32_t> bottomEdge;
        for ( int32_t i = 0; i < resolution; i++ )
        {
            float ratio = static_cast<float>( i ) / ( resolution );
            float r = ratio * Math::TwoPi;
            float x = Math::Cos( r ) * radius;
            float y = Math::Sin( r ) * radius;

            outVertices.emplace_back( x, y, -halfHeight );
            bottomEdge.emplace_back( int32_t( outVertices.size() - 1 ) );

            outVertices.emplace_back( x, y, halfHeight );
            topEdge.emplace_back( int32_t( outVertices.size() - 1 ) );
        }

        // Edge Indices
        //-------------------------------------------------------------------------

        outIndices.clear();

        for ( int32_t i = 0; i < resolution; i++ )
        {
            auto i0 = i * 2;
            auto i1 = ( i0 + 2 ) % ( resolution * 2 );
            auto i2 = ( i0 + 3 ) % ( resolution * 2 );
            auto i3 = i0 + 1;

            outIndices.emplace_back( i0 );
            outIndices.emplace_back( i1 );
            outIndices.emplace_back( i2 );

            outIndices.emplace_back( i0 );
            outIndices.emplace_back( i2 );
            outIndices.emplace_back( i3 );
        }

        // Top/Bottom Indices
        //-------------------------------------------------------------------------

        if ( isClosedCylinder )
        {
            outVertices.emplace_back( 0, 0, halfHeight );
            uint32_t const topCenterIdx = uint32_t( outVertices.size() - 1 );

            outVertices.emplace_back( 0, 0, -halfHeight );
            uint32_t const bottomCenterIdx = uint32_t( outVertices.size() - 1 );

            uint32_t const numEdgeVertices = (uint32_t) topEdge.size();
            for ( uint32_t i = 0u; i < numEdgeVertices; i++ )
            {
                uint32_t const nextEdgeIdx = ( i + 1 ) % numEdgeVertices;

                outIndices.emplace_back( topEdge[i] );
                outIndices.emplace_back( topEdge[nextEdgeIdx] );
                outIndices.emplace_back( topCenterIdx );

                outIndices.emplace_back( bottomCenterIdx );
                outIndices.emplace_back( bottomEdge[nextEdgeIdx] );
                outIndices.emplace_back( bottomEdge[i] );
            }
        }
    }

    //-------------------------------------------------------------------------

    static void MakeHemisphereMesh( int32_t numSlices, int32_t numStacks, float radius, TVector<Float3>& outVertices, TVector<uint32_t>& outIndices )
    {
        // Preallocate vertex storage
        int32_t const numVertsPerSphereHalf = ( numStacks * numSlices ) + 1;
        outVertices.resize( numVertsPerSphereHalf );

        // Add top
        outVertices[0] = Float3( 0, 0, radius );
        uint32_t const topCenterIdx = 0;

        // Generate vertices per stack / slice
        int32_t vertIdx = 1;
        for ( int32_t i = 0; i < numStacks; i++ )
        {
            auto phi = Math::PiDivTwo * float( i + 1 ) / float( numStacks );
            for ( int32_t j = 0; j < numSlices; j++ )
            {
                float theta = Math::TwoPi * float( j ) / float( numSlices );
                float x = Math::SanitizeFloat( Math::Sin( phi ) * Math::Cos( theta ) * radius );
                float y = Math::SanitizeFloat( Math::Sin( phi ) * Math::Sin( theta ) * radius );
                float z = Math::SanitizeFloat( Math::Cos( phi ) );

                outVertices[vertIdx] = Float3( x, y, ( z * radius ) );
                vertIdx++;
            }
        }

        // Preallocate index storage
        int32_t const numIndicesPerSphereHalf = ( numSlices * 3 ) + ( ( numStacks - 1 ) * numSlices * 6 );
        outIndices.resize( numIndicesPerSphereHalf );

        // Create indices for the top and bottom
        int32_t indexIdx = 0;
        for ( int32_t i = 0; i < numSlices; ++i )
        {
            auto i0 = i + 1;
            auto i1 = ( i + 1 ) % numSlices + 1;

            outIndices[indexIdx] = topCenterIdx + i0;
            indexIdx++;

            outIndices[indexIdx] = topCenterIdx + i1;
            indexIdx++;

            outIndices[indexIdx] = topCenterIdx;
            indexIdx++;
        }

        // Add quads per stack / slice
        for ( int32_t j = 0; j < numStacks - 1; j++ )
        {
            auto j0 = j * numSlices + 1;
            auto j1 = ( j + 1 ) * numSlices + 1;

            for ( int32_t i = 0; i < numSlices; i++ )
            {
                auto topI0 = topCenterIdx + j0 + i;
                auto topI1 = topCenterIdx + j0 + ( i + 1 ) % numSlices;
                auto topI2 = topCenterIdx + j1 + ( i + 1 ) % numSlices;
                auto topI3 = topCenterIdx + j1 + i;

                outIndices[indexIdx] = topI2;
                indexIdx++;

                outIndices[indexIdx] = topI1;
                indexIdx++;

                outIndices[indexIdx] = topI0;
                indexIdx++;

                outIndices[indexIdx] = topI3;
                indexIdx++;

                outIndices[indexIdx] = topI2;
                indexIdx++;

                outIndices[indexIdx] = topI0;
                indexIdx++;
            }
        }
    }

    //-------------------------------------------------------------------------

    static void MakeCapsuleMesh( int32_t numSlices, int32_t numStacks, float radius, float halfHeight, TVector<Float3>& outVertices, TVector<uint32_t>& outIndices )
    {
        // Preallocate vertex storage
        int32_t const numVertsPerSphereHalf = ( ( numStacks - 1 ) * numSlices ) + 1;
        outVertices.resize( numVertsPerSphereHalf * 2 );

        // Add top and bottom vertex
        outVertices[0] = Float3( 0, 0, ( halfHeight + radius ) );
        outVertices[numVertsPerSphereHalf] = Float3( 0, 0, -( halfHeight + radius ) );
        uint32_t const topCenterIdx = 0;
        uint32_t const bottomCenterIdx = numVertsPerSphereHalf;

        // Generate vertices per stack / slice
        int32_t vertIdx = 1;
        for ( int32_t i = 0; i < numStacks - 1; i++ )
        {
            auto phi = Math::PiDivTwo * float( i + 1 ) / float( numStacks );
            for ( int32_t j = 0; j < numSlices; j++ )
            {
                float theta = Math::TwoPi * float( j ) / float( numSlices );
                float x = Math::Sin( phi ) * Math::Cos( theta ) * radius;
                float y = Math::Sin( phi ) * Math::Sin( theta ) * radius;
                float z = Math::Cos( phi );

                outVertices[vertIdx] = Float3( x, y, ( z * radius ) + halfHeight );
                outVertices[numVertsPerSphereHalf + vertIdx] = Float3( x, y, ( z * -radius ) - halfHeight );
                vertIdx++;
            }
        }

        // Preallocate index storage
        int32_t const numIndicesPerSphereHalf = ( numSlices * 3 ) + ( ( numStacks - 2 ) * numSlices * 6 );
        outIndices.resize( numIndicesPerSphereHalf * 2 );

        // Create indices for the top and bottom
        int32_t indexIdx = 0;
        for ( int32_t i = 0; i < numSlices; ++i )
        {
            auto i0 = i + 1;
            auto i1 = ( i + 1 ) % numSlices + 1;

            outIndices[indexIdx] = topCenterIdx + i0;
            outIndices[numIndicesPerSphereHalf + indexIdx] = bottomCenterIdx;
            indexIdx++;

            outIndices[indexIdx] = topCenterIdx + i1;
            outIndices[numIndicesPerSphereHalf + indexIdx] = bottomCenterIdx + i1;
            indexIdx++;

            outIndices[indexIdx] = topCenterIdx;
            outIndices[numIndicesPerSphereHalf + indexIdx] = bottomCenterIdx + i0;
            indexIdx++;
        }

        // Add quads per stack / slice
        for ( int32_t j = 0; j < numStacks - 2; j++ )
        {
            auto j0 = j * numSlices + 1;
            auto j1 = ( j + 1 ) * numSlices + 1;

            for ( int32_t i = 0; i < numSlices; i++ )
            {
                auto topI0 = topCenterIdx + j0 + i;
                auto topI1 = topCenterIdx + j0 + ( i + 1 ) % numSlices;
                auto topI2 = topCenterIdx + j1 + ( i + 1 ) % numSlices;
                auto topI3 = topCenterIdx + j1 + i;

                auto bottomI0 = bottomCenterIdx + j0 + i;
                auto bottomI1 = bottomCenterIdx + j0 + ( i + 1 ) % numSlices;
                auto bottomI2 = bottomCenterIdx + j1 + ( i + 1 ) % numSlices;
                auto bottomI3 = bottomCenterIdx + j1 + i;

                outIndices[indexIdx] = topI2;
                outIndices[numIndicesPerSphereHalf + indexIdx] = bottomI0;
                indexIdx++;

                outIndices[indexIdx] = topI1;
                outIndices[numIndicesPerSphereHalf + indexIdx] = bottomI1;
                indexIdx++;

                outIndices[indexIdx] = topI0;
                outIndices[numIndicesPerSphereHalf + indexIdx] = bottomI2;
                indexIdx++;

                outIndices[indexIdx] = topI3;
                outIndices[numIndicesPerSphereHalf + indexIdx] = bottomI0;
                indexIdx++;

                outIndices[indexIdx] = topI2;
                outIndices[numIndicesPerSphereHalf + indexIdx] = bottomI2;
                indexIdx++;

                outIndices[indexIdx] = topI0;
                outIndices[numIndicesPerSphereHalf + indexIdx] = bottomI3;
                indexIdx++;
            }
        }

        // Add cylinder portion indices
        uint32_t const startIdxForTopRing = numVertsPerSphereHalf - numSlices;
        uint32_t const startIdxForBottomRing = ( numVertsPerSphereHalf * 2 ) - numSlices;

        for ( int32_t i = 0; i < numSlices; i++ )
        {
            auto i0 = startIdxForTopRing + i;
            auto i1 = startIdxForBottomRing + i;
            auto i2 = startIdxForBottomRing + ( i + 1 ) % numSlices;
            auto i3 = startIdxForTopRing + ( i + 1 ) % numSlices;

            outIndices.emplace_back( i0 );
            outIndices.emplace_back( i1 );
            outIndices.emplace_back( i2 );

            outIndices.emplace_back( i0 );
            outIndices.emplace_back( i2 );
            outIndices.emplace_back( i3 );
        }
    }
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    void DebugMesh::TransformVertices( Transform const& transform )
    {
        for ( Vertex& v : m_vertices )
        {
            v.m_pos = transform.TransformPoint( v.m_pos );
        }
    }

    void DebugMesh::RotateVertices( Quaternion const& rotation )
    {
        for ( Vertex& v : m_vertices )
        {
            v.m_pos = rotation.RotateVector( v.m_pos );
        }
    }

    void DebugMesh::OffsetVertices( Vector const& offset )
    {
        for ( Vertex& v : m_vertices )
        {
            v.m_pos += offset;
        }
    }

    void DebugMesh::FlattenIndices( bool generateNormals )
    {
        EE_ASSERT( HasIndices() );
        size_t const numIndices = m_indices.size();
        EE_ASSERT( numIndices % 3 == 0 );

        TVector<DebugMesh::Vertex> flattenedVertices;
        for ( int32_t i = 0; i < numIndices; i++ )
        {
            flattenedVertices.emplace_back( m_vertices[m_indices[i]] );
        }

        m_vertices.swap( flattenedVertices );
        m_indices.clear();

        //-------------------------------------------------------------------------

        if ( generateNormals )
        {
            int32_t const numVertices = (int32_t) m_vertices.size();
            EE_ASSERT( numVertices % 3 == 0 );
            for ( int32_t i = 0; i < numVertices; i += 3 )
            {
                Vector const& v0 = m_vertices[i].m_pos;
                Vector const& v1 = m_vertices[i + 1].m_pos;
                Vector const& v2 = m_vertices[i + 2].m_pos;

                Vector const s0 = ( v1 - v0 ).GetNormalized3();
                Vector const s1 = ( v2 - v1 ).GetNormalized3();
                Vector const normal = s0.Cross3( s1 ).GetNormalized3();

                m_vertices[i].m_normal = normal;
                m_vertices[i + 1].m_normal = normal;
                m_vertices[i + 2].m_normal = normal;
            }
        }
    }

    // Mesh Generators
    //-------------------------------------------------------------------------

    void DebugMesh::CreateSphere( Vector const& offset, float radius, DebugMesh& outMesh )
    {
        TVector<Float3> vertices;
        TVector<Int3> indices;
        MakeIcoSphereMesh( 3, vertices, indices );

        Vector const vRadius( radius );

        outMesh.m_vertices.clear();
        for ( auto const& v : vertices )
        {
            outMesh.m_vertices.emplace_back( ( v * vRadius ) + offset );
        }

        outMesh.m_indices.clear();
        for ( auto const& i : indices )
        {
            outMesh.m_indices.emplace_back( i[0] );
            outMesh.m_indices.emplace_back( i[1] );
            outMesh.m_indices.emplace_back( i[2] );
        }

        outMesh.FlattenIndices();
    }

    void DebugMesh::CreateHemisphere( Quaternion orientation, Vector const& offset, float radius, DebugMesh& outMesh )
    {
        TVector<Float3> vertices;
        TVector<uint32_t> indices;
        MakeHemisphereMesh( 16, 10, radius, vertices, indices );

        Vector const vRadius( radius );

        outMesh.m_vertices.clear();
        for ( auto const& v : vertices )
        {
            outMesh.m_vertices.emplace_back( ( v * vRadius ) + offset );
        }

        outMesh.m_indices.clear();
        for ( auto const& i : indices )
        {
            outMesh.m_indices.emplace_back( i );
        }

        if ( !orientation.IsIdentity() )
        {
            outMesh.RotateVertices( orientation );
        }

        if ( !offset.IsNearZero3() )
        {
            outMesh.OffsetVertices( offset );
        }

        outMesh.FlattenIndices();
    }

    void DebugMesh::CreateBox( Quaternion orientation, Vector const& offset, Float3 const& extents, DebugMesh& outMesh )
    {
        outMesh.m_vertices.clear();
        outMesh.m_indices.clear();

        Vector verts[8] =
        {
            ( Vector( -1.0f, -1.0f, -1.0f ) * extents ) + offset,  // 0 BFL
            ( Vector( -1.0f, 1.0f, -1.0f ) * extents ) + offset,   // 1 TFL
            ( Vector( 1.0f, 1.0f, -1.0f ) * extents ) + offset,    // 2 TFR
            ( Vector( 1.0f, -1.0f, -1.0f ) * extents ) + offset,   // 3 BFR
            ( Vector( -1.0f, -1.0f, 1.0f ) * extents ) + offset,   // 4 BBL
            ( Vector( -1.0f, 1.0f, 1.0f ) * extents ) + offset,    // 5 TBL
            ( Vector( 1.0f, 1.0f, 1.0f ) * extents ) + offset,     // 6 TBR
            ( Vector( 1.0f, -1.0f, 1.0f ) * extents ) + offset,    // 7 BBR
        };

        Vector normals[6] =
        {
            Vector( 1, 0, 0 ),
            Vector( 0, 1, 0 ),
            Vector( 0, 0, 1 ),
            Vector( -1, 0, 0 ),
            Vector( 0, -1, 0 ),
            Vector( 0, 0, -1 ),
        };

        outMesh.m_vertices =
        {
            // Front : -y
            { verts[0], normals[4], Colors::White },
            { verts[1], normals[4], Colors::White },
            { verts[3], normals[4], Colors::White },
            { verts[3], normals[4], Colors::White },
            { verts[1], normals[4], Colors::White },
            { verts[2], normals[4], Colors::White },

            // Right : -x
            { verts[3], normals[3], Colors::White },
            { verts[2], normals[3], Colors::White },
            { verts[7], normals[3], Colors::White },
            { verts[7], normals[3], Colors::White },
            { verts[2], normals[3], Colors::White },
            { verts[6], normals[3], Colors::White },

            // Back : y
            { verts[7], normals[1], Colors::White },
            { verts[6], normals[1], Colors::White },
            { verts[4], normals[1], Colors::White },
            { verts[4], normals[1], Colors::White },
            { verts[6], normals[1], Colors::White },
            { verts[5], normals[1], Colors::White },

            // Left: x
            { verts[4], normals[0], Colors::White },
            { verts[5], normals[0], Colors::White },
            { verts[0], normals[0], Colors::White },
            { verts[0], normals[0], Colors::White },
            { verts[5], normals[0], Colors::White },
            { verts[1], normals[0], Colors::White },

            // Top: z
            { verts[1], normals[2], Colors::White },
            { verts[5], normals[2], Colors::White },
            { verts[2], normals[2], Colors::White },
            { verts[2], normals[2], Colors::White },
            { verts[5], normals[2], Colors::White },
            { verts[6], normals[2], Colors::White },

            // Bottom: -z
            { verts[4], normals[5], Colors::White },
            { verts[0], normals[5], Colors::White },
            { verts[7], normals[5], Colors::White },
            { verts[7], normals[5], Colors::White },
            { verts[0], normals[5], Colors::White },
            { verts[3], normals[5], Colors::White },
        };

        if ( !orientation.IsIdentity() )
        {
            outMesh.RotateVertices( orientation );
        }
    }

    void DebugMesh::CreateCylinder( Quaternion orientation, Vector const& offset, float radius, float halfHeight, DebugMesh& outMesh )
    {
        outMesh.m_vertices.clear();
        outMesh.m_indices.clear();

        TVector<Float3> vertices;
        MakeCylinderMesh( 16, radius, halfHeight, true, vertices, outMesh.m_indices );

        outMesh.m_vertices.clear();
        for ( auto const& v : vertices )
        {
            outMesh.m_vertices.emplace_back( v );
        }

        if ( !orientation.IsIdentity() )
        {
            outMesh.RotateVertices( orientation );
        }

        if ( !offset.IsNearZero3() )
        {
            outMesh.OffsetVertices( offset );
        }

        outMesh.FlattenIndices();
    }

    void DebugMesh::CreateOpenCylinder( Quaternion orientation, Vector const& offset, float radius, float halfHeight, DebugMesh& outMesh )
    {
        outMesh.m_vertices.clear();
        outMesh.m_indices.clear();

        TVector<Float3> vertices;
        MakeCylinderMesh( 16, radius, halfHeight, false, vertices, outMesh.m_indices );

        outMesh.m_vertices.clear();
        for ( auto const& v : vertices )
        {
            outMesh.m_vertices.emplace_back( v );
        }

        if ( !orientation.IsIdentity() )
        {
            outMesh.RotateVertices( orientation );
        }

        if ( !offset.IsNearZero3() )
        {
            outMesh.OffsetVertices( offset );
        }

        outMesh.FlattenIndices();
    }

    void DebugMesh::CreateCapsule( Quaternion orientation, Vector const& offset, float radius, float halfHeight, DebugMesh& outMesh )
    {
        outMesh.m_vertices.clear();
        outMesh.m_indices.clear();

        TVector<Float3> vertices;
        MakeCapsuleMesh( 16, 10, radius, halfHeight, vertices, outMesh.m_indices );

        outMesh.m_vertices.clear();
        for ( auto const& v : vertices )
        {
            outMesh.m_vertices.emplace_back( v );
        }

        if ( !orientation.IsIdentity() )
        {
            outMesh.RotateVertices( orientation );
        }

        if ( !offset.IsNearZero3() )
        {
            outMesh.OffsetVertices( offset );
        }

        outMesh.FlattenIndices();
    }
}
#endif
#pragma once
#include "Engine/_Module/API.h"
#include "Base/Math/Vector.h"
#include "Base/Types/Color.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    struct EE_ENGINE_API DebugMesh
    {
        struct Vertex
        {
            Vertex() = default;
            Vertex( Vector const& pos, Vector const& normal = Vector::UnitZ, Color color = Colors::White ) : m_pos( pos ), m_normal( normal ), m_color( color ) {}

        public:

            Vector  m_pos;
            Vector  m_normal;
            Color   m_color = Colors::White;
        };

        // Create an ico-sphere mesh
        static void CreateSphere( Vector const& offset, float radius, DebugMesh& outMesh );

        // Create a northern hemisphere UV-sphere mesh ( z-up = north)
        static void CreateHemisphere( Quaternion orientation, Vector const& offset, float radius, DebugMesh& outMesh );

        // Create a capsule mesh - half-height is along the z-axis
        static void CreateCapsule( Quaternion orientation, Vector const& offset, float radius, float halfHeight, DebugMesh& outMesh );

        // Create a cylinder mesh - half-height is along the z-axis
        static void CreateCylinder( Quaternion orientation, Vector const& offset, float radius, float halfHeight, DebugMesh& outMesh );

        // Create an open cylinder mesh (a tube) - half-height is along the z-axis
        static void CreateOpenCylinder( Quaternion orientation, Vector const& offset, float radius, float halfHeight, DebugMesh& outMesh );

        // Create a box
        static void CreateBox( Quaternion orientation, Vector const& offset, Float3 const& extents, DebugMesh& outMesh );

    public:

        void Clear() { m_indices.clear(); m_vertices.clear(); }

        inline bool IsValid() const
        {
            if ( HasIndices() )
            {
                return m_vertices.size() >= 3 && ( m_indices.size() % 3 == 0 );
            }

            return m_vertices.size() >= 3 && ( m_vertices.size() % 3 == 0 );
        }

        // Transform all vertices
        void TransformVertices( Transform const& transform );

        // Rotate all vertices
        void RotateVertices( Quaternion const& rotation );

        // Rotate all vertices
        void OffsetVertices( Vector const& offset );

        // Did we provide indices?
        inline bool HasVertices() const { return !m_vertices.empty(); }

        // Did we provide indices?
        inline bool HasIndices() const { return !m_indices.empty(); }

        // Swap data with another debug mesh
        void Swap( DebugMesh& rhs )
        {
            m_vertices.swap( rhs.m_vertices );
            m_indices.swap( rhs.m_indices );
        }

        // Flatten out the index buffer
        void FlattenIndices( bool generateNormals = true );

    public:

        TVector<Vertex>         m_vertices;
        TVector<uint32_t>       m_indices; // Optional
    };
}
#endif
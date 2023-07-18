#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Base/Render/RenderBuffer.h"
#include "Base/Render/RenderStates.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------
// Base Mesh Data
//-------------------------------------------------------------------------
// The raw vertices/indices for a specific mesh
//
// Notes:
// * EE uses CCW to determine the facing direction
// * Meshes use the triangle list topology

namespace EE::Render
{
    class EE_ENGINE_API Mesh : public Resource::IResource
    {
        friend class MeshCompiler;
        friend class MeshLoader;

        EE_SERIALIZE( m_vertices, m_indices, m_sections, m_materials, m_vertexBuffer, m_indexBuffer, m_bounds );

    public:

        struct EE_ENGINE_API GeometrySection
        {
            EE_SERIALIZE( m_ID, m_startIndex, m_numIndices );

            GeometrySection() = default;
            GeometrySection( StringID ID, uint32_t startIndex, uint32_t numIndices );

            StringID                        m_ID;
            uint32_t                        m_startIndex = 0;
            uint32_t                        m_numIndices = 0;
        };

    public:

        virtual bool IsValid() const override
        {
            return m_indexBuffer.IsValid() && m_vertexBuffer.IsValid();
        }

        // Bounds
        inline OBB const& GetBounds() const { return m_bounds; }

        // Vertices
        inline Blob const& GetVertexData() const { return m_vertices; }
        inline int32_t GetNumVertices() const { return m_vertexBuffer.m_byteSize / m_vertexBuffer.m_byteStride; }
        inline VertexFormat const& GetVertexFormat() const { return m_vertexBuffer.m_vertexFormat; }
        inline RenderBuffer const& GetVertexBuffer() const { return m_vertexBuffer; }

        // Indices
        inline TVector<uint32_t> const& GetIndices() const { return m_indices; }
        inline int32_t GetNumIndices() const { return (int32_t) m_indices.size(); }
        inline RenderBuffer const& GetIndexBuffer() const { return m_indexBuffer; }

        // Mesh Sections
        inline TVector<GeometrySection> const& GetSections() const { return m_sections; }
        inline uint32_t GetNumSections() const { return (uint32_t) m_sections.size(); }
        inline GeometrySection GetSection( uint32_t i ) const { EE_ASSERT( i < GetNumSections() ); return m_sections[i]; }

        // Materials
        TVector<TResourcePtr<Material>> const& GetMaterials() const { return m_materials; }

        // Debug
        #if EE_DEVELOPMENT_TOOLS
        void DrawNormals( Drawing::DrawContext& drawingContext, Transform const& worldTransform ) const;
        #endif

    protected:

        Blob                                m_vertices;
        TVector<uint32_t>                   m_indices;
        TVector<GeometrySection>            m_sections;
        TVector<TResourcePtr<Material>>     m_materials;
        VertexBuffer                        m_vertexBuffer;
        RenderBuffer                        m_indexBuffer;
        OBB                                 m_bounds;
    };
}
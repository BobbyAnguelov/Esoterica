#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderGeometry.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Math/Vector.h"
#include "Base/Types/Color.h"
#include "Base/Types/BitFlags.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class GeometryOptimizeFlags
    {
        VertexRemap,
        VertexCache,
        VertexFetch,
    };

    class EE_ENGINE_API GeometryBuilder final
    {
    public:

        // Attributes
        //-------------------------------------------------------------------------

        struct PositionAttribute final
        {
            Float3              m_position = Float3::Zero;
            Float3              m_normal = Float3::Zero;

            inline PositionAttribute() = default;
            inline PositionAttribute( Float3 position, Float3 normal ) : m_position( position ), m_normal( normal ) {}
            inline PositionAttribute( Float4 position, Float4 normal ) : m_position( position ), m_normal( normal ) {}
            inline PositionAttribute( Vector position, Vector normal ) : m_position( position.ToFloat3() ), m_normal( normal.ToFloat3() ) {}
        };

        using TextureCoordinateAttribute = Float2;
        using ColorAttribute = Color;

        struct SkinningAttribute final
        {
            Int4                m_boneIndices = {};
            Float4              m_boneWeights = {};
        };

    public:

        inline void SetNumTextureCoordinateAttributes( uint32_t numTextureCoordinateAttributes ) { m_numTextureCoordinateAttributes = numTextureCoordinateAttributes; }
        inline void SetNumColorAttributes( uint32_t numColorAttributes ) { m_numColorAttributes = numColorAttributes; }
        inline void SetNumSkinningAttributes( uint32_t numSkinningAttributes ) { m_numSkinningAttributes = numSkinningAttributes; }

        inline size_t GetTextureCoordinateAttributesOffset() const { return sizeof( PositionAttribute ); }
        inline size_t GetColorAttributesOffset() const { return GetTextureCoordinateAttributesOffset() + m_numTextureCoordinateAttributes * sizeof( TextureCoordinateAttribute ); }
        inline size_t GetSkinningAttributesOffset() const { return GetColorAttributesOffset() + m_numColorAttributes * sizeof( ColorAttribute ); }

        inline void InitializeVertexFormat()
        {
            m_vertexStride = sizeof( PositionAttribute )
                + m_numTextureCoordinateAttributes * sizeof( TextureCoordinateAttribute )
                + m_numColorAttributes * sizeof( ColorAttribute )
                + m_numSkinningAttributes * sizeof( SkinningAttribute );
        }

        void SetIndices( TArrayView<uint32_t const> indices, bool flipWinding );
        void SetNumVertices( size_t numVertices );

        //-------------------------------------------------------------------------

        void SetPositionAttribute( size_t vertexIndex, PositionAttribute const& positionAttribute );
        PositionAttribute GetPositionAttribute( size_t vertexIndex ) const;

        void SetTextureCoordinateAttribute( size_t vertexIndex, size_t attributeIndex, TextureCoordinateAttribute const& textureCoordinateAttribute );
        TextureCoordinateAttribute GetTextureCoordinateAttribute( size_t vertexIndex, size_t attributeIndex ) const;
        bool GetTextureCoordinateAttribute( size_t vertexIndex, size_t attributeIndex, TextureCoordinateAttribute& textureCoordinateAttribute ) const;

        void SetColorAttribute( size_t vertexIndex, size_t attributeIndex, ColorAttribute const& colorAttribute );
        ColorAttribute GetColorAttribute( size_t vertexIndex, size_t attributeIndex ) const;
        bool GetColorAttribute( size_t vertexIndex, size_t attributeIndex, ColorAttribute& colorAttribute ) const;

        void SetSkinningAttribute( size_t vertexIndex, size_t attributeIndex, SkinningAttribute const& skinningAttribute );
        SkinningAttribute GetSkinningAttribute( size_t vertexIndex, size_t attributeIndex ) const;
        bool GetSkinningAttribute( size_t vertexIndex, size_t attributeIndex, SkinningAttribute& skinningAttribute ) const;

        //-------------------------------------------------------------------------

        bool Simplify( float normalsWeight, float uvWeight, float targetAttributeError, uint32_t targetTriangleCount, float targetTrianglePercentage );

        void Optimize( TBitFlags<GeometryOptimizeFlags> flags = TBitFlags<GeometryOptimizeFlags>( GeometryOptimizeFlags::VertexCache, GeometryOptimizeFlags::VertexFetch ) );

        //-------------------------------------------------------------------------

        AABB ComputeAABB() const;
        uint32_t BuildAndAppendClusters( Blob& clusterVertices, size_t clusterVertexStride, TAlignedVector<uint32_t>& clusterTriangles, Blob& clusters ) const;

        void BuildAndAppendGeometry( Geometry& geometry ) const;

        //-------------------------------------------------------------------------

        inline uint32_t GetVertexStride() const { return m_vertexStride; }
        inline uint32_t GetNumVertices() const { return uint32_t( m_vertices.size() / m_vertexStride ); }
        inline uint32_t GetNumIndices() const { return uint32_t( m_indices.size() ); }

        inline TAlignedVector<uint8_t> const& GetVertices() const { return m_vertices; }
        inline TAlignedVector<uint32_t> const& GetIndices() const { return m_indices; }

    private:

        uint32_t                    m_numTextureCoordinateAttributes = 0;
        uint32_t                    m_numColorAttributes = 0;
        uint32_t                    m_numSkinningAttributes = 0;

        uint32_t                    m_vertexStride = 0;

        TAlignedVector<uint8_t>     m_vertices;
        TAlignedVector<uint32_t>    m_indices;
    };
}

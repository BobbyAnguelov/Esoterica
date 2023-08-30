#pragma once

#include "RawSkeleton.h"
#include "Base/Types/Arrays.h"
#include "Base/Math/Matrix.h"
#include "Base/Types/String.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class EE_ENGINETOOLS_API RawMesh : public RawAsset
    {

    public:

        struct VertexData
        {
            VertexData() = default;

            bool operator==( VertexData const& rhs ) const;

            inline bool operator!=( VertexData const& rhs ) const
            {
                return !( *this == rhs );
            }

        public:

            Float4                              m_position = Float4::Zero;
            Float4                              m_color = Float4::Zero;
            Float4                              m_normal = Float4::Zero;
            Float4                              m_tangent = Float4::Zero;
            Float4                              m_binormal = Float4::Zero;
            TInlineVector<Float2, 3>            m_texCoords;

            // Optional skinning data
            TVector<int32_t>                    m_boneIndices;
            TVector<float>                      m_boneWeights;
        };

        //-------------------------------------------------------------------------

        struct GeometrySection
        {
            GeometrySection() = default;

            inline uint32_t GetNumTriangles() const { return (uint32_t) m_indices.size() / 3; }
            inline int32_t GetNumUVChannels() const { return m_numUVChannels; }

        public:

            String                              m_name;
            StringID                            m_materialNameID;
            TVector<VertexData>                 m_vertices;
            TVector<uint32_t>                   m_indices;
            int32_t                             m_numUVChannels = 0;

            bool                                m_clockwiseWinding = false;
        };

    public:

        RawMesh() = default;
        virtual bool IsValid() const override final;

        inline int32_t GetNumGeometrySections() const { return (int32_t) m_geometrySections.size(); }
        inline TVector<GeometrySection> const& GetGeometrySections() const { return m_geometrySections; }

        inline bool IsSkeletalMesh() const { return m_isSkeletalMesh; }
        inline RawSkeleton const& GetSkeleton() const { EE_ASSERT( IsSkeletalMesh() ); return m_skeleton; }
        inline int32_t GetNumBones() const { EE_ASSERT( IsSkeletalMesh() ); return m_skeleton.GetNumBones(); }
        inline int32_t GetNumBoneInfluencesPerVertex() const { EE_ASSERT( IsSkeletalMesh() ); return m_maxNumberOfBoneInfluences; }

        // Get a unique name for a new geometry section
        String GetUniqueGeometrySectionName( String const& desiredName ) const;

        // Merge all geometry sections together that have the same name
        void MergeGeometrySectionsByMaterial();

        // Apply scale to this mesh
        void ApplyScale( Float3 const& scale );

    protected:

        TVector<GeometrySection>                m_geometrySections;
        RawSkeleton                             m_skeleton;
        int32_t                                 m_maxNumberOfBoneInfluences = 0;
        bool                                    m_isSkeletalMesh = false;
    };
}
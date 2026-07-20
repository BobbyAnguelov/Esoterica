#pragma once

#include "ImportedSkeleton.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API Mesh : public ImportedData
    {
    public:

        constexpr static uint32_t const s_maxNumOfBoneInfluences = 8;
        constexpr static uint32_t const s_maxNumOfTextureCoords = 4;

        struct VertexData
        {
        public:

            VertexData() {}

            inline void Reset() { *this = VertexData(); }

            bool operator==( VertexData const& rhs ) const { return memcmp( this, &rhs, sizeof( VertexData ) ) == 0; }

        public:

            Float4                              m_position = Float4::Zero;
            Float4                              m_color = Float4::Zero;
            Float4                              m_normal = Float4::Zero;
            Float4                              m_tangent = Float4::Zero;
            Float4                              m_binormal = Float4::Zero;
            Float2                              m_texCoords[s_maxNumOfTextureCoords] = { Float2::Zero, Float2::Zero };

            // Optional skinning data
            uint32_t                            m_numBoneWeights = 0;
            int32_t                             m_boneIndices[s_maxNumOfBoneInfluences] = { -1,-1,-1,-1,-1,-1,-1,-1 };
            float                               m_boneWeights[s_maxNumOfBoneInfluences] = { 0 };
        };

        // Geometry Data
        //-------------------------------------------------------------------------
        // Can be shared across multiple sub-meshes

        struct Geometry
        {
            Geometry() = default;

            inline uint32_t GetNumTriangles() const { return (uint32_t) m_indices.size() / 3; }
            inline int32_t GetNumUVChannels() const { return m_numUVChannels; }

        public:

            StringID                            m_ID;
            TVector<VertexData>                 m_vertices;
            TVector<uint32_t>                   m_indices;
            uint32_t                            m_numUVChannels = 0;
            uint32_t                            m_numBoneInfluences = 0; // Multiple of 4 and less than s_maxNumOfBoneInfluences

            bool                                m_clockwiseWinding = false;
        };

        // A part of the mesh
        //-------------------------------------------------------------------------
        // Consists of a geometry, transform and material assignment

        struct Submesh
        {
            StringID                            m_ID;
            Matrix                              m_transform = Matrix::Identity;
            int32_t                             m_geometryIdx = InvalidIndex;
            StringID                            m_materialID;
        };

    public:

        Mesh() = default;
        virtual bool IsValid() const override final;

        void Finalize();

        inline int32_t GetNumGeometries() const { return (int32_t) m_geometries.size(); }
        inline TVector<Geometry> const& GetGeometries() const { return m_geometries; }
        inline TVector<Geometry>& GetGeometries() { return m_geometries; }

        inline bool IsSkeletalMesh() const { return m_isSkeletalMesh; }
        inline Skeleton const& GetSkeleton() const { EE_ASSERT( IsSkeletalMesh() ); return m_skeleton; }
        inline int32_t GetNumBones() const { EE_ASSERT( IsSkeletalMesh() ); return (int32_t) m_skeleton.GetNumBones(); }

        // Get the total number of mesh instances
        uint32_t GetNumSubmeshes() const { return (uint32_t) m_submeshes.size(); }

        // Get the mesh instances contain needed for this mesh
        TVector<Submesh> const& GetSubmeshes() const { return m_submeshes; }

        // Get the required number of influence, this will be a multiple of 4 and less than s_maxNumOfBoneInfluences
        uint32_t GetMaxNumberOfBoneInfluencesPerVertex() const { EE_ASSERT( IsSkeletalMesh() ); return m_maxNumberOfBoneInfluences; }

        // Apply scale to this mesh
        void ApplyScale( Float3 const& scale );

    protected:

        Skeleton                                m_skeleton;
        TVector<Geometry>                       m_geometries;
        TVector<Submesh>                        m_submeshes;
        uint32_t                                m_maxNumberOfBoneInfluences = 0;
        bool                                    m_isSkeletalMesh = false;
    };
}
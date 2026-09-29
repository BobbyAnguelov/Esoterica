#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderGeometry.h"
#include "Engine/Render/RenderProxies.h"
#include "Base/Types/StringID.h"
#include "Base/Math/Matrix43.h"

//-------------------------------------------------------------------------

namespace EE
{
    class DebugDrawContext;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct EE_ENGINE_API MeshStatistics final
    {
        void Accumulate( MeshStatistics const& src );

        //-------------------------------------------------------------------------

        EE_SERIALIZE
        (
            m_numVertices, m_numTriangles, m_numClusters,
            m_compressedSizeBytes, m_uncompressedSizeBytes, m_compressionRatio,
            m_averagePositionBitsPerAxisX, m_averagePositionBitsPerAxisY, m_averagePositionBitsPerAxisZ,
            m_minimumPositionBitsPerAxisX, m_minimumPositionBitsPerAxisY, m_minimumPositionBitsPerAxisZ,
            m_medianPositionBitsPerAxisX, m_medianPositionBitsPerAxisY, m_medianPositionBitsPerAxisZ,
            m_maximumPositionBitsPerAxisX, m_maximumPositionBitsPerAxisY, m_maximumPositionBitsPerAxisZ,
            m_minimumPositionBitsPerVertex, m_averagePositionBitsPerVertex, m_maximumPositionBitsPerVertex,
            m_minimumClusterUtilization, m_maximumClusterUtilization, m_averageClusterUtilization, m_medianClusterUtilization,
            m_clusterVertexOverhead, m_vertexTriangleReuse,
            m_minimumCompressionAccuracy, m_averageCompressionAccuracy
        );

        uint32_t                        m_numVertices = 0;
        uint32_t                        m_numTriangles = 0;
        uint32_t                        m_numClusters = 0;

        uint64_t                        m_compressedSizeBytes = 0;
        uint64_t                        m_uncompressedSizeBytes = 0;
        float                           m_compressionRatio = 0.0F;

        float                           m_averagePositionBitsPerAxisX = 0.0F;
        float                           m_averagePositionBitsPerAxisY = 0.0F;
        float                           m_averagePositionBitsPerAxisZ = 0.0F;

        uint32_t                        m_minimumPositionBitsPerAxisX = 0;
        uint32_t                        m_minimumPositionBitsPerAxisY = 0;
        uint32_t                        m_minimumPositionBitsPerAxisZ = 0;

        float                           m_medianPositionBitsPerAxisX = 0.0F;
        float                           m_medianPositionBitsPerAxisY = 0.0F;
        float                           m_medianPositionBitsPerAxisZ = 0.0F;

        uint32_t                        m_maximumPositionBitsPerAxisX = 0;
        uint32_t                        m_maximumPositionBitsPerAxisY = 0;
        uint32_t                        m_maximumPositionBitsPerAxisZ = 0;

        uint32_t                        m_minimumPositionBitsPerVertex = 0;
        float                           m_averagePositionBitsPerVertex = 0.0F;
        uint32_t                        m_maximumPositionBitsPerVertex = 0;

        float                           m_minimumClusterUtilization = 0.0F;
        float                           m_maximumClusterUtilization = 0.0F;
        float                           m_averageClusterUtilization = 0.0F;
        float                           m_medianClusterUtilization = 0.0F;

        float                           m_clusterVertexOverhead = 0.0F;
        float                           m_vertexTriangleReuse = 0.0F;

        float                           m_minimumCompressionAccuracy = 0.0F;
        float                           m_averageCompressionAccuracy = 0.0F;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Mesh : public Resource::IResource
    {
        friend class MeshLoader;
        friend class MeshCompiler;
        friend class RenderSystem;

        EE_SERIALIZE
        (
            m_sockets, m_submeshes, m_submeshLocalTransforms,
            m_geometry, m_geometryLODDistance, m_statisticsPerLOD, m_numLODs,
            m_meshBounds
        );

    public:

        struct Submesh
        {
            EE_SERIALIZE( m_ID, m_materialNameID, m_geometryIdx, m_material, m_lodMask );

            StringID                    m_ID;
            StringID                    m_materialNameID; // The name of the material in the source file
            uint32_t                    m_geometryIdx = 0xFFFFFFFF;
            TResourcePtr<Material>      m_material;
            uint8_t                     m_lodMask = 0;
        };

        struct Socket
        {
            EE_SERIALIZE( m_ID, m_boneIdx, m_offset );

            StringID                    m_ID;
            int32_t                     m_boneIdx = InvalidIndex;
            Transform                   m_offset;
        };

    protected:

        constexpr static int32_t const s_sharedMeshVersion = 28;

        struct ResourceUpdateState
        {
            AsyncBufferUpdate*          m_pMeshBufferUpdate = nullptr;
        };

    public:

        virtual bool IsValid() const override { return !m_submeshLocalTransforms.empty() && !m_geometry.empty(); }

        // Submeshes
        //-------------------------------------------------------------------------

        inline int32_t GetNumSubmeshes() const { return int32_t( m_submeshLocalTransforms.size() ); }

        inline Submesh const& GetSubmesh( size_t submeshIdx ) const { return m_submeshes[submeshIdx]; }

        inline TVector<Submesh> const& GetSubmeshes() const { return m_submeshes; }

        inline TVector<Matrix43> const& GetSubmeshLocalTransforms() const { return m_submeshLocalTransforms; }

        inline StringID GetSubmeshID( size_t submeshIdx ) const { return m_submeshes[submeshIdx].m_ID; }

        inline int32_t GetSubmeshIndex( StringID submeshID ) const { return VectorFindIndex( m_submeshes, submeshID, [] ( Submesh const& sm, StringID submeshID ) { return sm.m_ID == submeshID; } ); }

        inline uint32_t GetSubmeshGeometryIndex( size_t submeshID ) const { return m_submeshes[submeshID].m_geometryIdx; }

        inline Material const* GetMaterial( int32_t submeshIdx ) const { return m_submeshes[submeshIdx].m_material.IsLoaded() ? m_submeshes[submeshIdx].m_material.GetPtr() : nullptr; }

        // Sockets
        //-------------------------------------------------------------------------

        // Get the number of sockets for this mesh
        inline int32_t GetNumSockets() const { return (int32_t) m_sockets.size(); }

        // Get the number of sockets for this mesh
        inline Socket const& GetSocket( int32_t socketIdx ) const { return m_sockets[socketIdx]; }

        // Get the socket index for a given ID
        inline int32_t GetSocketIndex( StringID socketID ) const { return VectorFindIndex( m_sockets, socketID, [] ( Socket const& s, StringID ID ) { return s.m_ID == ID; } ); }

        // Get the socket for a given ID
        Socket const* GetSocket( StringID socketID ) const;

        // Bounds
        //-------------------------------------------------------------------------

        inline const OBB& GetBounds() const { return m_meshBounds; }

        // LOD
        //-------------------------------------------------------------------------

        // Get the number of LODs we support
        inline uint32_t GetNumLODs() const { return m_numLODs; }

        // Get the distance threshold for the LODs we support
        inline TVector<float> const& GetLODDistances() const { return m_geometryLODDistance; }

        // Get the statistics for a specific LOD
        inline MeshStatistics const& GetStatistics( uint32_t lodIndex ) const
        {
            EE_ASSERT( lodIndex < m_statisticsPerLOD.size() );
            return m_statisticsPerLOD[lodIndex];
        }

        // Render Data
        //-------------------------------------------------------------------------

        inline TVector<Geometry> const& GetGeometry() const { return m_geometry; }

        inline RHI::Buffer* GetMeshBuffer( uint32_t index ) const { return m_meshBuffers[index]; }

    protected:

        // Core Data
        //-------------------------------------------------------------------------

        TVector<Socket>                     m_sockets;
        OBB                                 m_meshBounds;

        // Internal renderer data
        //-------------------------------------------------------------------------

        TVector<RHI::Buffer*>               m_meshBuffers;

        // Serialized submesh data
        //-------------------------------------------------------------------------

        TVector<Submesh>                    m_submeshes;
        TVector<Matrix43>                   m_submeshLocalTransforms;

        TVector<Geometry>                   m_geometry;
        TVector<float>                      m_geometryLODDistance;
        TVector<MeshStatistics>             m_statisticsPerLOD;
        uint32_t                            m_numLODs;

        TVector<ResourceUpdateState>        m_meshBuffersState;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API StaticMesh final : public Mesh
    {
        EE_RESOURCE( "mesh", "Static Mesh", Colors::LightSalmon, s_sharedMeshVersion + 32, false );
        EE_SERIALIZE( EE_SERIALIZE_BASE( Mesh ) );
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SkeletalMesh final : public Mesh
    {
        friend class MeshLoader;
        friend class MeshCompiler;

        EE_RESOURCE( "skelmesh", "Skeletal Mesh", Colors::LightCoral, s_sharedMeshVersion + 33, false );
        EE_SERIALIZE( EE_SERIALIZE_BASE( Mesh ), m_boneIDs, m_parentBoneIndices, m_parentSpaceBindPose, m_modelSpaceBindPose, m_modelSpaceInverseBindPose );

    public:

        virtual bool IsValid() const override;

        // Bone Info
        //-------------------------------------------------------------------------

        inline int32_t GetNumBones() const { return int32_t( m_boneIDs.size() ); }

        int32_t GetBoneIndex( StringID const& boneID ) const;

        EE_FORCE_INLINE bool IsValidBoneIndex( int32_t idx ) const { return idx >= 0 && idx < m_boneIDs.size(); }

        inline int32_t GetParentBoneIndex( int32_t const& idx ) const
        {
            EE_ASSERT( idx < int32_t( m_parentBoneIndices.size() ) );
            return m_parentBoneIndices[idx];
        }

        StringID GetBoneID( int32_t idx ) const
        {
            EE_ASSERT( idx < int32_t( m_boneIDs.size() ) );
            return m_boneIDs[idx];
        }

        inline TVector<StringID> const& GetBoneIDs() const { return m_boneIDs; }
        inline TVector<int32_t> const& GetParentBoneIndices() const { return m_parentBoneIndices; }

        // Returns whether the specified bone is an direct descendant of the specified parent bone
        bool IsDirectChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const { return m_parentBoneIndices[childBoneIdx] == parentBoneIdx; }

        // Returns whether the specified bone is a descendant of the specified parent bone (checks entire hierarchy, not just immediate parents)
        bool IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const;

        // Bind Poses
        inline TVector<Transform> const& GetParentSpaceBindPose() const { return m_parentSpaceBindPose; }
        inline TVector<Transform> const& GetModelSpaceBindPose() const { return m_modelSpaceBindPose; }
        inline TVector<Transform> const& GetModelSpaceInverseBindPose() const { return m_modelSpaceInverseBindPose; }

        EE_FORCE_INLINE Transform const& GetParentSpaceBindPoseTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_parentSpaceBindPose.size() );
            return m_parentSpaceBindPose[idx];
        }

        EE_FORCE_INLINE Transform const& GetModelSpaceBindPoseTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_modelSpaceBindPose.size() );
            return m_modelSpaceBindPose[idx];
        }

        EE_FORCE_INLINE Transform const& GetModelSpaceInverseBindPoseTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_modelSpaceInverseBindPose.size() );
            return m_modelSpaceInverseBindPose[idx];
        }

        // Debug
        #if EE_DEVELOPMENT_TOOLS
        void DrawBindPose( DebugDrawContext& drawingContext, Transform const& worldTransform, bool drawBoneNames = false ) const;
        #endif

    private:

        TVector<StringID>               m_boneIDs;
        TVector<int32_t>                m_parentBoneIndices;
        TVector<Transform>              m_parentSpaceBindPose;
        TVector<Transform>              m_modelSpaceBindPose;
        TVector<Transform>              m_modelSpaceInverseBindPose;
    };
}

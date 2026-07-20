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
    class EE_ENGINE_API Mesh : public Resource::IResource
    {
        friend class MeshLoader;
        friend class MeshCompiler;
        friend class RenderSystem;

        EE_SERIALIZE( m_sockets, m_submeshes, m_submeshLocalTransforms, m_geometry, m_geometryLODDistance, m_meshBounds, m_numLODs );

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

        constexpr static int32_t const s_sharedMeshVersion = 20;

    private:

        // Internal resource loader data
        //-------------------------------------------------------------------------

        struct ResourceUpdateState
        {
            AsyncBufferUpdate*          m_pVertexBufferUpdate = nullptr;
            AsyncBufferUpdate*          m_pTriangleBufferUpdate = nullptr;
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

        // Render Data
        //-------------------------------------------------------------------------

        inline TVector<Geometry> const& GetGeometry() const { return m_geometry; }

        inline RHI::Buffer* GetClusterVertexBuffer( uint32_t index ) const { return m_clusterVertexBuffers[index]; }
        inline RHI::Buffer* GetClusterTriangleBuffer( uint32_t index ) const { return m_clusterTriangleBuffers[index]; }

        inline MeshHandle const& GetMeshHandle() const { return m_meshHandle; }
        inline ClustersHandle const& GetClustersHandle() const { return m_clustersHandle; }

    protected:

        // Core Data
        //-------------------------------------------------------------------------

        TVector<Socket>                     m_sockets;
        OBB                                 m_meshBounds;

        // Internal renderer data
        //-------------------------------------------------------------------------

        TVector<RHI::Buffer*>               m_clusterVertexBuffers;
        TVector<RHI::Buffer*>               m_clusterTriangleBuffers;

        MeshHandle                          m_meshHandle;
        ClustersHandle                      m_clustersHandle;

        // Serialized submesh data
        //-------------------------------------------------------------------------

        TVector<Submesh>                    m_submeshes;
        TVector<Matrix43>                   m_submeshLocalTransforms;

        TVector<Geometry>                   m_geometry;
        TVector<float>                      m_geometryLODDistance;
        uint32_t                            m_numLODs;

    private:

        TVector<ResourceUpdateState>        m_clusterBuffersState;
        AsyncMeshUpdate*                    m_pMeshUpdate = nullptr;
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

        EE_RESOURCE( "skelmesh", "Skeletal Mesh", Colors::LightCoral, s_sharedMeshVersion + 32, false );
        EE_SERIALIZE( EE_SERIALIZE_BASE( Mesh ), m_boneIDs, m_parentBoneIndices, m_bindPose, m_inverseBindPose );

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

        inline TVector<StringID>& GetBoneIDs() { return m_boneIDs; }
        inline TVector<StringID> const& GetBoneIDs() const { return m_boneIDs; }

        inline TVector<int32_t>& GetParentBoneIndices() { return m_parentBoneIndices; }
        inline TVector<int32_t> const& GetParentBoneIndices() const { return m_parentBoneIndices; }

        // Returns whether the specified bone is an direct descendant of the specified parent bone
        bool IsDirectChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const { return m_parentBoneIndices[childBoneIdx] == parentBoneIdx; }

        // Returns whether the specified bone is a descendant of the specified parent bone (checks entire hierarchy, not just immediate parents)
        bool IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const;

        // Bind Poses
        inline TVector<Transform>& GetBindPose() { return m_bindPose; }
        inline TVector<Transform>& GetInverseBindPose() { return m_inverseBindPose; }

        inline TVector<Transform> const& GetBindPose() const { return m_bindPose; }
        inline TVector<Transform> const& GetInverseBindPose() const { return m_inverseBindPose; }

        EE_FORCE_INLINE Transform const& GetBindPoseTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_bindPose.size() );
            return m_bindPose[idx];
        }

        EE_FORCE_INLINE Transform const& GetInverseBindPoseTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_inverseBindPose.size() );
            return m_inverseBindPose[idx];
        }

        // Debug
        #if EE_DEVELOPMENT_TOOLS
        void DrawBindPose( DebugDrawContext& drawingContext, Transform const& worldTransform, bool drawBoneNames = false ) const;
        #endif

    private:

        TVector<StringID>               m_boneIDs;
        TVector<int32_t>                m_parentBoneIndices;
        TVector<Transform>              m_bindPose; // Note: bind pose is in global space
        TVector<Transform>              m_inverseBindPose;
    };
}

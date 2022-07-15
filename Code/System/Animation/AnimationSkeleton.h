#pragma once

#include "System/_Module/API.h"
#include "System/Resource/IResource.h"
#include "System/Types/StringID.h"
#include "System/Math/Transform.h"
#include "System/Types/BitFlags.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    // Per-bone flags to provide extra information
    enum class BoneFlags
    {
        None,
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API Skeleton : public Resource::IResource
    {
        EE_REGISTER_RESOURCE( 'skel', "Animation Skeleton" );
        EE_SERIALIZE( m_boneIDs, m_localReferencePose, m_parentIndices, m_boneFlags );

        friend class SkeletonCompiler;
        friend class SkeletonLoader;

    public:

        virtual bool IsValid() const final;
        inline int32_t GetNumBones() const { return (int32_t) m_boneIDs.size(); }

        // Bone info
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsValidBoneIndex( int32_t idx ) const { return idx >= 0 && idx < m_boneIDs.size(); }

        // Get the index for a given bone ID, can return InvalidIndex
        inline int32_t GetBoneIndex( StringID const& ID ) const { return VectorFindIndex( m_boneIDs, ID ); }

        // Get all parent indices
        inline TVector<int32_t> const& GetParentBoneIndices() const { return m_parentIndices; }

        // Get the direct parent for a given bone
        inline int32_t GetParentBoneIndex( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_parentIndices.size() );
            return m_parentIndices[idx];
        }

        // Find the index of the first child encountered for the specified bone. Returns InvalidIndex if this is a leaf bone.
        int32_t GetFirstChildBoneIndex( int32_t boneIdx ) const;

        // Returns whether the specified bone is a child of the specified parent bone
        bool IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const;

        // Returns whether the specified bone is a parent of the specified child bone
        EE_FORCE_INLINE bool IsParentBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const { return IsChildBoneOf( parentBoneIdx, childBoneIdx ); }

        // Returns whether the specified bone is a child of the specified parent bone
        EE_FORCE_INLINE bool AreBonesInTheSameHierarchy( int32_t boneIdx0, int32_t boneIdx1 ) const { return IsChildBoneOf( boneIdx0, boneIdx1) || IsChildBoneOf( boneIdx1, boneIdx0 ); }

        // Get the boneID for a specified bone index
        EE_FORCE_INLINE StringID GetBoneID( int32_t idx ) const
        {
            EE_ASSERT( IsValidBoneIndex( idx ) );
            return m_boneIDs[idx];
        }

        // Pose info
        //-------------------------------------------------------------------------

        TVector<Transform> const& GetLocalReferencePose() const { return m_localReferencePose; }
        TVector<Transform> const& GetGlobalReferencePose() const { return m_globalReferencePose; }

        inline Transform GetBoneTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_localReferencePose.size() );
            return m_localReferencePose[idx];
        }

        Transform GetBoneGlobalTransform( int32_t idx ) const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform ) const;
        #endif

    private:

        TVector<StringID>                   m_boneIDs;
        TVector<int32_t>                      m_parentIndices;
        TVector<Transform>                  m_localReferencePose;
        TVector<Transform>                  m_globalReferencePose;
        TVector<TBitFlags<BoneFlags>>       m_boneFlags;
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    EE_SYSTEM_API void DrawRootBone( Drawing::DrawContext& ctx, Transform const& transform );
    #endif
}
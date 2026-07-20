#pragma once

#include "Engine/_Module/API.h"
#include "AnimationBoneMask.h"
#include "AnimationFloatChannels.h"
#include "Base/Resource/IResource.h"
#include "Base/Math/Transform.h"
#include "Base/Types/BitFlags.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE { class DebugDrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using SecondarySkeletonList = TInlineVector<Skeleton const*, 3>;

    // Per-bone flags to provide extra information
    enum class BoneFlags
    {
        None,
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    struct DrawOptions
    {
        DrawOptions() = default;

        inline bool IsBoneFilteredOut( int32_t boneIdx ) const
        {
            if ( m_pBoneIdxFilter == nullptr )
            {
                return false;
            }

            return !VectorContains( *m_pBoneIdxFilter, boneIdx );
        }

    public:

        Color                   m_boneColor = Colors::HotPink;
        float                   m_lineThickness = 2.0f;
        float                   m_axisThickness = 3.0f;
        float                   m_axisLength = 0.01f;
        TVector<float> const*   m_pBoneWeights = nullptr;
        TVector<int32_t> const* m_pBoneIdxFilter = nullptr;
        bool                    m_drawAxes = true;
        bool                    m_drawBoneNames = false;
    };
    #endif

    //-------------------------------------------------------------------------
    // Animation Skeleton
    //-------------------------------------------------------------------------

    class EE_ENGINE_API Skeleton : public Resource::IResource
    {
        EE_RESOURCE( "skeleton", "Animation Skeleton", Colors::PaleVioletRed, 13, false );
        EE_SERIALIZE( m_boneIDs, m_parentSpaceReferencePose, m_parentIndices, m_boneFlags, m_numBonesToSampleAtLowLOD, m_secondarySkeletons, m_boneIndexLUT, m_floatChannelSets );

        friend class SkeletonCompiler;
        friend class SkeletonLoader;

    public:

        enum class LOD : uint8_t
        {
            Low,
            High
        };

        struct SecondarySkeleton
        {
            EE_SERIALIZE( m_attachmentBoneID, m_skeleton );

            StringID                        m_attachmentBoneID; // The bone that we expect to attach this skeleton to
            Resource::ResourcePtr           m_skeleton;
        };

    public:

        #if EE_DEVELOPMENT_TOOLS
        static inline Color GetColorForBoneWeight( float weight ) { return Color::EvaluateRedGreenGradient( weight ); }

        // Draw a debug widget representing the root bone
        static void DrawRootBone( DebugDrawContext& ctx, Transform const& transform );

        // Perform basic validation on a primary skeleton and a set of secondary skeletons
        static bool ValidateSkeletonSetup( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons );
        #endif

    public:

        virtual bool IsValid() const final;

        // Get the total number of bones in the skeleton
        inline int32_t GetNumBones() const { return (int32_t) m_boneIDs.size(); }

        // Get the number of bones to sample at a specific LOD
        inline int32_t GetNumBones( LOD lod ) const { return ( lod == LOD::Low ) ? m_numBonesToSampleAtLowLOD : GetNumBones(); }

        // Bone info
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsValidBoneIndex( int32_t idx ) const { return idx >= 0 && idx < m_boneIDs.size(); }

        // Get the index for a given bone ID, can return InvalidIndex
        inline int32_t GetBoneIndex( StringID const& ID ) const
        {
            auto foundIter = m_boneIndexLUT.find( ID );
            return ( foundIter != m_boneIndexLUT.end() ) ? foundIter->second : InvalidIndex;
        }

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

        // Returns whether the specified bone is an direct descendant of the specified parent bone
        bool IsDirectChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const { return m_parentIndices[childBoneIdx] == parentBoneIdx; }

        // Returns whether the specified bone is a descendant of the specified parent bone (checks entire hierarchy, not just immediate parents)
        bool IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const;

        // Returns whether the specified bone is a child of the specified parent bone
        EE_FORCE_INLINE bool AreBonesInTheSameHierarchy( int32_t boneIdx0, int32_t boneIdx1 ) const { return IsChildBoneOf( boneIdx0, boneIdx1) || IsChildBoneOf( boneIdx1, boneIdx0 ); }

        // Returns whether or not the specified bone has children
        EE_FORCE_INLINE bool IsLeafBone( int32_t boneIdx ) const { EE_ASSERT( IsValidBoneIndex( boneIdx ) ); return VectorContains( m_parentIndices, boneIdx ); }

        // Get all boneIDs
        EE_FORCE_INLINE TVector<StringID> const& GetBoneIDs() const { return m_boneIDs; }

        // Get the boneID for a specified bone index
        EE_FORCE_INLINE StringID GetBoneID( int32_t boneIdx ) const
        {
            EE_ASSERT( IsValidBoneIndex( boneIdx ) );
            return m_boneIDs[boneIdx];
        }

        // Get the LOD for a specific bone
        inline LOD GetBoneLOD( int32_t boneIdx ) const { return ( boneIdx > m_numBonesToSampleAtLowLOD ) ? LOD::High : LOD::Low; }

        // Will this bone be in a high LOD pose
        inline bool IsBoneHighLOD( int32_t boneIdx ) const { return boneIdx > m_numBonesToSampleAtLowLOD; }

        // Will this bone be in a low LOD pose
        inline bool IsBoneLowLOD( int32_t boneIdx ) const { return boneIdx <= m_numBonesToSampleAtLowLOD; }

        // Pose info
        //-------------------------------------------------------------------------

        TVector<Transform> const& GetParentSpaceReferencePose() const { return m_parentSpaceReferencePose; }
        TVector<Transform> const& GetModelSpaceReferencePose() const { return m_modelSpaceReferencePose; }

        // Get the parent space transform for a specified bone
        inline Transform const& GetBoneTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_parentSpaceReferencePose.size() );
            return m_parentSpaceReferencePose[idx];
        }

        EE_FORCE_INLINE Transform const& GetBoneParentSpaceTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_parentSpaceReferencePose.size() );
            return m_parentSpaceReferencePose[idx]; 
        }

        EE_FORCE_INLINE Transform const& GetBoneModelSpaceTransform( int32_t idx ) const
        {
            EE_ASSERT( idx >= 0 && idx < m_modelSpaceReferencePose.size() );
            return m_modelSpaceReferencePose[idx]; 
        }

        // Float Channels
        //-------------------------------------------------------------------------

        inline int32_t GetNumFloatChannelSets() const { return (int32_t) m_floatChannelSets.size(); }
        inline TVector<FloatChannelSet> const& GetFloatChannelSets() const { return m_floatChannelSets; }
        int32_t GetFloatChannelSetIndex( StringID ID ) const;
        inline StringID GetFloatChannelSetID( int32_t idx ) const { return m_floatChannelSets[idx].m_ID; }
        FloatChannelSet const *GetFloatChannelSet( int32_t channelSetIdx ) const { return &m_floatChannelSets[channelSetIdx]; }
        FloatChannelSet const *GetFloatChannelSet( StringID channelSetID ) const;


        // Bone Masks
        //-------------------------------------------------------------------------
        // We have a set of authored masks for this skeleton

        uint32_t GetNumBoneMaskSets() const { return (uint32_t) m_boneMaskSets.size(); }
        inline bool IsValidBoneMaskSetID( StringID maskSetID ) const { return GetBoneMaskSetIndex( maskSetID ) != InvalidIndex; }
        int8_t GetBoneMaskSetIndex( StringID maskSetID ) const;
        StringID GetBoneMaskSetID( int32_t nMaskSetIdx ) const { return m_boneMaskSets[nMaskSetIdx].m_ID; }
        BoneMaskSet const* GetBoneMaskSet( int32_t maskSetIdx ) const { return &m_boneMaskSets[maskSetIdx]; }
        BoneMaskSet const* GetBoneMaskSet( StringID maskSetID ) const;

        // Debug & Preview
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform, LOD lod, DrawOptions const& options = DrawOptions() ) const;
        inline ResourceID const& GetPreviewMeshID() const { return m_previewMeshID; }
        inline TVector<SecondarySkeleton> const& GetPotentialSecondarySkeletons() const { return m_secondarySkeletons; }
        inline StringID GetPreviewAttachmentBoneIDForSecondarySkeleton( Skeleton const* pSkeleton ) const;
        #endif

    private:

        TVector<StringID>                       m_boneIDs;
        TVector<int32_t>                        m_parentIndices;
        TVector<Transform>                      m_parentSpaceReferencePose;
        TVector<Transform>                      m_modelSpaceReferencePose;
        TVector<TBitFlags<BoneFlags>>           m_boneFlags;
        THashMap<StringID, int32_t>             m_boneIndexLUT;
        int32_t                                 m_numBonesToSampleAtLowLOD = 0; // The number of bones we should sample when operating at a low LOD
        TVector<BoneMaskSet>                    m_boneMaskSets;
        TVector<SecondarySkeleton>              m_secondarySkeletons;
        TVector<FloatChannelSet>                m_floatChannelSets;

        #if EE_DEVELOPMENT_TOOLS
        ResourceID                              m_previewMeshID;
        #endif
    };
}
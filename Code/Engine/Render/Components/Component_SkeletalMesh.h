#pragma once

#include "Component_RenderMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation { class Pose; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API SkeletalMeshComponent : public MeshComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( SkeletalMeshComponent );

    public:

        using MeshComponent::MeshComponent;

        // Mesh Data
        //-------------------------------------------------------------------------

        virtual bool HasMeshResourceSet() const override final { return m_mesh.IsSet(); }

        inline void SetMesh( ResourceID meshResourceID )
        {
            EE_ASSERT( IsUnloaded() );
            EE_ASSERT( meshResourceID.IsValid() );
            m_mesh = meshResourceID;
        }

        inline SkeletalMesh const* GetMesh() const
        {
            EE_ASSERT( m_mesh != nullptr && m_mesh->IsValid() );
            return m_mesh.GetPtr();
        }

        // Skeletal Pose
        //-------------------------------------------------------------------------

        // Get the character space (global) transforms for the mesh
        inline TVector<Transform> const& GetBoneTransforms() const { return m_boneTransforms; }

        // The the global space transform for a specific bone
        inline void SetBoneTransform( int32_t boneIdx, Transform const& transform )
        {
            EE_ASSERT( boneIdx >= 0 && boneIdx < m_boneTransforms.size() );
            m_boneTransforms[boneIdx] = transform;
        }

        // This function will finalize the pose, run any procedural bone solvers and generate the skinning transforms
        // Only run this function once per frame once you have set the final global pose
        void FinalizePose();

        // Get the skinning transforms for this mesh - these are the global transforms relative to the bind pose
        inline TVector<Matrix> const& GetSkinningTransforms() const { return m_skinningTransforms; }

        // Animation Pose
        //-------------------------------------------------------------------------

        inline bool HasSkeletonResourceSet() const { return m_skeleton.IsSet(); }
        inline Animation::Skeleton const* GetSkeleton() const { return m_skeleton.GetPtr(); }
        void SetSkeleton( ResourceID skeletonResourceID );

        void SetPose( Animation::Pose const* pPose );

        void ResetPose();

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawPose( Drawing::DrawContext& drawingContext ) const;
        #endif

    protected:

        virtual TVector<TResourcePtr<Render::Material>> const& GetDefaultMaterials() const override final;

        void UpdateSkinningTransforms();
        void GenerateAnimationBoneMap();

        virtual OBB CalculateLocalBounds() const override final;

        virtual void Initialize() override;
        virtual void Shutdown() override;

        virtual bool TryFindAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const override final;
        virtual bool HasSocket( StringID socketID ) const override final;

    protected:

        EE_EXPOSE TResourcePtr<SkeletalMesh>            m_mesh;
        EE_EXPOSE TResourcePtr<Animation::Skeleton>     m_skeleton = nullptr;
        TVector<int32_t>                                m_animToMeshBoneMap;
        TVector<Transform>                              m_boneTransforms;
        TVector<Matrix>                                 m_skinningTransforms;
    };

    //-------------------------------------------------------------------------

    // We often have the need to find the specific mesh component that is the main character mesh.
    // This class makes it explicit, no need for name or tag matching!
    class EE_ENGINE_API CharacterMeshComponent final : public SkeletalMeshComponent
    {
        EE_REGISTER_SINGLETON_ENTITY_COMPONENT( CharacterMeshComponent );

        using SkeletalMeshComponent::SkeletalMeshComponent;
    };
}
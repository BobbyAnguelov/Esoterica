#pragma once

#include "Component_RenderMesh.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"
#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Pose;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct SkinningProxy;
    class DeviceRenderWorld;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SkeletalMeshComponent : public MeshComponent
    {
        EE_ENTITY_COMPONENT( SkeletalMeshComponent );

        friend class RenderWorldSystem;

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

        // Get the model space transforms for the mesh
        inline TVector<Transform> const& GetBoneTransforms() const { return m_modelSpaceBoneTransforms; }

        // The the model space transform for a specific bone
        inline void SetBoneTransform( int32_t boneIdx, Transform const& transform )
        {
            EE_ASSERT( boneIdx >= 0 && boneIdx < m_modelSpaceBoneTransforms.size() );
            m_modelSpaceBoneTransforms[boneIdx] = transform;
        }

        // This function will finalize the pose, run any procedural bone solvers and generate the skinning transforms
        // Only run this function once per frame once you have set the final global pose
        void FinalizePose();

        // Animation Pose
        //-------------------------------------------------------------------------

        inline bool HasSkeletonResourceSet() const { return m_skeleton.IsSet(); }
        inline Animation::Skeleton const* GetSkeleton() const { return m_skeleton.GetPtr(); }
        void SetSkeleton( ResourceID skeletonResourceID );

        void SetPose( Animation::Pose const* pPose );

        void ResetPose();

        //-------------------------------------------------------------------------

        inline TEntityWorldSystemSignal<SkeletalMeshComponent>* GetInstanceDataUpdateSignal() { return &m_instanceDataUpdateSignal; }

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawPose( DebugDrawContext& drawingContext ) const;
        #endif

    protected:

        void GenerateAnimationBoneMap();

        virtual Mesh const* GetMeshResource() const override { return m_mesh.IsLoaded() ? m_mesh.GetPtr() : nullptr; }

        virtual OBB CalculateLocalBounds() const override final;

        virtual void OnWorldTransformUpdated() override;
        virtual void OnRenderInstanceDataUpdated() override;

        virtual void Initialize() override;
        virtual void Shutdown() override;

        virtual bool GetAttachmentSocketTransformInternal( StringID socketID, Transform& outSocketWorldTransform ) const override final;
        virtual bool HasSocket( StringID socketID ) const override final;

    protected:

        EE_REFLECT();
        TResourcePtr<SkeletalMesh>                      m_mesh;

        EE_REFLECT();
        TResourcePtr<Animation::Skeleton>               m_skeleton = nullptr;

        TVector<int32_t>                                m_animToMeshBoneMap;
        TVector<Transform>                              m_modelSpaceBoneTransforms;

        TEntityWorldSystemSignal<SkeletalMeshComponent> m_instanceDataUpdateSignal;

    private:

        //-------------------------------------------------------------------------

        void QueueInitializeMeshInstance( DeviceRenderWorld* pDeviceRenderWorld );

        void UpdateSkinningProxy();

        inline uint32_t ComputeInstanceDataSizeInBytes()
        {
            return MeshComponent::ComputeInstanceDataSizeInBytes( GetMesh() );
        }

        inline void WriteInstanceData( TArrayView<uint32_t> bufferData_WriteCombined ) const
        {
            EE_ASSERT( m_meshInstanceRootProxy.IsValid() );
            EE_ASSERT( m_skinningProxy.IsValid() );
            MeshComponent::WriteInstanceData( GetMesh(), m_meshInstanceProxy.m_instanceHandle, m_skinningProxy.m_bonesHandle, bufferData_WriteCombined );
        }

    private:

        // Internal renderer data
        //---------------------------------------------------------------------------------------------------

        MeshInstanceProxy                               m_meshInstanceRootProxy = {};
        MeshInstanceProxy                               m_meshInstanceProxy = {};
        SkinningProxy                                   m_skinningProxy = {};
    };

    //-------------------------------------------------------------------------

    // We often have the need to find the specific mesh component that is the main character mesh.
    // This class makes it explicit, no need for name or tag matching!
    class EE_ENGINE_API CharacterMeshComponent final : public SkeletalMeshComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( CharacterMeshComponent );

        using SkeletalMeshComponent::SkeletalMeshComponent;
    };
}

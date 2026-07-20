#include "Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    OBB SkeletalMeshComponent::CalculateLocalBounds() const
    {
        OBB bounds;

        if ( HasMeshResourceSet() )
        {
            if ( m_modelSpaceBoneTransforms.empty() )
            {
                bounds = m_mesh->GetBounds();
            }
            else // Use bones to calculate bounds
            {
                AABB newBounds;
                for ( auto const& boneTransform : m_modelSpaceBoneTransforms )
                {
                    newBounds.AddPoint( boneTransform.GetTranslation() );
                }

                bounds = OBB( newBounds );
            }
        }
        else
        {
            bounds = MeshComponent::CalculateLocalBounds();
        }

        return bounds;
    }

    void SkeletalMeshComponent::OnWorldTransformUpdated()
    {
        if ( !m_meshInstanceProxy.IsValid() )
        {
            return;
        }

        m_meshInstanceRootProxy.WriteRootTransform( GetWorldTransform(), GetWorldNonUniformScale() );
    }

    void SkeletalMeshComponent::OnRenderInstanceDataUpdated()
    {
        GetInstanceDataUpdateSignal()->Send( this );
    }

    void SkeletalMeshComponent::Initialize()
    {
        MeshComponent::Initialize();

        if ( HasMeshResourceSet() )
        {
            EE_ASSERT( m_mesh.IsLoaded() );

            if ( HasSkeletonResourceSet() )
            {
                GenerateAnimationBoneMap();
            }

            // Set mesh to reference pose
            //-------------------------------------------------------------------------

            m_modelSpaceBoneTransforms.resize( m_mesh->GetNumBones() );
            ResetPose();

            //-------------------------------------------------------------------------

            FinalizePose();
        }
    }

    void SkeletalMeshComponent::Shutdown()
    {
        m_modelSpaceBoneTransforms.clear();
        m_animToMeshBoneMap.clear();
        MeshComponent::Shutdown();
    }

    bool SkeletalMeshComponent::GetAttachmentSocketTransformInternal( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        EE_ASSERT( socketID.IsValid() );

        outSocketWorldTransform = GetWorldTransform();

        if ( m_mesh.IsSet() && m_mesh.IsLoaded() )
        {
            // Check mesh sockets first
            auto const pSocket = m_mesh->GetSocket( socketID );
            if ( pSocket != nullptr )
            {
                if ( pSocket->m_boneIdx != InvalidIndex )
                {
                    EE_ASSERT( pSocket->m_boneIdx < m_mesh->GetNumBones() );

                    if ( IsInitialized() )
                    {
                        outSocketWorldTransform = m_modelSpaceBoneTransforms[pSocket->m_boneIdx] * outSocketWorldTransform;
                    }
                    else
                    {
                        outSocketWorldTransform = m_mesh->GetBindPose()[pSocket->m_boneIdx] * outSocketWorldTransform;
                    }
                }
                else
                {
                    outSocketWorldTransform = pSocket->m_offset * outSocketWorldTransform;
                }

                return true;
            }

            // Check bones next
            auto const boneIdx = m_mesh->GetBoneIndex( socketID );
            if ( boneIdx != InvalidIndex )
            {
                if ( IsInitialized() )
                {
                    outSocketWorldTransform = m_modelSpaceBoneTransforms[boneIdx] * outSocketWorldTransform;
                }
                else
                {
                    outSocketWorldTransform = m_mesh->GetBindPose()[boneIdx] * outSocketWorldTransform;
                }

                return true;
            }
        }

        return false;
    }

    bool SkeletalMeshComponent::HasSocket( StringID socketID ) const
    {
        EE_ASSERT( socketID.IsValid() );

        if ( m_mesh.IsSet() && m_mesh.IsLoaded() )
        {
            int32_t boneIdx = m_mesh->GetBoneIndex( socketID );
            return boneIdx != InvalidIndex;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshComponent::SetSkeleton( ResourceID skeletonResourceID )
    {
        EE_ASSERT( IsUnloaded() );
        EE_ASSERT( skeletonResourceID.IsValid() );
        m_skeleton = skeletonResourceID;
    }

    void SkeletalMeshComponent::SetPose( Animation::Pose const* pPose )
    {
        EE_PROFILE_FUNCTION_RENDER();
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( HasMeshResourceSet() && HasSkeletonResourceSet() );
        EE_ASSERT( !m_animToMeshBoneMap.empty() );
        EE_ASSERT( pPose != nullptr && pPose->HasModelSpaceTransforms() );

        int32_t const numAnimBones = pPose->GetNumBones();
        for ( auto animBoneIdx = 0; animBoneIdx < numAnimBones; animBoneIdx++ )
        {
            int32_t const meshBoneIdx = m_animToMeshBoneMap[animBoneIdx];
            if ( meshBoneIdx != InvalidIndex )
            {
                Transform const boneTransform = pPose->GetModelSpaceTransform( animBoneIdx );
                m_modelSpaceBoneTransforms[meshBoneIdx] = boneTransform;
            }
        }
    }

    void SkeletalMeshComponent::ResetPose()
    {
        EE_ASSERT( IsInitialized() );

        if ( HasSkeletonResourceSet() )
        {
            Animation::Pose referencePose( m_skeleton.GetPtr() );
            referencePose.CalculateModelSpaceTransforms();
            SetPose( &referencePose );
        }
        else
        {
            m_modelSpaceBoneTransforms = m_mesh->GetBindPose();
        }
    }

    void SkeletalMeshComponent::FinalizePose()
    {
        EE_PROFILE_FUNCTION_RENDER();
        EE_ASSERT( m_mesh.IsSet() && m_mesh.IsLoaded() );

        NotifySocketsUpdated();
        UpdateBounds();
        UpdateSkinningProxy();
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshComponent::GenerateAnimationBoneMap()
    {
        EE_ASSERT( m_mesh != nullptr && m_skeleton != nullptr );

        auto const pMesh = GetMesh();

        auto const numBones = m_skeleton->GetNumBones();
        m_animToMeshBoneMap.resize( numBones, InvalidIndex );

        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            auto const& boneID = m_skeleton->GetBoneID( boneIdx );
            m_animToMeshBoneMap[boneIdx] = pMesh->GetBoneIndex( boneID );
        }
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshComponent::QueueInitializeMeshInstance( DeviceRenderWorld* pDeviceRenderWorld )
    {
        EE_ASSERT( m_meshInstanceRootProxy.IsValid() );
        EE_ASSERT( m_meshInstanceProxy.IsValid() );
        EE_ASSERT( m_skinningProxy.IsValid() );

        pDeviceRenderWorld->QueueMeshInstanceInitialize
        (
            uint32_t( m_meshInstanceProxy.m_instanceHandle.m_offset ),
            uint32_t( m_meshInstanceRootProxy.m_instanceHandle.m_offset ),
            GetMesh(),
            m_materialOverrides
        );

        m_meshInstanceRootProxy.WriteRootTransform( GetWorldTransform(), GetWorldNonUniformScale() );
        m_meshInstanceProxy.WriteLocalTransforms( GetMesh()->GetSubmeshLocalTransforms() );
    }

    void SkeletalMeshComponent::UpdateSkinningProxy()
    {
        if ( !m_skinningProxy.IsValid() )
        {
            return;
        }

        EE_ASSERT( m_mesh.IsSet() && m_mesh.IsLoaded() );

        m_skinningProxy.WriteTransforms( m_modelSpaceBoneTransforms, m_mesh->GetInverseBindPose() );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void SkeletalMeshComponent::DrawPose( DebugDrawContext& drawingContext ) const
    {
        EE_ASSERT( IsInitialized() );

        if ( !m_mesh.IsSet() || !m_mesh.IsLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        Transform const& worldTransform = GetWorldTransform();
        auto const       numBones = m_modelSpaceBoneTransforms.size();

        Transform boneWorldTransform = m_modelSpaceBoneTransforms[0] * worldTransform;
        drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange );
        drawingContext.DrawAxis( boneWorldTransform, 0.05f );

        for ( auto i = 1; i < numBones; i++ )
        {
            boneWorldTransform = m_modelSpaceBoneTransforms[i] * worldTransform;

            auto const      parentBoneIdx = m_mesh->GetParentBoneIndex( i );
            Transform const parentBoneWorldTransform = m_modelSpaceBoneTransforms[parentBoneIdx] * worldTransform;

            drawingContext.DrawLine( parentBoneWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), Colors::Orange );
            drawingContext.DrawAxis( boneWorldTransform, 0.03f, 2.0f );
        }
    }
    #endif
}

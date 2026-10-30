#include "Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"
#include "Engine/Render/ProceduralBones/ProceduralBoneSolver.h"

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
        WriteMeshInstanceRootTransform();
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

            if ( m_mesh->HasProceduralBones() )
            {
                m_pProceduralBoneSolver = EE::New<ProceduralBoneSolver>( m_mesh.GetPtr() );
            }

            // Set mesh to reference pose
            //-------------------------------------------------------------------------

            m_parentSpaceBoneTransforms.resize( m_mesh->GetNumBones() );
            m_modelSpaceBoneTransforms.resize( m_mesh->GetNumBones() );
            ResetPose();

            //-------------------------------------------------------------------------

            FinalizePose();
        }
    }

    void SkeletalMeshComponent::Shutdown()
    {
        EE::Delete( m_pProceduralBoneSolver );
        m_parentSpaceBoneTransforms.clear();
        m_modelSpaceBoneTransforms.clear();
        m_meshToAnimBoneMap.clear();
        m_transformsDirty = false;
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
                        outSocketWorldTransform = m_mesh->GetModelSpaceBindPoseTransform( pSocket->m_boneIdx ) * outSocketWorldTransform;
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
                    outSocketWorldTransform = m_mesh->GetModelSpaceBindPoseTransform( boneIdx ) * outSocketWorldTransform;
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
        EE_ASSERT( !m_meshToAnimBoneMap.empty() );
        EE_ASSERT( pPose != nullptr );
        EE_ASSERT( !pPose->IsPoseSet() || pPose->IsParentSpacePose() );
        EE_ASSERT( pPose->GetSkeleton() == m_skeleton.GetPtr() );

        if ( pPose->IsPoseSet() && pPose->IsParentSpacePose() )
        {
            SetParentSpaceTransformsFromAnimation( pPose->GetParentSpaceTransforms() );
        }
        else // Clear the pose
        {
            ResetPose();
        }
    }

    void SkeletalMeshComponent::ResetPose()
    {
        EE_ASSERT( IsInitialized() );

        if ( HasSkeletonResourceSet() )
        {
            SetParentSpaceTransformsFromAnimation( m_skeleton->GetParentSpaceReferencePose() );
        }
        else
        {
            m_parentSpaceBoneTransforms = m_mesh->GetParentSpaceBindPose();
            m_modelSpaceBoneTransforms = m_mesh->GetModelSpaceBindPose();
            m_transformsDirty = false;
        }
    }

    void SkeletalMeshComponent::SetParentSpaceTransformsFromAnimation( TVector<Transform> const& parentSpaceTransforms )
    {
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( HasMeshResourceSet() && HasSkeletonResourceSet() );
        EE_ASSERT( !m_meshToAnimBoneMap.empty() );

        int32_t const numMeshBones = m_mesh->GetNumBones();
        for ( auto meshBoneIdx = 0; meshBoneIdx < numMeshBones; meshBoneIdx++ )
        {
            int32_t const animBoneIdx = m_meshToAnimBoneMap[meshBoneIdx];
            if ( animBoneIdx != InvalidIndex )
            {
                Transform const boneTransform = parentSpaceTransforms[animBoneIdx];
                m_parentSpaceBoneTransforms[meshBoneIdx] = boneTransform;
            }
            else
            {
                m_parentSpaceBoneTransforms[meshBoneIdx] = m_mesh->GetParentSpaceBindPoseTransform( meshBoneIdx );
            }
        }

        m_transformsDirty = true;
    }

    void SkeletalMeshComponent::FinalizePose()
    {
        EE_PROFILE_FUNCTION_RENDER();
        EE_ASSERT( m_mesh.IsSet() && m_mesh.IsLoaded() );

        if ( m_transformsDirty )
        {
            // Run procedural bones
            //-------------------------------------------------------------------------

            if ( HasProceduralBones() && m_proceduralBonesEnabled )
            {
                EE_PROFILE_SCOPE_RENDER( "Procedural Bone Solver" );
                m_pProceduralBoneSolver->Solve( m_parentSpaceBoneTransforms );
            }

            // Calculate model space transforms
            //-------------------------------------------------------------------------

            CalculateModelSpaceTransforms();

            //-------------------------------------------------------------------------

            m_transformsDirty = false;
        }

        //-------------------------------------------------------------------------

        NotifySocketsUpdated();
        UpdateBounds();
        UpdateSkinningProxy();
    }

    void SkeletalMeshComponent::CalculateModelSpaceTransforms()
    {
        EE_ASSERT( m_mesh != nullptr );

        int32_t const numMeshBones = m_mesh->GetNumBones();
        m_modelSpaceBoneTransforms[0] = m_parentSpaceBoneTransforms[0];
        for ( int32_t boneIdx = 1; boneIdx < numMeshBones; boneIdx++ )
        {
            int32_t const parentIdx = m_mesh->GetParentBoneIndex( boneIdx );
            EE_ASSERT( parentIdx < boneIdx );
            m_modelSpaceBoneTransforms[boneIdx] = m_parentSpaceBoneTransforms[boneIdx] * m_modelSpaceBoneTransforms[parentIdx];
        }
    }

    void SkeletalMeshComponent::GenerateAnimationBoneMap()
    {
        EE_ASSERT( m_mesh != nullptr && m_skeleton != nullptr );

        auto const numMeshBones = m_mesh->GetNumBones();
        m_meshToAnimBoneMap.resize( numMeshBones, InvalidIndex );

        for ( auto meshBoneIdx = 0; meshBoneIdx < numMeshBones; meshBoneIdx++ )
        {
            auto const& meshBoneID = m_mesh->GetBoneID( meshBoneIdx );
            m_meshToAnimBoneMap[meshBoneIdx] = m_skeleton->GetBoneIndex( meshBoneID );
        }
    }

    void SkeletalMeshComponent::UpdateSkinningProxy()
    {
        if ( !m_skinningProxy.IsValid() )
        {
            return;
        }

        EE_ASSERT( m_mesh.IsSet() && m_mesh.IsLoaded() );

        m_skinningProxy.WriteTransforms( m_modelSpaceBoneTransforms, m_mesh->GetModelSpaceInverseBindPose() );
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
        auto const numBones = m_modelSpaceBoneTransforms.size();

        Transform boneWorldTransform = m_modelSpaceBoneTransforms[0] * worldTransform;
        drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange );
        drawingContext.DrawAxis( boneWorldTransform, 0.05f );

        for ( auto i = 1; i < numBones; i++ )
        {
            boneWorldTransform = m_modelSpaceBoneTransforms[i] * worldTransform;

            auto const parentBoneIdx = m_mesh->GetParentBoneIndex( i );
            Transform const parentBoneWorldTransform = m_modelSpaceBoneTransforms[parentBoneIdx] * worldTransform;

            drawingContext.DrawLine( parentBoneWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), Colors::Orange );
            drawingContext.DrawAxis( boneWorldTransform, 0.03f, 2.0f );
        }
    }
    #endif
}

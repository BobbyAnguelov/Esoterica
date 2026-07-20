#include "Component_RenderMesh.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/Shaders/MeshInstance.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void MeshComponent::Initialize()
    {
        SpatialEntityComponent::Initialize();

        if ( HasMeshResourceSet() )
        {
            auto pMesh = GetMeshResource();
            EE_ASSERT( pMesh != nullptr );
            m_submeshesHidden.resize( pMesh->GetNumSubmeshes() );
            m_materialOverrides.resize( pMesh->GetNumSubmeshes() );
        }
    }

    void MeshComponent::SetViewLayers( TBitFlags<ViewLayer> viewLayers )
    {
        EE_ASSERT( !IsInitialized() );
        m_viewLayers = viewLayers;
    }

    int32_t MeshComponent::GetNumSubmeshes() const
    {
        auto pMesh = GetMeshResource();
        if ( pMesh != nullptr )
        {
            return pMesh->GetNumSubmeshes();
        }

        return 0;
    }

    StringID MeshComponent::GetSubmeshID( int32_t submeshIdx ) const
    {
        auto pMesh = GetMeshResource();
        if ( pMesh != nullptr )
        {
            EE_ASSERT( submeshIdx >= 0 && submeshIdx < pMesh->GetNumSubmeshes() );
            return pMesh->GetSubmeshID( submeshIdx );
        }

        return StringID();
    }

    //-------------------------------------------------------------------------

    void MeshComponent::SetVisible( bool visible )
    {
        m_componentHidden = !visible;
        OnRenderInstanceDataUpdated();
    }

    bool MeshComponent::IsSubmeshVisible( int32_t submeshIdx ) const
    {
        if ( submeshIdx >= 0 && submeshIdx >= m_submeshesHidden.size() )
        {
            return true;
        }

        return !m_submeshesHidden[submeshIdx];
    }

    void MeshComponent::SetSubmeshVisibility( int32_t submeshIdx, bool isVisible )
    {
        EE_ASSERT( submeshIdx >= 0 && submeshIdx < int32_t( GetMeshResource()->GetNumSubmeshes() ) );

        m_submeshesHidden.resize( GetMeshResource()->GetNumSubmeshes() );
        m_submeshesHidden[submeshIdx] = !isVisible;

        OnRenderInstanceDataUpdated();
    }

    void MeshComponent::SetSubmeshVisibility( TPair<int32_t, bool> const* pData, size_t size )
    {
        for ( size_t i = 0; i < size; i++ )
        {
            int32_t const instanceIdx = pData[i].first;
            EE_ASSERT( instanceIdx < int32_t( GetMeshResource()->GetNumSubmeshes() ) );

            m_submeshesHidden.resize( GetMeshResource()->GetNumSubmeshes() );
            m_submeshesHidden[instanceIdx] = !pData[i].second;
        }

        OnRenderInstanceDataUpdated();
    }

    void MeshComponent::SetAllSubmeshVisibility( bool isVisible )
    {
        if ( isVisible )
        {
            m_submeshesHidden.clear();
        }
        else
        {
            m_submeshesHidden.resize( GetMeshResource()->GetNumSubmeshes() );
            eastl::fill( m_submeshesHidden.begin(), m_submeshesHidden.end(), !isVisible );
        }

        OnRenderInstanceDataUpdated();
    }

    //-------------------------------------------------------------------------

    bool MeshComponent::IsMaterialOverriden( int32_t submeshIdx ) const
    {
        EE_ASSERT( submeshIdx >= 0 );
        if ( submeshIdx > m_materialOverrides.size() )
        {
            return false;
        }

        return m_materialOverrides[submeshIdx].IsSet();
    }

    Material const* MeshComponent::GetMaterialOverride( int32_t submeshIdx ) const
    {
        EE_ASSERT( submeshIdx >= 0 );
        if ( submeshIdx > m_materialOverrides.size() )
        {
            return nullptr;
        }

        return m_materialOverrides[submeshIdx].IsLoaded() ? m_materialOverrides[submeshIdx].GetPtr() : nullptr;
    }

    ResourceID MeshComponent::GetMaterialOverrideResourceID( int32_t submeshIdx ) const
    {
        EE_ASSERT( submeshIdx >= 0 );
        if ( submeshIdx > m_materialOverrides.size() )
        {
            return ResourceID();
        }

        return m_materialOverrides[submeshIdx].IsLoaded() ? m_materialOverrides[submeshIdx].GetResourceID() : ResourceID();
    }

    void MeshComponent::SetMaterialOverride( int32_t submeshIdx, ResourceID const& materialResourceID )
    {
        EE_ASSERT( submeshIdx >= 0 );

        auto SetOverride = [this, submeshIdx, materialResourceID] ()
        {
            if ( submeshIdx >= m_materialOverrides.size() )
            {
                m_materialOverrides.resize( submeshIdx + 1 );
            }

            m_materialOverrides[submeshIdx] = materialResourceID;
        };

        RequestRuntimeResourceChange( SetOverride );
    }

    void MeshComponent::ClearMaterialOverrides()
    {
        EE_ASSERT( !IsInitialized() );

        auto SetOverride = [this] ()
        {
            for ( auto& materialSlot : m_materialOverrides )
            {
                materialSlot.Clear();
            }
        };

        RequestRuntimeResourceChange( SetOverride );
    }

    //-------------------------------------------------------------------------

    void MeshComponent::SetForcedMinLOD( int32_t forceMinLOD )
    {
        m_forcedMinLOD = Math::Clamp( forceMinLOD, -1, 7 );
        OnRenderInstanceDataUpdated();
    }

    int32_t MeshComponent::GetForcedMinLOD() const
    {
        return m_forcedMinLOD;
    }

    void MeshComponent::SetForcedLOD( int32_t forceLOD )
    {
        m_forcedLOD = Math::Clamp( forceLOD, -1, 7 );
        OnRenderInstanceDataUpdated();
    }

    int32_t MeshComponent::GetForcedLOD() const
    {
        return m_forcedLOD;
    }

    //-------------------------------------------------------------------------

    uint32_t MeshComponent::ComputeInstanceDataSizeInBytes( Mesh const* pMeshResource ) const
    {
        uint32_t const numInstances = uint32_t( pMeshResource->GetSubmeshLocalTransforms().size() );
        uint32_t baseSize = sizeof( ShaderTypes::MeshInstanceRoot ) / 4;

        baseSize += ( ( numInstances + 31 ) / 32 );
        baseSize += uint32_t( pMeshResource->GetLODDistances().size() );

        return baseSize * 4;
    }

    void MeshComponent::WriteInstanceData( Mesh const* pMeshResource, HandleAllocator<uint32_t>::Handle meshInstanceHandle, HandleAllocator<uint32_t>::Handle bonesHandle, TArrayView<uint32_t> bufferData_WriteCombined ) const
    {
        uint32_t numLODs = uint32_t( pMeshResource->GetLODDistances().size() );
        EE_ASSERT( numLODs <= 8 );

        ShaderTypes::MeshInstanceRoot meshInstanceRoot = {};
        meshInstanceRoot.m_instanceHidden = !IsVisible();

        meshInstanceRoot.m_numLODs = numLODs;

        meshInstanceRoot.m_firstInstance = meshInstanceHandle.m_offset;
        meshInstanceRoot.m_numInstances = meshInstanceHandle.m_size;

        if ( bonesHandle.IsValid() )
        {
            meshInstanceRoot.m_boneOffset = bonesHandle.m_offset;
        }
        else
        {
            meshInstanceRoot.m_boneOffset = ~0U;
        }

        meshInstanceRoot.m_renderViewLayerFlags = m_viewLayers;

        EE_ASSERT( m_forcedMinLOD <= 7 );
        EE_ASSERT( m_forcedLOD <= 7 );

        meshInstanceRoot.m_forceMinLOD = uint32_t( Math::Max( 0, m_forcedMinLOD ) );
        meshInstanceRoot.m_forceLOD = uint32_t( Math::Max( 0, m_forcedLOD ) );
        meshInstanceRoot.m_useForcedLOD = ( m_forcedLOD >= 0 );

        Matrix transformMatrix = GetWorldTransform().ToMatrix();
        float const globalUniformScale = GetWorldTransform().GetScale();

        // Validate that this matrix is a valid 4x3 matrix with implicit (0,0,0,1) column
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 0 ).GetW(), 0.0F ) );
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 1 ).GetW(), 0.0F ) );
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 2 ).GetW(), 0.0F ) );
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 3 ).GetW(), 1.0F ) );

        transformMatrix.GetRow( 0 ).StoreFloat3( meshInstanceRoot.m_rootTransform + 0 );
        transformMatrix.GetRow( 1 ).StoreFloat3( meshInstanceRoot.m_rootTransform + 3 );
        transformMatrix.GetRow( 2 ).StoreFloat3( meshInstanceRoot.m_rootTransform + 6 );
        transformMatrix.GetRow( 3 ).StoreFloat3( meshInstanceRoot.m_rootTransform + 9 );

        std::memcpy( bufferData_WriteCombined.data(), &meshInstanceRoot, sizeof( ShaderTypes::MeshInstanceRoot ) );

        // Write per-instance data
        for ( uint32_t instanceIndex = 0; instanceIndex < meshInstanceRoot.m_numInstances; ++instanceIndex )
        {
            // Resolve instance hidden
            bool instanceHidden = false;
            if ( instanceIndex < m_submeshesHidden.size() )
            {
                instanceHidden = m_submeshesHidden[instanceIndex];
            }

            // Write instance data.
            meshInstanceRoot.WriteInstanceHidden( bufferData_WriteCombined, instanceIndex, instanceHidden );
        }

        // Write per-LOD data
        TArrayView<float const> meshDataLODDistance = pMeshResource->GetLODDistances();
        for ( uint32_t lodIndex = 0; lodIndex < uint32_t( meshDataLODDistance.size() ); ++lodIndex )
        {
            meshInstanceRoot.WriteLODDistance( bufferData_WriteCombined, lodIndex, meshDataLODDistance[lodIndex] * globalUniformScale );
        }
    }
}

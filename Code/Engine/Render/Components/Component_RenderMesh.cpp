#include "Component_RenderMesh.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/Shaders/MeshInstance.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool MeshComponent::SubmeshSettings::operator==( SubmeshSettings const& rhs ) const
    {
        if ( m_hiddenSubmeshes.size() != rhs.m_hiddenSubmeshes.size() )
        {
            return false;
        }

        if ( m_materialOverrides.size() != rhs.m_materialOverrides.size() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        int32_t const numHidden = (int32_t) m_hiddenSubmeshes.size();
        for ( int32_t i = 0; i < numHidden; i++ )
        {
            if ( m_hiddenSubmeshes[i] != rhs.m_hiddenSubmeshes[i] )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        int32_t const numOverrides = (int32_t) m_materialOverrides.size();
        for ( int32_t i = 0; i < numOverrides; i++ )
        {
            if ( m_materialOverrides[i] != rhs.m_materialOverrides[i] )
            {
                return false;
            }
        }

        return true;
    }

    MeshComponent::MaterialOverride* MeshComponent::SubmeshSettings::GetMaterialOverride( int16_t submeshIdx )
    {
        for ( auto& materialOverride : m_materialOverrides )
        {
            if ( materialOverride.m_submeshIdx == submeshIdx )
            {
                return &materialOverride;
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    void MeshComponent::Initialize()
    {
        SpatialEntityComponent::Initialize();
        ValidateAndFixSubmeshSettings();
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

    void MeshComponent::ValidateAndFixSubmeshSettings()
    {
        if ( !HasMeshResourceSet() )
        {
            m_submeshSettings.Clear();
        }
        else
        {
            if ( IsMeshLoaded() )
            {
                int32_t const numSubmeshes = GetMeshResource()->GetNumSubmeshes();

                // Fill all resource IDs (and remove any invalid ones)
                for ( int32_t i = int32_t( m_submeshSettings.m_materialOverrides.size() ) - 1; i >= 0; i-- )
                {
                    int16_t const submeshIdx = m_submeshSettings.m_materialOverrides[i].m_submeshIdx;
                    if ( submeshIdx < 0 || submeshIdx >= numSubmeshes )
                    {
                        m_submeshSettings.m_materialOverrides.erase_unsorted( m_submeshSettings.m_materialOverrides.begin() + i );
                    }
                }

                // Fill all visibility state
                for ( int32_t i = int32_t( m_submeshSettings.m_hiddenSubmeshes.size() ) - 1; i >= 0; i-- )
                {
                    if ( m_submeshSettings.m_hiddenSubmeshes[i] < 0 || m_submeshSettings.m_hiddenSubmeshes[i] >= numSubmeshes )
                    {
                        m_submeshSettings.m_hiddenSubmeshes.erase_unsorted( m_submeshSettings.m_hiddenSubmeshes.begin() + i );
                    }
                }
            }
            else // Mesh not loaded
            {
                // Dont mess with the settings as we cannot validate anything...
            }
        }
    }

    //-------------------------------------------------------------------------

    void MeshComponent::SetVisible( bool visible )
    {
        m_componentHidden = !visible;
        OnRenderInstanceDataUpdated();
    }

    void MeshComponent::SetSubmeshVisibility( int16_t submeshIdx, bool isVisible )
    {
        EE_ASSERT( submeshIdx >= 0 );

        if ( IsMeshLoaded() )
        {
            EE_ASSERT( submeshIdx < int32_t( GetMeshResource()->GetNumSubmeshes() ) );
        }

        //-------------------------------------------------------------------------

        if ( isVisible )
        {
            m_submeshSettings.m_hiddenSubmeshes.erase_first( submeshIdx );
        }
        else
        {
            VectorEmplaceBackUnique( m_submeshSettings.m_hiddenSubmeshes, submeshIdx );
        }

        OnRenderInstanceDataUpdated();
    }

    void MeshComponent::SetSubmeshVisibility( TVector<int16_t> const& hiddenSubmeshes )
    {
        m_submeshSettings.m_hiddenSubmeshes = hiddenSubmeshes;
        OnRenderInstanceDataUpdated();
    }

    //-------------------------------------------------------------------------

    bool MeshComponent::IsMaterialOverridden( int16_t submeshIdx ) const
    {
        EE_ASSERT( submeshIdx >= 0 );
        return m_submeshSettings.GetMaterialOverride( submeshIdx ) != nullptr;
    }

    Material const* MeshComponent::GetMaterialOverride( int16_t submeshIdx ) const
    {
        EE_ASSERT( submeshIdx >= 0 );
        auto pOverride = m_submeshSettings.GetMaterialOverride( submeshIdx );
        if ( pOverride == nullptr )
        {
            return nullptr;
        }

        return pOverride->m_material.IsLoaded() ? pOverride->m_material.GetPtr() : nullptr;
    }

    ResourceID MeshComponent::GetMaterialOverrideResourceID( int16_t submeshIdx ) const
    {
        EE_ASSERT( submeshIdx >= 0 );
        auto pOverride = m_submeshSettings.GetMaterialOverride( submeshIdx );
        if ( pOverride == nullptr )
        {
            return ResourceID();
        }

        return pOverride->m_material.GetResourceID();
    }

    void MeshComponent::SetMaterialOverride( int16_t submeshIdx, ResourceID const& materialResourceID )
    {
        EE_ASSERT( submeshIdx >= 0 );

        auto SetOverride = [this, submeshIdx, materialResourceID] ()
        {
            auto pOverride = m_submeshSettings.GetMaterialOverride( submeshIdx );
            if ( pOverride == nullptr )
            {
                m_submeshSettings.m_materialOverrides.emplace_back( submeshIdx, materialResourceID );
            }
            else
            {
                pOverride->m_material = materialResourceID;
            }
        };

        RequestRuntimeResourceChange( SetOverride );
    }

    void MeshComponent::ClearMaterialOverrides()
    {
        RequestRuntimeResourceChange( [this] () { m_submeshSettings.m_materialOverrides.clear(); } );
    }

    TInlineVector<Material const*, 50> MeshComponent::GetResolvedMaterials() const
    {
        TInlineVector<Material const*, 50> materials;

        if ( !IsMeshLoaded() )
        {
            return materials;
        }

        //-------------------------------------------------------------------------

        auto pMesh = GetMeshResource();
        EE_ASSERT( pMesh != nullptr );

        int32_t const numSubmeshes = pMesh->GetNumSubmeshes();
        materials.reserve( numSubmeshes );

        for ( int16_t submeshIdx = 0; submeshIdx < numSubmeshes; submeshIdx++ )
        {
            auto pOverride = m_submeshSettings.GetMaterialOverride( submeshIdx );
            if ( pOverride != nullptr )
            {
                materials.emplace_back( pOverride->m_material.IsLoaded() ? pOverride->m_material.GetPtr() : nullptr );
            }
            else // Use default material
            {
                materials.emplace_back( pMesh->GetMaterial( submeshIdx ) );
            }
        }

        return materials;
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

        meshInstanceRoot.m_numSkinningAttributes = pMeshResource->GetGeometry()[0].GetNumSkinningAttributes();

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
            bool const instanceHidden = VectorContains( m_submeshSettings.m_hiddenSubmeshes, (int16_t) instanceIndex );
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

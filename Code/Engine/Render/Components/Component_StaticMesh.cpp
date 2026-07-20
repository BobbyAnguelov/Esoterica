#include "Component_StaticMesh.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    OBB StaticMeshComponent::CalculateLocalBounds() const
    {
        if ( HasMeshResourceSet() )
        {
            OBB scaledMeshBounds = m_mesh->GetBounds();
            scaledMeshBounds.m_center *= GetWorldNonUniformScale();
            scaledMeshBounds.m_extents *= GetWorldNonUniformScale();
            return scaledMeshBounds;
        }

        return MeshComponent::CalculateLocalBounds();
    }

    bool StaticMeshComponent::GetAttachmentSocketTransformInternal( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        EE_ASSERT( socketID.IsValid() );

        outSocketWorldTransform = GetWorldTransform();

        if ( m_mesh.IsSet() && m_mesh.IsLoaded() )
        {
            auto const pSocket = m_mesh->GetSocket( socketID );
            if ( pSocket != nullptr )
            {
                outSocketWorldTransform = pSocket->m_offset * outSocketWorldTransform;
                return true;
            }
        }

        return false;
    }

    void StaticMeshComponent::OnWorldTransformUpdated()
    {
        if ( !m_meshInstanceProxy.IsValid() )
        {
            return;
        }

        m_meshInstanceRootProxy.WriteRootTransform( GetWorldTransform(), GetWorldNonUniformScale() );
    }

    void StaticMeshComponent::OnNonUniformScaleChanged()
    {
        if ( !m_meshInstanceProxy.IsValid() )
        {
            return;
        }

        m_meshInstanceRootProxy.WriteRootTransform( GetWorldTransform(), GetWorldNonUniformScale() );
    }

    void StaticMeshComponent::OnRenderInstanceDataUpdated()
    {
        GetInstanceDataUpdateSignal()->Send( this );
    }

    //-------------------------------------------------------------------------

    void StaticMeshComponent::QueueInitializeMeshInstance( DeviceRenderWorld* pDeviceRenderWorld )
    {
        EE_ASSERT( m_meshInstanceRootProxy.IsValid() );
        EE_ASSERT( m_meshInstanceProxy.IsValid() );

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

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void StaticMeshComponent::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        if ( Math::IsNearZero( m_nonUniformScale.m_x ) ) m_nonUniformScale.m_x = 0.001f;
        if ( Math::IsNearZero( m_nonUniformScale.m_y ) ) m_nonUniformScale.m_y = 0.001f;
        if ( Math::IsNearZero( m_nonUniformScale.m_z ) ) m_nonUniformScale.m_z = 0.001f;

        MeshComponent::PostPropertyEdit( pPropertyEdited );
    }
    #endif
}

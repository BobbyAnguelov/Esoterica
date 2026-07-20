#pragma once

#include "Component_RenderMesh.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"
#include "Engine/Render/RenderMesh.h"

//-----------------------------------------------------------------------------------------------------------

namespace EE::Render
{
    class DeviceRenderWorld;

    //-------------------------------------------------------------------------------------------------------

    class EE_ENGINE_API StaticMeshComponent final : public MeshComponent
    {
        EE_ENTITY_COMPONENT( StaticMeshComponent );

        friend class RenderWorldSystem;

    public:

        using MeshComponent::MeshComponent;

        // Local Scale
        //---------------------------------------------------------------------------------------------------

        virtual bool SupportsNonUniformScale() const override { return true; }

        // Mesh Data
        //---------------------------------------------------------------------------------------------------

        virtual bool HasMeshResourceSet() const override{ return m_mesh.IsSet(); }

        inline void SetMesh( ResourceID meshResourceID )
        {
            EE_ASSERT( IsUnloaded() );
            EE_ASSERT( meshResourceID.IsValid() );
            m_mesh = meshResourceID;
        }

        inline StaticMesh const* GetMesh() const
        {
            EE_ASSERT( m_mesh != nullptr && m_mesh->IsValid() );
            return m_mesh.GetPtr();
        }

        //---------------------------------------------------------------------------------------------------

        inline TEntityWorldSystemSignal<StaticMeshComponent>* GetInstanceDataUpdateSignal() { return &m_instanceDataUpdateSignal; }

    private:

        virtual Mesh const* GetMeshResource() const override { return m_mesh.IsLoaded() ? m_mesh.GetPtr() : nullptr; }

        virtual OBB CalculateLocalBounds() const override final;

        virtual bool GetAttachmentSocketTransformInternal( StringID socketID, Transform& outSocketWorldTransform ) const override;

        virtual void OnWorldTransformUpdated() override;
        virtual void OnNonUniformScaleChanged() override;
        virtual void OnRenderInstanceDataUpdated() override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

        virtual Float3* GetNonUniformScaleForEdit() override { return &m_nonUniformScale; }

        //---------------------------------------------------------------------------------------------------

        inline uint32_t ComputeInstanceDataSizeInBytes()
        {
            return MeshComponent::ComputeInstanceDataSizeInBytes( GetMesh() );
        }

        inline void WriteInstanceData( TArrayView<uint32_t> bufferData_WriteCombined ) const
        {
            EE_ASSERT( m_meshInstanceRootProxy.IsValid() );
            MeshComponent::WriteInstanceData( GetMesh(), m_meshInstanceProxy.m_instanceHandle, {}, bufferData_WriteCombined );
        }

        void QueueInitializeMeshInstance( DeviceRenderWorld* pDeviceRenderWorld );

    private:

        EE_REFLECT();
        TResourcePtr<StaticMesh>                        m_mesh;

        // A local scale that doesnt propagate but that can allow for non-uniform scaling of meshes
        EE_REFLECT();
        Float3                                          m_nonUniformScale = Float3::One;

        //---------------------------------------------------------------------------------------------------

        TEntityWorldSystemSignal<StaticMeshComponent>   m_instanceDataUpdateSignal;

        // Internal renderer data
        //---------------------------------------------------------------------------------------------------

        MeshInstanceProxy                               m_meshInstanceRootProxy = {};
        MeshInstanceProxy                               m_meshInstanceProxy = {};
    };
}

#include "WorldSystem_Render.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderViewport.h"
#include "Base/Types/Arrays.h"
#include "Base/Profiling.h"
#include "Base/Render/RHI.h"
#include "Base/Threading/TaskSystem.h"

#include "Engine/Render/Shaders/MeshInstance.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    #if EE_DEVELOPMENT_TOOLS
    void RenderWorldSystem::UpdateViewportPickingData( RenderViewport* pViewport ) const
    {
        PickingData& pickingData = pViewport->GetPickingData();
        pickingData.clear();

        if ( !pViewport->IsPickingEnabled() )
        {
            return;
        }

        auto ResolvePickingData = [this] ( DeviceAppendBuffer<PickingResult> const& buffer, PickingData& pickingData )
        {
            auto TryResolvePickingID = [this] ( PickingResult const& pr )
            {
                if ( pr.m_hitTestID != PickingID::InvalidID )
                {
                    return PickingID( pr.m_hitTestID, PickingID::InvalidID, pr.m_sortPriority, pr.m_intersectionDistance );
                }

                for ( StaticMeshComponent const* pComponent : m_staticMeshComponents )
                {
                    if ( ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset <= pr.m_instanceID ) && ( pr.m_instanceID < ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset + pComponent->m_meshInstanceProxy.m_instanceHandle.m_size ) ) )
                    {
                        return PickingID( pComponent->GetEntityID().m_value, pComponent->GetID().m_value, pr.m_sortPriority, pr.m_intersectionDistance );
                    }
                }

                for ( SkeletalMeshComponent const* pComponent : m_skeletalMeshComponents )
                {
                    if ( ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset <= pr.m_instanceID ) && ( pr.m_instanceID < ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset + pComponent->m_meshInstanceProxy.m_instanceHandle.m_size ) ) )
                    {
                        return PickingID( pComponent->GetEntityID().m_value, pComponent->GetID().m_value, pr.m_sortPriority, pr.m_intersectionDistance );
                    }
                }

                return PickingID();
            };

            //-----------------------------------------------------------------------------------------------

            for ( PickingResult const& result : buffer.m_bufferData )
            {
                PickingID pickingID = TryResolvePickingID( result );
                if ( pickingID.IsSet() )
                {
                    pickingData.push_back( pickingID );
                }
            }
        };

        uint32_t frameIndex = m_pRenderSystem->GetFrameIndex();

        ResolvePickingData( pViewport->m_debugDrawPickingResultsBuffer, pickingData );
        ResolvePickingData( pViewport->m_instancePickingResultsBuffer, pickingData );

        pickingData.DeduplicateAndSort();
    }
    #endif

    void RenderWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        m_pTaskSystem = systemRegistry.GetSystem<TaskSystem>();
        m_pRenderSystem = systemRegistry.GetSystem<RenderSystem>();

        m_deviceRenderWorld.Initialize( m_pTaskSystem, m_pRenderSystem );

        m_materialShaderClusterCapacity.Initialize();

        // TODO: Need to make it a resource instead of allocating it here
        static constexpr uint32_t g_RadianceResolution = 128;
        static constexpr uint32_t g_IrradianceResolution = 32;

        RHI::TextureParameters renderTargetParameters = {};
        renderTargetParameters.m_width = g_RadianceResolution;
        renderTargetParameters.m_height = g_RadianceResolution;
        renderTargetParameters.m_arrayLayers = 6;
        renderTargetParameters.m_mipLevels = RHI::ComputeTextureMipLevels( g_RadianceResolution, g_RadianceResolution, 1 );
        renderTargetParameters.m_format = RHI::DataFormat::RGBA16_SFloat;
        renderTargetParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::TextureCube,
                                                                                        RHI::DescriptorTypeFlags::RenderTarget );
        renderTargetParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
        renderTargetParameters.m_debugName = "GlobalEnvironmentMap Radiance Target";

        m_pRadianceTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), renderTargetParameters );

        renderTargetParameters.m_width = g_IrradianceResolution;
        renderTargetParameters.m_height = g_IrradianceResolution;
        renderTargetParameters.m_format = RHI::DataFormat::RGBA32_SFloat;
        renderTargetParameters.m_mipLevels = 1;
        renderTargetParameters.m_debugName = "GlobalEnvironmentMap Irradiance Target";

        m_pIrradianceTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), renderTargetParameters );
    }

    void RenderWorldSystem::ShutdownSystem()
    {
        m_pRenderSystem->WaitAllQueuesIdle();

        m_materialShaderClusterCapacity.Shutdown();

        EE_ASSERT( m_numShadowCastingDirectionalLights == 0 );

        m_deviceRenderWorld.Shutdown( m_pRenderSystem );

        RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( m_pRadianceTexture ) );
        RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( m_pIrradianceTexture ) );

        m_pRenderSystem = nullptr;
        m_pTaskSystem = nullptr;
    }

    void RenderWorldSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            if ( pStaticMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( !pStaticMeshComponent->m_meshInstanceProxy.m_instanceHandle.IsValid() );

                Mesh const* pStaticMesh = pStaticMeshComponent->GetMesh();

                AddMeshClusters( pStaticMesh, pStaticMeshComponent->m_materialOverrides, pStaticMeshComponent->m_viewLayers );

                uint32_t instanceDataSizeInBytes = pStaticMeshComponent->ComputeInstanceDataSizeInBytes();

                pStaticMeshComponent->m_meshInstanceRootProxy = m_deviceRenderWorld.AllocateMeshInstanceRoot( ( instanceDataSizeInBytes + 63 ) / 64 );
                pStaticMeshComponent->m_meshInstanceProxy = m_deviceRenderWorld.AllocateMeshInstance( pStaticMesh->GetNumSubmeshes() );

                pStaticMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );

                if ( pStaticMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }

                m_staticMeshComponents.Add( pStaticMeshComponent );
                m_staticMeshComponentInstanceUpdateQueue.Bind( pStaticMeshComponent, pStaticMeshComponent->GetInstanceDataUpdateSignal() );

                if ( instanceDataSizeInBytes )
                {
                    pStaticMeshComponent->GetInstanceDataUpdateSignal()->Send( pStaticMeshComponent );
                }
            }
        }
        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            if ( pSkeletalMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( !pSkeletalMeshComponent->m_meshInstanceProxy.m_instanceHandle.IsValid() );
                EE_ASSERT( !pSkeletalMeshComponent->m_skinningProxy.IsValid() );

                SkeletalMesh const* pSkeletalMesh = pSkeletalMeshComponent->GetMesh();

                AddMeshClusters( pSkeletalMesh, pSkeletalMeshComponent->m_materialOverrides, pSkeletalMeshComponent->m_viewLayers );

                uint32_t instanceDataSizeInBytes = pSkeletalMeshComponent->ComputeInstanceDataSizeInBytes();

                pSkeletalMeshComponent->m_skinningProxy = m_deviceRenderWorld.AllocateSkinningInstance( pSkeletalMesh->GetNumBones() );

                pSkeletalMeshComponent->m_meshInstanceRootProxy = m_deviceRenderWorld.AllocateMeshInstanceRoot( ( instanceDataSizeInBytes + 63 ) / 64 );
                pSkeletalMeshComponent->m_meshInstanceProxy = m_deviceRenderWorld.AllocateMeshInstance( pSkeletalMesh->GetNumSubmeshes() );

                pSkeletalMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );
                pSkeletalMeshComponent->UpdateSkinningProxy();

                if ( pSkeletalMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }

                m_skeletalMeshComponents.Add( pSkeletalMeshComponent );
                m_skeletalMeshComponentInstanceUpdateQueue.Bind( pSkeletalMeshComponent, pSkeletalMeshComponent->GetInstanceDataUpdateSignal() );

                if ( instanceDataSizeInBytes )
                {
                    pSkeletalMeshComponent->GetInstanceDataUpdateSignal()->Send( pSkeletalMeshComponent );
                }
            }
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                if ( pDirectionalLightComponent->GetShadowed() )
                {
                    pDirectionalLightComponent->m_cascadedShadowIndex = uint16_t( m_numShadowCastingDirectionalLights );
                    m_numShadowCastingDirectionalLights++;
                }

                pDirectionalLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocateDirectionalLight();
                pDirectionalLightComponent->OnWorldTransformUpdated();

                m_directionalLightComponents.Add( pDirectionalLightComponent );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                pPointLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocatePointLight();
                pPointLightComponent->OnWorldTransformUpdated();

                m_pointLightComponents.Add( pPointLightComponent );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                pSpotLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocateSpotLight();
                pSpotLightComponent->OnWorldTransformUpdated();

                m_spotLightComponents.Add( pSpotLightComponent );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
        }
    }

    void RenderWorldSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            if ( pStaticMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( pStaticMeshComponent->m_meshInstanceProxy.m_instanceHandle.IsValid() );

                RemoveMeshClusters( pStaticMeshComponent->GetMesh(), pStaticMeshComponent->m_materialOverrides, pStaticMeshComponent->m_viewLayers );

                m_staticMeshComponentInstanceUpdateQueue.Unbind( pStaticMeshComponent, pStaticMeshComponent->GetInstanceDataUpdateSignal() );

                m_staticMeshComponents.Remove( pStaticMeshComponent->GetID() );
                m_deviceRenderWorld.DeallocateMeshInstance( eastl::move( pStaticMeshComponent->m_meshInstanceProxy ) );
                m_deviceRenderWorld.DeallocateMeshInstanceRoot( eastl::move( pStaticMeshComponent->m_meshInstanceRootProxy ) );

                if ( pStaticMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }
            }
        }
        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            if ( pSkeletalMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( pSkeletalMeshComponent->m_meshInstanceProxy.IsValid() );
                EE_ASSERT( pSkeletalMeshComponent->m_skinningProxy.IsValid() );

                RemoveMeshClusters( pSkeletalMeshComponent->GetMesh(), pSkeletalMeshComponent->m_materialOverrides, pSkeletalMeshComponent->m_viewLayers );

                m_skeletalMeshComponentInstanceUpdateQueue.Unbind( pSkeletalMeshComponent, pSkeletalMeshComponent->GetInstanceDataUpdateSignal() );

                m_skeletalMeshComponents.Remove( pSkeletalMeshComponent->GetID() );
                m_deviceRenderWorld.DeallocateSkinningInstance( eastl::move( pSkeletalMeshComponent->m_skinningProxy ) );
                m_deviceRenderWorld.DeallocateMeshInstance( eastl::move( pSkeletalMeshComponent->m_meshInstanceProxy ) );
                m_deviceRenderWorld.DeallocateMeshInstanceRoot( eastl::move( pSkeletalMeshComponent->m_meshInstanceRootProxy ) );

                if ( pSkeletalMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }
            }
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                if ( pDirectionalLightComponent->GetShadowed() )
                {
                    m_numShadowCastingDirectionalLights--;
                }

                m_deviceRenderWorld.DeallocateDirectionalLight( eastl::move( pDirectionalLightComponent->m_lightInstanceProxy ) );

                m_directionalLightComponents.Remove( pDirectionalLightComponent->GetID() );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocatePointLight( eastl::move( pPointLightComponent->m_lightInstanceProxy ) );

                m_pointLightComponents.Remove( pPointLightComponent->GetID() );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocateSpotLight( eastl::move( pSpotLightComponent->m_lightInstanceProxy ) );

                m_spotLightComponents.Remove( pSpotLightComponent->GetID() );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            // Do nothing
        }
    }

    void RenderWorldSystem::AddMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers )
    {
        int32_t const numSubmeshes = pMeshResource->GetNumSubmeshes();
        for ( int32_t submeshIdx = 0; submeshIdx < numSubmeshes; ++submeshIdx )
        {
            uint32_t const geometryIdx = pMeshResource->GetSubmeshGeometryIndex( submeshIdx );

            // Resolve material override
            Material const* pMaterial = pMeshResource->GetMaterial( submeshIdx );
            if ( submeshIdx < materialOverrides.size() )
            {
                TResourcePtr<Material> const& overrideMaterial = materialOverrides[submeshIdx];
                if ( overrideMaterial.IsLoaded() )
                {
                    EE_ASSERT( overrideMaterial->IsValid() );
                    pMaterial = overrideMaterial.GetPtr();
                }
            }

            if ( pMaterial == nullptr )
            {
                pMaterial = m_pRenderSystem->GetPlaceholderMaterial();
            }

            EE_ASSERT( pMaterial != nullptr );

            int32_t shaderIndex = pMaterial->GetShaderIndex();
            EE_ASSERT( shaderIndex != -1 );

            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
            uint32_t const requiredClusterCapacity = geometry.GetNumClusters();

            m_materialShaderClusterCapacity.AddGlobalClusters( requiredClusterCapacity );
            ForEachViewLayer( [this, viewLayers, &shaderIndex, requiredClusterCapacity] ( ViewLayer viewLayer )
            {
                if ( viewLayers.IsFlagSet( viewLayer ) )
                {
                    m_materialShaderClusterCapacity.AddViewLayerClusters( uint32_t( viewLayer ), shaderIndex, requiredClusterCapacity );
                }
            } );
        }
    }

    void RenderWorldSystem::RemoveMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers )
    {
        int32_t const numSubmeshes = pMeshResource->GetNumSubmeshes();
        for ( int32_t submeshIdx = 0; submeshIdx < pMeshResource->GetNumSubmeshes(); ++submeshIdx )
        {
            uint32_t const geometryIdx = pMeshResource->GetSubmeshGeometryIndex( submeshIdx );

            // Resolve material override
            Material const* pMaterial = pMeshResource->GetMaterial( submeshIdx );
            if ( submeshIdx < materialOverrides.size() )
            {
                TResourcePtr<Material> const& overrideMaterial = materialOverrides[submeshIdx];
                if ( overrideMaterial.IsLoaded() )
                {
                    EE_ASSERT( overrideMaterial->IsValid() );
                    pMaterial = overrideMaterial.GetPtr();
                }
            }

            if ( pMaterial == nullptr )
            {
                pMaterial = m_pRenderSystem->GetPlaceholderMaterial();
            }

            EE_ASSERT( pMaterial != nullptr );

            int32_t shaderIndex = pMaterial->GetShaderIndex();
            EE_ASSERT( shaderIndex != -1 );

            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
            uint32_t const requiredClusterCapacity = geometry.GetNumClusters();

            m_materialShaderClusterCapacity.RemoveGlobalClusters( requiredClusterCapacity );
            ForEachViewLayer( [this, viewLayers, &shaderIndex, requiredClusterCapacity] ( ViewLayer viewLayer )
            {
                if ( viewLayers.IsFlagSet( viewLayer ) )
                {
                    m_materialShaderClusterCapacity.RemoveViewLayerClusters( uint32_t( viewLayer ), shaderIndex, requiredClusterCapacity );
                }
            } );
        }
    }

    void RenderWorldSystem::UpdateDeviceResources()
    {
        EE_PROFILE_FUNCTION_RENDER();

        m_deviceRenderWorld.UpdateDeviceResources_BeforeInstanceInitialize( m_pRenderSystem );

        // InstanceUpdate StaticMesh
        //---------------------------------------------------------------------------------------------------
        {
            TEntityMessageQueue<StaticMeshComponent>::Message staticMeshComponentMessage = {};
            while ( m_staticMeshComponentInstanceUpdateQueue.Dequeue( staticMeshComponentMessage ) )
            {
                StaticMeshComponent* pStaticMeshComponent = staticMeshComponentMessage.m_pComponent;
                EE_ASSERT( pStaticMeshComponent );

                auto CopyBufferMemory = [pStaticMeshComponent] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
                {
                    uint32_t instanceDataSizeInBytes = pStaticMeshComponent->ComputeInstanceDataSizeInBytes();
                    EE_ASSERT( pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size == ( ( instanceDataSizeInBytes + 63 ) / 64 ) );
                    EE_ASSERT( ( ( instanceDataSizeInBytes + 63 ) / 64 ) * 64 == dstSize );

                    pStaticMeshComponent->WriteInstanceData( { reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined ), instanceDataSizeInBytes / sizeof( uint32_t ) } );
                };

                m_pRenderSystem->QueueBufferUpdate
                (
                    CopyBufferMemory,
                    m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
                    pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_offset * sizeof( ShaderTypes::MeshInstanceRoot ),
                    pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size * sizeof( ShaderTypes::MeshInstanceRoot )
                );
                pStaticMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );
            }

            m_staticMeshComponentInstanceUpdateQueue.ClearIgnoredComponents();
        }

        // InstanceUpdate SkeletalMesh
        //---------------------------------------------------------------------------------------------------
        {
            TEntityMessageQueue<SkeletalMeshComponent>::Message skeletalMeshComponentMessage = {};
            while ( m_skeletalMeshComponentInstanceUpdateQueue.Dequeue( skeletalMeshComponentMessage ) )
            {
                SkeletalMeshComponent* pSkeletalMeshComponent = skeletalMeshComponentMessage.m_pComponent;
                EE_ASSERT( pSkeletalMeshComponent );

                auto CopyBufferMemory = [pSkeletalMeshComponent] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
                {
                    uint32_t instanceDataSizeInBytes = pSkeletalMeshComponent->ComputeInstanceDataSizeInBytes();
                    EE_ASSERT( pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size == ( ( instanceDataSizeInBytes + 63 ) / 64 ) );
                    EE_ASSERT( ( ( instanceDataSizeInBytes + 63 ) / 64 ) * 64 == dstSize );

                    pSkeletalMeshComponent->WriteInstanceData( { reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined ), instanceDataSizeInBytes / sizeof( uint32_t ) } );
                };

                m_pRenderSystem->QueueBufferUpdate
                (
                    CopyBufferMemory,
                    m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
                    pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_offset * sizeof( ShaderTypes::MeshInstanceRoot ),
                    pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size * sizeof( ShaderTypes::MeshInstanceRoot )
                );
                pSkeletalMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );
            }

            m_skeletalMeshComponentInstanceUpdateQueue.ClearIgnoredComponents();
        }

        m_deviceRenderWorld.UpdateDeviceResources_AfterInstanceInitialize( m_pRenderSystem );
    }

    RHI::TextureHandle RenderWorldSystem::GetRadianceTextureHandle() const
    {
        return RHI::GetTextureHandle( m_pRadianceTexture, RHI::DescriptorTypeFlags::TextureCube, 0 );
    }

    float RenderWorldSystem::GetRadianceTextureMipLevels() const
    {
        return float( m_pRadianceTexture->m_mipLevels );
    }

    RHI::TextureHandle RenderWorldSystem::GetIrradianceTextureHandle() const
    {
        return RHI::GetTextureHandle( m_pIrradianceTexture, RHI::DescriptorTypeFlags::TextureCube, 0 );
    }
}

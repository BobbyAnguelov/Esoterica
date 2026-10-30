#pragma once

#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/Device/DeviceInstancePool.h"
#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/Shaders/Renderer/RenderView.esh"
#include "Engine/Render/Shaders/Renderer/LightInstance.esh"
#include "Base/Threading/TaskSystem.h"
#include "Base/Profiling.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Render/RHI.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Types/Containers_ForwardDecl.h"

#include "Engine/Render/Shaders/Renderer/WorldUpdate.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderSystem;

    // World representation in device memory for rendering purposes
    //-------------------------------------------------------------------------

    class EE_ENGINE_API DeviceRenderWorld
    {
    public:

        uint32_t GetSkinningTransformBufferCapacity() const;

        uint32_t GetNumMeshInstanceRootPages() const;

        uint32_t GetNumDirectionalLightPages() const;
        uint32_t GetNumPointLightPages() const;
        uint32_t GetNumSpotLightPages() const;
        uint32_t GetNumDecalPages() const;

        void Initialize( TaskSystem* pTaskSystem, RenderSystem* pRenderSystem );
        void Shutdown( RenderSystem* pRenderSystem );

        // Mesh instances
        //-------------------------------------------------------------------------

        MeshInstanceRootProxy AllocateMeshInstanceRoot();
        void DeallocateMeshInstanceRoot( MeshInstanceRootProxy&& meshInstanceRootProxy );

        MeshInstanceProxy AllocateMeshInstance( uint32_t shaderIndex, TArrayView<uint32_t const> numClustersPerInstance );
        void DeallocateMeshInstance( MeshInstanceProxy&& meshInstanceProxy );

        SkinningProxy AllocateSkinningInstance( uint32_t numBones );
        void DeallocateSkinningInstance( SkinningProxy&& skinningProxy );

        // Lights
        //-------------------------------------------------------------------------

        LightInstanceProxy AllocateDirectionalLight();
        LightInstanceProxy AllocatePointLight();
        LightInstanceProxy AllocateSpotLight();

        void DeallocateDirectionalLight( LightInstanceProxy&& lightInstanceProxy );
        void DeallocatePointLight( LightInstanceProxy&& lightInstanceProxy );
        void DeallocateSpotLight( LightInstanceProxy&& lightInstanceProxy );

        // Decals
        //-------------------------------------------------------------------------

        DecalInstanceProxy AllocateDecal();
        void DeallocateDecal( DecalInstanceProxy&& decalInstanceProxy );

        // Render views
        //-------------------------------------------------------------------------

        RenderViewProxy AllocateRenderViews( DeviceRenderViewType type, uint32_t numViews );
        void DeallocateRenderViews( RenderViewProxy&& renderViewProxy );

        inline DeviceRenderView* GetDeviceRenderView( uint32_t viewIndex )
        {
            EE_ASSERT( viewIndex < m_renderViews.m_storage.GetCapacity() );
            return m_renderViews.m_storage.m_allocator.GetData() + viewIndex;
        }

        inline DeviceRenderView const* GetDeviceRenderView( uint32_t viewIndex ) const
        {
            EE_ASSERT( viewIndex < m_renderViews.m_storage.GetCapacity() );
            return m_renderViews.m_storage.m_allocator.GetData() + viewIndex;
        }

        inline uint32_t GetDeviceRenderViewIndex( DeviceRenderView const* pRenderView ) const
        {
            EE_ASSERT( pRenderView >= m_renderViews.m_storage.m_allocator.GetData() && pRenderView < m_renderViews.m_storage.m_allocator.GetData() + m_renderViews.m_storage.GetCapacity() );
            return uint32_t( pRenderView - m_renderViews.m_storage.m_allocator.GetData() );
        }

        template <typename F>
        inline void ForEachRenderViewGroup( DeviceRenderViewType type, F fn )
        {
            uint32_t const numViews = GetRenderViewGroupSize( type );
            size_t nextGroupViewIndex = 0;

            m_renderViews.m_storage.m_allocator.ForEachAllocatedItem( [this, &nextGroupViewIndex, numViews, type, fn] ( DeviceRenderView* pRenderView, uint32_t viewIndex )
            {
                if ( viewIndex < nextGroupViewIndex || pRenderView->m_deviceRenderViewType != type )
                {
                    return;
                }

                fn( TArrayView<DeviceRenderView>( pRenderView, numViews ) );
                nextGroupViewIndex = viewIndex + numViews;
            } );
        }

        template <typename F>
        inline void ForEachRenderViewGroup( DeviceRenderViewType type, F fn ) const
        {
            uint32_t const numViews = GetRenderViewGroupSize( type );
            size_t nextGroupViewIndex = 0;

            m_renderViews.m_storage.m_allocator.ForEachAllocatedItem( [&nextGroupViewIndex, numViews, type, fn] ( DeviceRenderView const* pRenderView, size_t viewIndex )
            {
                if ( viewIndex < nextGroupViewIndex || pRenderView->m_deviceRenderViewType != type )
                {
                    return;
                }

                fn( TArrayView<DeviceRenderView const>( pRenderView, numViews ) );
                nextGroupViewIndex = viewIndex + numViews;
            } );
        }

        inline uint32_t GetRenderViewCapacity() const { return m_renderViews.m_storage.GetCapacity(); }

        //-------------------------------------------------------------------------

        void QueueMeshInstanceInitialize( MeshInstanceProxy const& meshInstanceProxy, uint32_t rootInstanceID, RHI::Buffer* pMeshBuffer, uint32_t shaderParametersOffsetIn32ByteBlocks, uint32_t numClusters, uint32_t lodMask, uint32_t instanceIndex, uint32_t clusterToInstanceBase, bool instanceHidden );
        void QueuePointLightInitialize( LightInstanceProxy const& lightInstanceProxy, uint16_t shadowMapHandle, uint32_t shadowMapResolution );
        void QueueSpotLightInitialize( LightInstanceProxy const& lightInstanceProxy, uint16_t shadowMapHandle, uint32_t shadowMapResolution );
        void QueueDecalInitialize( DecalInstanceProxy const& decalInstanceProxy, uint16_t albedoTexture, uint16_t pbrTexture, uint16_t normalTexture, uint16_t emissiveTexture, uint16_t parallaxTexture, uint16_t flags );

        //-------------------------------------------------------------------------

        // TODO: This is 2 functions for stupid reasons, need to refactor.
        // WorldSystem_Render has a dumb circular dependency when queueing instance initialize commands
        void UpdateDeviceResources_BeforeInstanceInitialize( RenderSystem* pRenderSystem );
        void UpdateDeviceResources_AfterInstanceInitialize( RenderSystem* pRenderSystem );

        void DispatchWorldUpdate( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex );
        void WaitForCopyTasks( RenderSystem* pRenderSystem );

        RHI::BufferHandle GetMeshInstanceRootPageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetSkinningTransformBufferHandle() const;
        RHI::BufferHandle GetMeshInstanceRootBufferHandle() const;

        RHI::BufferHandle GetDirectionalLightPageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetPointLightPageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetSpotLightPageBufferHandle( uint32_t frameIndex ) const;

        RHI::BufferHandle GetDirectionalLightBufferHandle() const;
        RHI::BufferHandle GetPointLightBufferHandle() const;
        RHI::BufferHandle GetSpotLightBufferHandle() const;

        RHI::BufferHandle GetDecalPageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetDecalBufferHandle() const;

        RHI::Buffer* GetMeshInstanceRootBuffer() const;

        //-------------------------------------------------------------------------

        RHI::Buffer* GetRenderViewBuffer() const;
        RHI::BufferHandle GetRenderViewBufferHandle() const;

        //-------------------------------------------------------------------------

        uint32_t GetNumMeshInstanceShaderPools() const;
        uint32_t GetMeshInstanceCapacity( size_t shaderIndex ) const;
        uint32_t GetClusterCapacity( size_t shaderIndex ) const;

        RHI::Buffer* GetMeshInstancePageBuffer( uint32_t shaderIndex, uint32_t frameIndex ) const;
        RHI::Buffer* GetMeshInstanceBuffer( uint32_t shaderIndex ) const;
        RHI::Buffer* GetClusterToInstanceBuffer( uint32_t shaderIndex ) const;

    private:

        //-------------------------------------------------------------------------

        struct MeshInstanceShaderPool
        {
            DeviceInstanceStorage<ShaderTypes::MeshInstance>                    m_instances = {};

            HandleAllocator<uint32_t>                                           m_clusterAllocator = {};
            DeviceResizeBuffer                                                  m_clusterToInstanceBuffer = {};
        };

        using MeshInstanceRootDeviceInstancePool = DeviceInstancePool<ShaderTypes::MeshInstanceRoot, ShaderTypes::MeshInstanceRootUpdateCommand>;
        using MeshInstanceDeviceInitializeQueue = DeviceInstanceInitializeQueue<ShaderTypes::MeshInstanceInitializeCommand>;
        using MeshInstanceDeviceUpdateCommands = DeviceInstanceUpdateCommands<ShaderTypes::MeshInstanceTransformUpdateCommand>;

        using RenderViewDeviceInstancePool = DeviceInstancePool<ShaderTypes::RenderView, ShaderTypes::RenderViewUpdateCommand, PageAllocator<DeviceRenderView, uint16_t>>;
        using SkinningTransformDeviceInstancePool = DeviceInstancePool<ShaderTypes::SkinningTransform, ShaderTypes::SkinningTransformUpdateCommand>;

        using DecalDeviceInstancePool = DeviceInstancePool<ShaderTypes::DecalInstance, ShaderTypes::DecalInstanceUpdateCommand>;
        using DecalDeviceInitializeQueue = DeviceInstanceInitializeQueue<ShaderTypes::DecalInstanceInitializeCommand>;

        using PointLightDeviceInstancePool = DeviceInstancePool<ShaderTypes::LightInstance_PointLight, ShaderTypes::PointLightTransformUpdateCommand>;
        using PointLightDeviceInitializeQueue = DeviceInstanceInitializeQueue<ShaderTypes::PointLightInitializeCommand>;

        using SpotLightDeviceInstancePool = DeviceInstancePool<ShaderTypes::LightInstance_SpotLight, ShaderTypes::SpotLightTransformUpdateCommand>;
        using SpotLightDeviceInitializeQueue = DeviceInstanceInitializeQueue<ShaderTypes::SpotLightInitializeCommand>;

        using DirectionalLightDeviceInstancePool = DeviceInstancePool<ShaderTypes::LightInstance_DirectionalLight, ShaderTypes::DirectionalLightUpdateCommand>;

    private:

        //-------------------------------------------------------------------------

        MeshInstanceShaderPool& GetMeshInstanceShaderPool( uint32_t shaderIndex );

    private:

        //-------------------------------------------------------------------------

        TaskSystem*                                                                 m_pTaskSystem = nullptr;
        RenderSystem*                                                               m_pRenderSystem = nullptr;

        ComputeShader const*                                                        m_pInitializeInstancesShader = nullptr;
        ComputeShader const*                                                        m_pUpdateInstancesShader = nullptr;
        ComputeShader const*                                                        m_pClusterToInstanceUpdateShader = nullptr;

        DeviceResizeBuffer                                                          m_meshInstanceBufferHandles = {};
        DeviceResizeBuffer                                                          m_clusterToInstanceBufferHandles = {};

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>                                 m_worldUpdateConstantBuffers = {};

        MeshInstanceRootDeviceInstancePool                                          m_meshInstanceRoots = {};
        TVector<MeshInstanceShaderPool>                                             m_meshInstanceShaderPools;

        MeshInstanceDeviceInitializeQueue                                           m_meshInstanceInitialize = {};
        MeshInstanceDeviceUpdateCommands                                            m_meshInstanceUpdates = {};

        RenderViewDeviceInstancePool                                                m_renderViews = {};
        SkinningTransformDeviceInstancePool                                         m_skinningTransforms = {};

        DecalDeviceInstancePool                                                     m_decals = {};
        DecalDeviceInitializeQueue                                                  m_decalInitialize = {};

        PointLightDeviceInstancePool                                                m_pointLights = {};
        PointLightDeviceInitializeQueue                                             m_pointLightInitialize = {};

        SpotLightDeviceInstancePool                                                 m_spotLights = {};
        SpotLightDeviceInitializeQueue                                              m_spotLightInitialize = {};

        DirectionalLightDeviceInstancePool                                          m_directionalLights = {};
    };
}

#pragma once

#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/Device/DeviceRenderView.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"
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

        // Render views
        //-------------------------------------------------------------------------

        RenderViewProxy AllocateRenderViews( DeviceRenderViewType type, uint32_t numViews );
        void DeallocateRenderViews( RenderViewProxy&& renderViewProxy );

        inline DeviceRenderView* GetDeviceRenderView( uint32_t viewIndex )
        {
            EE_ASSERT( viewIndex < m_renderViewAllocator.GetCapacityInItems() );
            return m_renderViewAllocator.GetData() + viewIndex;
        }

        inline DeviceRenderView const* GetDeviceRenderView( uint32_t viewIndex ) const
        {
            EE_ASSERT( viewIndex < m_renderViewAllocator.GetCapacityInItems() );
            return m_renderViewAllocator.GetData() + viewIndex;
        }

        inline uint32_t GetDeviceRenderViewIndex( DeviceRenderView const* pRenderView ) const
        {
            EE_ASSERT( pRenderView >= m_renderViewAllocator.GetData() && pRenderView < m_renderViewAllocator.GetData() + m_renderViewAllocator.GetCapacityInItems() );
            return uint32_t( pRenderView - m_renderViewAllocator.GetData() );
        }

        template <typename F>
        inline void ForEachRenderViewGroup( DeviceRenderViewType type, F fn )
        {
            uint32_t const numViews = GetRenderViewGroupSize( type );
            size_t nextGroupViewIndex = 0;

            m_renderViewAllocator.ForEachAllocatedItem( [this, &nextGroupViewIndex, numViews, type, fn] ( DeviceRenderView* pRenderView, uint32_t viewIndex )
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

            m_renderViewAllocator.ForEachAllocatedItem( [&nextGroupViewIndex, numViews, type, fn] ( DeviceRenderView const* pRenderView, size_t viewIndex )
            {
                if ( viewIndex < nextGroupViewIndex || pRenderView->m_deviceRenderViewType != type )
                {
                    return;
                }

                fn( TArrayView<DeviceRenderView const>( pRenderView, numViews ) );
                nextGroupViewIndex = viewIndex + numViews;
            } );
        }

        inline uint32_t GetRenderViewCapacity() const { return m_renderViewAllocator.GetCapacityInItems(); }

        //-------------------------------------------------------------------------

        void QueueMeshInstanceInitialize( MeshInstanceProxy const& meshInstanceProxy, uint32_t rootInstanceID, RHI::Buffer* pMeshBuffer, uint32_t shaderParametersOffsetIn32ByteBlocks, uint32_t numClusters, uint32_t lodMask, uint32_t instanceIndex, uint32_t clusterToInstanceBase, bool instanceHidden );
        void QueuePointLightInitialize( LightInstanceProxy const& lightInstanceProxy, uint16_t shadowMapHandle, uint32_t shadowMapResolution );
        void QueueSpotLightInitialize( LightInstanceProxy const& lightInstanceProxy, uint16_t shadowMapHandle, uint32_t shadowMapResolution );

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

        template <typename T>
        struct UpdateCommandsPool
        {
            UpdateCommandsPool() = default;

            UpdateCommandsPool( UpdateCommandsPool const& ) = delete;
            UpdateCommandsPool& operator=( UpdateCommandsPool const& ) = delete;

            UpdateCommandsPool( UpdateCommandsPool&& other ) noexcept
                : m_counter( other.m_counter.load() )
                , m_sequence( other.m_sequence )
                , m_memoryPool( eastl::move( other.m_memoryPool ) )
                , m_numUpdateCommands( other.m_numUpdateCommands )
            {}

            UpdateCommandsPool& operator=( UpdateCommandsPool&& other ) noexcept
            {
                m_counter.store( other.m_counter.load() );
                m_sequence = other.m_sequence;
                m_memoryPool = eastl::move( other.m_memoryPool );
                m_numUpdateCommands = other.m_numUpdateCommands;
                return *this;
            }

            void Initialize();
            void Shutdown();

            void Update();
            void Submit();

            //-------------------------------------------------------------------------

            eastl::atomic<uint32_t>                                                 m_counter = 0;
            uint64_t                                                                m_sequence = 0;
            PageMemoryPool<T>                                                       m_memoryPool = {};
            uint32_t                                                                m_numUpdateCommands = 0;
        };

        //-------------------------------------------------------------------------

        struct MeshInstanceShaderPool
        {
            HandleAllocator<uint32_t>                                           m_instanceAllocator = {};
            DeviceResizeBuffer                                                  m_instanceBuffer = {};
            TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                   m_instancePageBuffers = {};

            HandleAllocator<uint32_t>                                           m_clusterAllocator = {};
            DeviceResizeBuffer                                                  m_clusterToInstanceBuffer = {};
        };

        //-------------------------------------------------------------------------

        template<typename T>
        class CopyBufferDataTask final : public ITaskSet
        {
        public:

            T const* m_pSrcMemory = nullptr;
            T*       m_pDstMemory_WriteCombined = nullptr;

            CopyBufferDataTask() = default;

            CopyBufferDataTask( CopyBufferDataTask const& ) = delete;
            CopyBufferDataTask& operator=( CopyBufferDataTask const& ) = delete;

            CopyBufferDataTask( CopyBufferDataTask&& other ) noexcept
                : m_pSrcMemory( other.m_pSrcMemory )
                , m_pDstMemory_WriteCombined( other.m_pDstMemory_WriteCombined )
            {
                other.m_pSrcMemory = nullptr;
                other.m_pDstMemory_WriteCombined = nullptr;
            }

            CopyBufferDataTask& operator=( CopyBufferDataTask&& other ) noexcept
            {
                m_pSrcMemory = other.m_pSrcMemory;
                m_pDstMemory_WriteCombined = other.m_pDstMemory_WriteCombined;

                other.m_pSrcMemory = nullptr;
                other.m_pDstMemory_WriteCombined = nullptr;

                return *this;
            }

        private:

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadIndex ) override
            {
                EE_PROFILE_SCOPE_RENDER( "CopyBufferDataTask" );

                Memory::CopyToWriteCombined
                (
                    m_pDstMemory_WriteCombined + range.start,
                    m_pSrcMemory + range.start,
                    ( range.end - range.start ) * sizeof( T )
                );
            }
        };

    private:

        //-------------------------------------------------------------------------

        MeshInstanceShaderPool& GetMeshInstanceShaderPool( uint32_t shaderIndex );

        //-------------------------------------------------------------------------

        TaskSystem*                                                                 m_pTaskSystem = nullptr;
        RenderSystem*                                                               m_pRenderSystem = nullptr;

        ComputeShader const*                                                        m_pWorldUpdateShader = nullptr;
        ComputeShader const*                                                        m_pClusterToInstanceUpdateShader = nullptr;

        // TODO: Need some kind of scratch GPU memory allocator to avoid tracking all these buffers
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_initializeBuffers_MeshInstance = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_initializeBuffers_PointLight = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_initializeBuffers_SpotLight = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_MeshInstanceRoot = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_MeshInstance = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_DirectionalLight = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_PointLight = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_SpotLight = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_directionalLightPageBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_pointLightPageBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_spotLightPageBuffers = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_SkinningTransform = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_RenderView = {};

        // TODO: Use QueueBufferUpdate API instead of triple buffering it, should save a lot of memory
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_meshInstanceRootPageBuffers = {};

        DeviceResizeBuffer                                                          m_meshInstanceRootBuffer = {};
        DeviceResizeBuffer                                                          m_directionalLightBuffer = {};
        DeviceResizeBuffer                                                          m_pointLightBuffer = {};
        DeviceResizeBuffer                                                          m_spotLightBuffer = {};
        DeviceResizeBuffer                                                          m_skinningTransformBuffer = {};
        DeviceResizeBuffer                                                          m_renderViewBuffer = {};

        TVector<MeshInstanceShaderPool>                                             m_meshInstanceShaderPools;

        PageAllocator<DeviceRenderView, uint16_t>                                   m_renderViewAllocator = {};

        DeviceResizeBuffer                                                          m_meshInstanceBufferHandles = {};
        DeviceResizeBuffer                                                          m_clusterToInstanceBufferHandles = {};

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>                                 m_worldUpdateConstantBuffers = {};

        HandleAllocator<uint32_t>                                                   m_meshInstanceRootHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_skinningTransformHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_directionalLightHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_pointLightHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_spotLightHandleAllocator = {};

        UpdateCommandsPool<ShaderTypes::MeshInstanceRootUpdateCommand>              m_updatePool_MeshInstanceRoot = {};
        UpdateCommandsPool<ShaderTypes::MeshInstanceTransformUpdateCommand>         m_updatePool_MeshInstance = {};
        UpdateCommandsPool<ShaderTypes::DirectionalLightUpdateCommand>              m_updatePool_DirectionalLight = {};
        UpdateCommandsPool<ShaderTypes::PointLightTransformUpdateCommand>           m_updatePool_PointLight = {};
        UpdateCommandsPool<ShaderTypes::SpotLightTransformUpdateCommand>            m_updatePool_SpotLight = {};
        UpdateCommandsPool<ShaderTypes::SkinningTransformUpdateCommand>             m_updatePool_SkinningTransform = {};
        UpdateCommandsPool<ShaderTypes::RenderViewUpdateCommand>                    m_updatePool_RenderView = {};

        // TODO: Get rid of intermediate buffers and write directly to mapped buffers
        TAlignedVector<ShaderTypes::MeshInstanceInitializeCommand>                  m_initializeCommands_MeshInstance;
        TAlignedVector<ShaderTypes::PointLightInitializeCommand>                    m_initializeCommands_PointLight;
        TAlignedVector<ShaderTypes::SpotLightInitializeCommand>                     m_initializeCommands_SpotLight;

        CopyBufferDataTask<ShaderTypes::MeshInstanceInitializeCommand>              m_copyInitializeCommands_MeshInstance;
        CopyBufferDataTask<ShaderTypes::PointLightInitializeCommand>                m_copyInitializeCommands_PointLight;
        CopyBufferDataTask<ShaderTypes::SpotLightInitializeCommand>                 m_copyInitializeCommands_SpotLight;

        CopyBufferDataTask<ShaderTypes::MeshInstanceRootUpdateCommand>              m_copyUpdateCommands_MeshInstanceRoot;
        CopyBufferDataTask<ShaderTypes::MeshInstanceTransformUpdateCommand>         m_copyUpdateCommands_MeshInstance;
        CopyBufferDataTask<ShaderTypes::DirectionalLightUpdateCommand>              m_copyUpdateCommands_DirectionalLight;
        CopyBufferDataTask<ShaderTypes::PointLightTransformUpdateCommand>           m_copyUpdateCommands_PointLight;
        CopyBufferDataTask<ShaderTypes::SpotLightTransformUpdateCommand>            m_copyUpdateCommands_SpotLight;
        CopyBufferDataTask<ShaderTypes::SkinningTransformUpdateCommand>             m_copyUpdateCommands_SkinningTransform;
        CopyBufferDataTask<ShaderTypes::RenderViewUpdateCommand>                    m_copyUpdateCommands_RenderView;
    };
}

#pragma once

#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/Shaders/EngineShader.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Profiling.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Resource/ResourcePtr.h"

#include "Engine/Render/Shaders/Renderer/WorldUpdate.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;
    class SkeletalMesh;
    class RenderSystem;
    class Material;

    // World representation in device memory for rendering purposes
    //-------------------------------------------------------------------------

    class EE_ENGINE_API DeviceRenderWorld
    {
    public:

        uint32_t GetSkinningTransformBufferCapacity() const;

        uint32_t GetNumMeshInstancePages() const;

        uint32_t GetNumDirectionalLightPages() const;
        uint32_t GetNumPointLightPages() const;
        uint32_t GetNumSpotLightPages() const;

        void Initialize( TaskSystem* pTaskSystem, RenderSystem* pRenderSystem );
        void Shutdown( RenderSystem* pRenderSystem );

        MeshInstanceProxy AllocateMeshInstanceRoot( uint32_t num64ByteBlocks );
        void DeallocateMeshInstanceRoot( MeshInstanceProxy&& meshInstanceProxy );

        MeshInstanceProxy AllocateMeshInstance( uint32_t numInstances );
        void DeallocateMeshInstance( MeshInstanceProxy&& meshInstanceProxy );

        SkinningProxy AllocateSkinningInstance( uint32_t numBones );
        void DeallocateSkinningInstance( SkinningProxy&& skinningProxy );

        LightInstanceProxy AllocateDirectionalLight();
        LightInstanceProxy AllocatePointLight();
        LightInstanceProxy AllocateSpotLight();
        void DeallocateDirectionalLight( LightInstanceProxy&& lightInstanceProxy );
        void DeallocatePointLight( LightInstanceProxy&& lightInstanceProxy );
        void DeallocateSpotLight( LightInstanceProxy&& lightInstanceProxy );

        void QueueMeshInstanceInitialize( uint32_t instanceID, uint32_t rootInstanceID, Mesh const* pMesh, TArrayView<TResourcePtr<Material>> materialOverrides );

        // TODO: This is 2 functions for stupid reasons, need to refactor.
        // WorldSystem_Render has a dumb circular dependency when queueing instance initialize commands
        void UpdateDeviceResources_BeforeInstanceInitialize( RenderSystem* pRenderSystem );
        void UpdateDeviceResources_AfterInstanceInitialize( RenderSystem* pRenderSystem );

        void DispatchWorldUpdate( RHI::BufferHandle meshBuffer, RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex );
        void WaitForCopyTasks( RenderSystem* pRenderSystem );

        RHI::BufferHandle GetMeshInstancePageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetSkinningTransformBufferHandle() const;
        RHI::BufferHandle GetMeshInstanceBufferHandle() const;
        RHI::BufferHandle GetMeshInstanceRootBufferHandle() const;

        RHI::BufferHandle GetDirectionalLightPageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetPointLightPageBufferHandle( uint32_t frameIndex ) const;
        RHI::BufferHandle GetSpotLightPageBufferHandle( uint32_t frameIndex ) const;

        RHI::BufferHandle GetDirectionalLightBufferHandle() const;
        RHI::BufferHandle GetPointLightBufferHandle() const;
        RHI::BufferHandle GetSpotLightBufferHandle() const;

        RHI::Buffer* GetMeshInstanceBuffer() const;
        RHI::Buffer* GetMeshInstanceRootBuffer() const;

    private:

        template<typename T>
        class CopyBufferDataTask final : public ITaskSet
        {
        public:

            T const* m_pSrcMemory = nullptr;
            T*       m_pDstMemory_WriteCombined = nullptr;

        private:

            virtual void ExecuteRange( TaskSetPartition range, uint32_t threadIndex ) override
            {
                EE_PROFILE_SCOPE_RENDER( "CopyBufferDataTask" );

                Memory::CopyToWriteCombined(
                    m_pDstMemory_WriteCombined + range.start,
                    m_pSrcMemory + range.start,
                    ( range.end - range.start ) * sizeof( T ) );
            }
        };

        template <typename T>
        struct UpdateCommandsPool
        {
            void Initialize();
            void Shutdown();

            void Update();
            void Submit();

            //-----------------------------------------------------------------------------------------------

            eastl::atomic<uint32_t>                                                 m_counter = 0;
            uint64_t                                                                m_sequence = 0;
            PageMemoryPool<T>                                                       m_memoryPool = {};
            uint32_t                                                                m_numUpdateCommands = 0;
        };

    private:

        TaskSystem*                                                                 m_pTaskSystem = nullptr;

        ComputeShader const*                                                        m_pWorldUpdateShader = nullptr;

        Material const*                                                             m_pPlaceholderMaterial = nullptr;

        // TODO: Need some kind of scratch GPU memory allocator to avoid tracking all these buffers
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_initializeBuffers_MeshInstance = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_MeshInstanceRoot = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_MeshInstance = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_DirectionalLight = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_PointLight = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_SpotLight = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_directionalLightPageBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_pointLightPageBuffers = {};
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_spotLightPageBuffers = {};

        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_updateBuffers_SkinningTransform = {};

        // TODO: Use QueueBufferUpdate API instead of triple buffering it, should save a lot of memory
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>                           m_meshInstancePageBuffers = {};

        DeviceResizeBuffer                                                          m_meshInstanceRootBuffer = {};
        DeviceResizeBuffer                                                          m_meshInstanceBuffer = {};
        DeviceResizeBuffer                                                          m_directionalLightBuffer = {};
        DeviceResizeBuffer                                                          m_pointLightBuffer = {};
        DeviceResizeBuffer                                                          m_spotLightBuffer = {};
        DeviceResizeBuffer                                                          m_skinningTransformBuffer = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>                                 m_worldUpdateConstantBuffers = {};

        HandleAllocator<uint32_t>                                                   m_meshInstanceRootHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_meshInstanceHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_skinningTransformHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_directionalLightHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_pointLightHandleAllocator = {};
        HandleAllocator<uint32_t>                                                   m_spotLightHandleAllocator = {};

        UpdateCommandsPool<ShaderTypes::MeshInstanceTransformUpdateCommand>         m_updatePool_MeshInstanceRoot = {};
        UpdateCommandsPool<ShaderTypes::MeshInstanceTransformUpdateCommand>         m_updatePool_MeshInstance = {};
        UpdateCommandsPool<ShaderTypes::DirectionalLightUpdateCommand>              m_updatePool_DirectionalLight = {};
        UpdateCommandsPool<ShaderTypes::PointLightUpdateCommand>                    m_updatePool_PointLight = {};
        UpdateCommandsPool<ShaderTypes::SpotLightUpdateCommand>                     m_updatePool_SpotLight = {};
        UpdateCommandsPool<ShaderTypes::SkinningTransformUpdateCommand>             m_updatePool_SkinningTransform = {};

        // TODO: Get rid of intermediate buffers and write directly to mapped buffers
        TAlignedVector<ShaderTypes::MeshInstanceInitializeCommand>                  m_initializeCommands_MeshInstance;

        CopyBufferDataTask<ShaderTypes::MeshInstanceInitializeCommand>              m_copyInitializeCommands_MeshInstance;

        CopyBufferDataTask<ShaderTypes::MeshInstanceTransformUpdateCommand>         m_copyUpdateCommands_MeshInstanceRoot;
        CopyBufferDataTask<ShaderTypes::MeshInstanceTransformUpdateCommand>         m_copyUpdateCommands_MeshInstance;
        CopyBufferDataTask<ShaderTypes::DirectionalLightUpdateCommand>              m_copyUpdateCommands_DirectionalLight;
        CopyBufferDataTask<ShaderTypes::PointLightUpdateCommand>                    m_copyUpdateCommands_PointLight;
        CopyBufferDataTask<ShaderTypes::SpotLightUpdateCommand>                     m_copyUpdateCommands_SpotLight;
        CopyBufferDataTask<ShaderTypes::SkinningTransformUpdateCommand>             m_copyUpdateCommands_SkinningTransform;
    };
}

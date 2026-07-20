#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderAsyncResourceUpdate.h"
#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/Shaders/CommonSamplers.esh"
#include "Base/Systems.h"
#include "Base/Render/RHI.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Viewport;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Material;
    class Mesh;
    struct Geometry;

    class RenderSettings;
    class Window;
    class RenderViewport;

    namespace ShaderTypes
    {
        struct Mesh;
    }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderSystem : public ISystem
    {
    public:

        EE_SYSTEM( RenderSystem );

    private:

        #if EE_DEVELOPMENT_TOOLS
        enum class InternalStage
        {
            None = 0,
            ResourceUpdate,
            AsyncResourceUpdate,
            GraphicsUpdate
        };
        #endif

    public:

        //-------------------------------------------------------------------------

        void Initialize( RenderSettings const& settings );
        void Shutdown();

        void InitializeShaders();
        void ShutdownShaders();

        void WaitForFrameStart();

        void StartAsyncResourceUpdates( bool wait );
        void SubmitAsyncResourceUpdates( bool wait );

        void StartResourceUpdates( bool wait );
        void SubmitResourceUpdates( bool wait );

        void StartFrame();
        void SubmitFrame();

        void WaitAllQueuesIdle();
        void WaitGraphicsQueueIdle();
        void WaitComputeQueueIdle();
        void WaitTransferQueueIdle();

        inline uint32_t GetFrameIndex() const { return m_frameIndex; }

        inline RHI::Context* GetContextRHI() { return m_pContextRHI; }
        inline RHI::Queue* GetGraphicsQueue() { return m_pGraphicsQueue; }
        inline RHI::Queue* GetComputeQueue() { return m_pComputeQueue; }

        inline RHI::Sampler* GetPointWrapSampler() const { return m_commonSamplers[COMMON_SAMPLER_POINT_WRAP]; }
        inline RHI::Sampler* GetLinearWrapSampler() const { return m_commonSamplers[COMMON_SAMPLER_LINEAR_WRAP]; }
        inline RHI::Sampler* GetPointClampSampler() const { return m_commonSamplers[COMMON_SAMPLER_POINT_CLAMP]; }
        inline RHI::Sampler* GetLinearClampMaxSampler() const { return m_commonSamplers[COMMON_SAMPLER_LINEAR_CLAMP_MAX]; }

        inline void SetTonemapLUT( RHI::Texture* pTonemapLUT ) { m_pTonemapLUT = pTonemapLUT; }
        inline RHI::Texture* GetTonemapLUT() const { return m_pTonemapLUT; }

        inline void SetSMAAAreaTexture( RHI::Texture* pAreaTexture ) { m_pAreaTexture = pAreaTexture; }
        inline RHI::Texture* GetSMAAAreaTexture() const { return m_pAreaTexture; }

        inline void SetSMAASearchTexture( RHI::Texture* pSearchTexture ) { m_pSearchTexture = pSearchTexture; }
        inline RHI::Texture* GetSMAASearchTexture() const { return m_pSearchTexture; }

        inline void SetPlaceholderMaterial( Material const* pPlaceholderMaterial ) { m_pPlaceholderMaterial = pPlaceholderMaterial; }
        inline Material const* GetPlaceholderMaterial()const { return m_pPlaceholderMaterial; }

    public:

        void RegisterRenderWindow( Window* pRenderWindow );
        void UnregisterRenderWindow( Window* pRenderWindow );

        TArrayView<Window* const> GetRegisteredRenderWindows() const;
        bool IsSingleRenderWindow() const;

    public:

        // Viewports
        //-------------------------------------------------------------------------

        Viewport* CreateViewport( Render::Window* pRenderWindow );
        void DestroyViewport( Viewport* pViewport );

    public:

        template<typename... RHIResourceList>
        void QueueResourceDelete( RHIResourceList&&... resourcesRHI );

        // Resource create/update API
        //-------------------------------------------------------------------------

        template<typename F>
        RHI::Buffer* QueueBufferCreate( F copyFn, RHI::BufferParameters const& dstBufferParameters );

        template<typename F>
        void QueueBufferUpdate( F copyFn, RHI::Buffer* pDstBuffer, uint64_t dstOffset, uint64_t dstSize );

        void QueueBufferCopy( RHI::Buffer* pDstBuffer, size_t dstOffset, RHI::Buffer* pSrcBuffer, size_t srcOffset, size_t srcSize );

        template<typename F>
        RHI::Texture* QueueTextureCreate( F copyFn, RHI::TextureParameters const& dstTextureParameters );

        template<typename F>
        void QueueTextureUpdate( F copyFn, RHI::Texture* pDstTexture, RHI::TextureCopyRegion const& dstRegion, uint32_t numMipLevels, uint32_t numArraySlices, RHI::TextureState dstTextureState );

        // Meshes
        //-------------------------------------------------------------------------

        MeshUpdate CreateMesh( size_t numMeshes, size_t numClustersForAllMeshes );
        void WriteCommonMeshData( MeshUpdate const& meshUpdate, size_t dstMesh, size_t dstCluster, Geometry const& meshData ) const;
        void DeleteMesh( MeshHandle&& meshHandle, ClustersHandle&& clustersHandle );
        void QueueMeshUpdate( MeshHandle const& handle, ClustersHandle const& clustersHandle );

        RHI::BufferHandle GetMeshBufferHandle() const;
        RHI::BufferHandle GetClusterBufferHandle() const;

        // Shaders
        //-------------------------------------------------------------------------

        inline TVector<MaterialShader> const& GetMaterialShaders() const { return m_materialShaders; }
        inline TVector<SurfaceShader> const& GetSurfaceShaders() const { return m_surfaceShaders; }
        inline TVector<ComputeShader> const& GetComputeShaders() const { return m_computeShaders; }

        MaterialShader const* FindMaterialShader( StringID const& shaderID ) const;
        SurfaceShader const* FindSurfaceShader( StringID const& shaderID ) const;
        ComputeShader const* FindComputeShader( StringID const& shaderID ) const;

        // Find index of a shader by ID, use the index to reference shader buckets in the renderer.
        // Returns -1 when not found.
        int32_t FindMaterialShaderIndex( StringID const& shaderID ) const;

        // Shader parameters
        //-------------------------------------------------------------------------

        MaterialShaderParametersInstance CreateShaderParameters( size_t shaderIndex );
        void DeleteShaderParameters( MaterialShaderParametersInstance&& parameters );
        void QueueShaderParametersUpdate( MaterialShaderParametersInstance const& parameters );

        // Generic shader data - same as shader parameters
        //-------------------------------------------------------------------------

        ShaderDataHandle CreateShaderData( uint32_t shaderDataSizeInBytes );
        void DeleteShaderData( ShaderDataHandle&& instanceDataHandle );
        void QueueShaderDataUpdate( ShaderDataHandle const& instanceDataHandle );

        RHI::BufferHandle GetShaderDataBufferHandle() const;

        // Async API, can be used safely from multiple threads.
        // Uses a polling model and usually takes multiple frames to complete, see the corresponding data structures for more information.
        //-------------------------------------------------------------------------

        AsyncBufferUpdate* CreateBufferAsync( RHI::BufferParameters const& bufferParameters );
        AsyncTextureUpdate* CreateTextureAsync( RHI::TextureParameters const& textureParameters );
        AsyncMeshUpdate* CreateMeshAsync( size_t numMeshes, size_t numClustersForAllMeshes );
        AsyncMaterialParametersUpdate* CreateMaterialParametersAsync( size_t shaderIndex );
        AsyncShaderDataUpdate* CreateShaderDataAsync( uint32_t shaderDataSizeInBytes );

    private:

        static constexpr uint32_t MaxPendingTransfers = RHI::MaxPendingFrames * 2;
        static constexpr uint64_t StagingBufferAlignment = 32;

    private:

        RHI::Context*                                                           m_pContextRHI = nullptr;
        RHI::Queue*                                                             m_pGraphicsQueue = nullptr;
        RHI::Queue*                                                             m_pComputeQueue = nullptr;
        RHI::Queue*                                                             m_pTransferQueue = nullptr;

        uint32_t                                                                m_frameIndex = 0;
        uint32_t                                                                m_asyncTransferIndex = 0;

        #if EE_DEVELOPMENT_TOOLS
        TArray<InternalStage, RHI::MaxPendingFrames>                            m_internalStage = {};
        #endif

        TInlineVector<RenderViewport*, 3>                                       m_renderViewports;

        // Transfer queue stuff
        RHI::Buffer*                                                            m_pStagingBuffer = nullptr;

        TArray<RHI::CommandPool*, MaxPendingTransfers>                          m_asyncTransferCommandPools = {};
        TArray<RHI::CommandBuffer*, MaxPendingTransfers>                        m_asyncTransferCommandBuffers = {};
        TArray<uint64_t, MaxPendingTransfers>                                   m_asyncTransferSemaphores = {};

        TArray<RHI::CommandPool*, RHI::MaxPendingFrames>                        m_frameCommandPools = {};
        TArray<RHI::CommandBuffer*, RHI::MaxPendingFrames>                      m_frameCommandBuffers = {};
        TArray<uint64_t, RHI::MaxPendingFrames>                                 m_frameSemaphores = {};
        TArray<uint64_t, RHI::MaxPendingFrames>                                 m_resourceUpdateSemaphores = {};

        TArray<bool, RHI::MaxPendingFrames>                                     m_hasCopyBarriers = {};

        // Async resource updates
        Threading::ReadWriteMutex                                               m_asyncResourceUpdateMutex = {};

        TVector<AsyncBufferUpdate*>                                             m_asyncBufferUpdateQueue = {};
        TVector<AsyncTextureUpdate*>                                            m_asyncTextureUpdateQueue = {};
        TVector<AsyncMeshUpdate*>                                               m_asyncMeshUpdateQueue = {};
        TVector<AsyncShaderDataUpdate*>                                         m_asyncShaderDataUpdateQueue = {};
        TVector<AsyncMaterialParametersUpdate*>                                 m_asyncMaterialParametersUpdateQueue = {};

        // Delete queues
        Threading::ReadWriteMutex                                               m_resourceDeleteMutex = {};

        TVector<TPair<RHI::Buffer*, int32_t>>                                   m_resourceDeleteQueue_Buffer = {};
        TVector<TPair<RHI::Texture*, int32_t>>                                  m_resourceDeleteQueue_Texture = {};
        TVector<TPair<RHI::BufferSubAllocation, int32_t>>                       m_resourceDeleteQueue_StagingAllocation = {};
        TVector<TPair<TPair<MeshHandle, ClustersHandle>, int32_t>>              m_resourceDeleteQueue_Mesh = {};
        TVector<TPair<ShaderDataHandle, int32_t>>                               m_resourceDeleteQueue_ShaderData = {};
        TVector<TPair<MaterialShaderParametersInstance, int32_t>>               m_resourceDeleteQueue_MaterialParametersInstance = {};

        // Commonly used samplers
        TArray<RHI::Sampler*, 6>                                                m_commonSamplers = {};

        // Shaders
        TVector<MaterialShader>                                                 m_materialShaders;
        TVector<SurfaceShader>                                                  m_surfaceShaders;
        TVector<ComputeShader>                                                  m_computeShaders;

        // Shader data
        PageAllocator<Buffer32ByteBlock, uint32_t>                              m_shaderDataAllocator;
        RHI::Buffer*                                                            m_pShaderDataBuffer = nullptr;

        // Meshes
        PageAllocator<ShaderTypes::Mesh, uint16_t>                              m_meshAllocator;
        RHI::Buffer*                                                            m_pMeshBuffer = nullptr;

        PageAllocator<ShaderTypes::MeshCluster, uint32_t>                       m_meshClusterAllocator;
        RHI::Buffer*                                                            m_pMeshClusterBuffer = nullptr;

        // Render windows
        TVector<Window*>                                                        m_registeredRenderWindows;

        // Common resources
        RHI::Texture*                                                           m_pTonemapLUT = nullptr;
        RHI::Texture*                                                           m_pAreaTexture = nullptr;
        RHI::Texture*                                                           m_pSearchTexture = nullptr;
        Material const*                                                         m_pPlaceholderMaterial = nullptr;
    };

    //-----------------------------------------------------------------------------

    template<typename ...RHIResourceList>
    inline void RenderSystem::QueueResourceDelete( RHIResourceList && ...resourcesRHI )
    {
        Threading::ScopeLockWrite lock( m_resourceDeleteMutex );

        auto fn = [this] ( auto&& resource )
        {
            bool resourceValid = true;

            if constexpr ( eastl::is_pointer_v<decltype( resource )> )
            {
                resourceValid = resource != nullptr;
            }

            if ( resourceValid )
            {
                using ResourceType = eastl::decay_t<decltype( resource )>;

                static_assert( eastl::is_same_v<ResourceType, RHI::Buffer*> || eastl::is_same_v<ResourceType, RHI::Texture*> ||
                               eastl::is_same_v<ResourceType, DeviceBufferState> || eastl::is_same_v<ResourceType, DeviceTextureState> ||
                               eastl::is_same_v<ResourceType, TPair<MeshHandle, ClustersHandle>> ||
                               eastl::is_same_v<ResourceType, ShaderDataHandle> || eastl::is_same_v<ResourceType, MaterialShaderParametersInstance>,
                               "Unsupported resource type" );

                if constexpr ( eastl::is_same_v<ResourceType, RHI::Buffer*> || eastl::is_same_v<ResourceType, DeviceBufferState> )
                {
                    m_resourceDeleteQueue_Buffer.emplace_back( eastl::forward<decltype( resource )>( resource ), RHI::MaxPendingFrames );
                }
                else if constexpr ( eastl::is_same_v<ResourceType, RHI::Texture*> || eastl::is_same_v<ResourceType, DeviceTextureState> )
                {
                    m_resourceDeleteQueue_Texture.emplace_back( eastl::forward<decltype( resource )>( resource ), RHI::MaxPendingFrames );
                }
                else if constexpr ( eastl::is_same_v<ResourceType, TPair<MeshHandle, ClustersHandle>> )
                {
                    m_resourceDeleteQueue_Mesh.emplace_back( eastl::forward<decltype( resource )>( resource ), RHI::MaxPendingFrames );
                }
                else if constexpr ( eastl::is_same_v<ResourceType, ShaderDataHandle> )
                {
                    m_resourceDeleteQueue_ShaderData.emplace_back( eastl::forward<decltype( resource )>( resource ), RHI::MaxPendingFrames );
                }
                else if constexpr ( eastl::is_same_v<ResourceType, MaterialShaderParametersInstance> )
                {
                    m_resourceDeleteQueue_MaterialParametersInstance.emplace_back( eastl::forward<decltype( resource )>( resource ), RHI::MaxPendingFrames );
                }

                resource = {};
            }
        };

        ( fn( eastl::forward<RHIResourceList>( resourcesRHI ) ), ... );
    }

    template<typename F>
    inline RHI::Buffer* RenderSystem::QueueBufferCreate( F copyFn, RHI::BufferParameters const& dstBufferParameters )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_internalStage[m_frameIndex] == InternalStage::ResourceUpdate );

        RHI::Buffer* pDstBuffer = RHI::CreateBuffer( m_pContextRHI, dstBufferParameters );

        QueueBufferUpdate( copyFn, pDstBuffer, 0, dstBufferParameters.m_bufferSize );

        return pDstBuffer;
    }

    template<typename F>
    inline void RenderSystem::QueueBufferUpdate( F copyFn, RHI::Buffer* pDstBuffer, uint64_t dstOffset, uint64_t dstSize )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( ( dstOffset + dstSize ) <= pDstBuffer->m_size );
        EE_ASSERT( m_internalStage[m_frameIndex] == InternalStage::ResourceUpdate );

        if ( m_pTransferQueue->m_unifiedMemory || pDstBuffer->m_memoryType != RHI::ResourceMemoryType::DeviceLocal )
        {
            if ( !pDstBuffer->m_pMappedAddress_WriteCombined )
            {
                RHI::MapBuffer( m_pContextRHI, pDstBuffer, { dstOffset, dstSize } );
            }

            copyFn( static_cast<uint8_t*>( pDstBuffer->m_pMappedAddress_WriteCombined ) + dstOffset, dstSize );
        }
        else
        {
            RHI::CommandBuffer* pCommandBuffer = m_frameCommandBuffers[m_frameIndex];

            RHI::BufferSubAllocation stagingAllocation = RHI::BufferSubAllocate( m_pStagingBuffer, dstSize, StagingBufferAlignment );
            if ( !stagingAllocation.IsValid() )
            {
                //EE_LOG_MESSAGE( LogCategory::Render, "DeviceResourceUpdate", "Allocating extra staging buffer for buffer update: %.2fMb", float( pDstBuffer->m_size ) / ( 1024.0F * 1024.0F ) );

                RHI::BufferParameters extraStagingBufferParameters = {};
                extraStagingBufferParameters.m_bufferSize = dstSize;
                extraStagingBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
                extraStagingBufferParameters.m_nodeIndex = m_pTransferQueue->m_nodeIndex;
                extraStagingBufferParameters.m_queueType = RHI::QueueType::Transfer;
                extraStagingBufferParameters.m_flags.SetMultipleFlags( RHI::BufferFlags::NoDescriptors, RHI::BufferFlags::PersistentMap );
                extraStagingBufferParameters.m_debugName = "ExtraStagingBuffer";

                RHI::Buffer* pExtraStagingBuffer = RHI::CreateBuffer( m_pContextRHI, extraStagingBufferParameters );

                copyFn( static_cast<uint8_t*>( pExtraStagingBuffer->m_pMappedAddress_WriteCombined ), dstSize );

                RHI::CmdCopyBuffer( pCommandBuffer, pDstBuffer, dstOffset, pExtraStagingBuffer, 0, dstSize );

                QueueResourceDelete( eastl::move( pExtraStagingBuffer ) );
            }
            else
            {
                copyFn( static_cast<uint8_t*>( m_pStagingBuffer->m_pMappedAddress_WriteCombined ) + stagingAllocation.m_offset, dstSize );

                RHI::CmdCopyBuffer( pCommandBuffer, pDstBuffer, dstOffset, m_pStagingBuffer, stagingAllocation.m_offset, dstSize );

                m_resourceDeleteQueue_StagingAllocation.emplace_back( eastl::move( stagingAllocation ), RHI::MaxPendingFrames );
            }

            m_hasCopyBarriers[m_frameIndex] = true;
        }
    }

    inline void RenderSystem::QueueBufferCopy( RHI::Buffer* pDstBuffer, size_t dstOffset, RHI::Buffer* pSrcBuffer, size_t srcOffset, size_t srcSize )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_internalStage[m_frameIndex] == InternalStage::ResourceUpdate );

        RHI::CmdCopyBuffer( m_frameCommandBuffers[m_frameIndex], pDstBuffer, dstOffset, pSrcBuffer, srcOffset, srcSize );
    }

    template<typename F>
    inline RHI::Texture* RenderSystem::QueueTextureCreate( F copyFn, RHI::TextureParameters const& dstTextureParameters )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_internalStage[m_frameIndex] == InternalStage::ResourceUpdate );
        EE_ASSERT( dstTextureParameters.m_initialState != RHI::TextureState::Undefined );

        RHI::TextureParameters textureParameters = dstTextureParameters;
        textureParameters.m_initialState = RHI::TextureState::Common;

        RHI::TextureCopyRegion dstTextureRegion = {};
        dstTextureRegion.m_width = dstTextureParameters.m_width;
        dstTextureRegion.m_height = dstTextureParameters.m_height;
        dstTextureRegion.m_depth = dstTextureParameters.m_depth;

        RHI::Texture* pDstTexture = RHI::CreateTexture( m_pContextRHI, textureParameters );

        QueueTextureUpdate
        (
            copyFn, pDstTexture, dstTextureRegion,
            dstTextureParameters.m_mipLevels,
            dstTextureParameters.m_arrayLayers,
            dstTextureParameters.m_initialState
        );

        return pDstTexture;
    }

    template <typename F>
    inline void RenderSystem::QueueTextureUpdate
    (
        F copyFn,
        RHI::Texture* pDstTexture,
        RHI::TextureCopyRegion const& dstTextureRegion,
        uint32_t numDstMipLevels,
        uint32_t numDstArrayLayers, RHI::TextureState dstTextureState
    )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_internalStage[m_frameIndex] == InternalStage::ResourceUpdate );

        size_t sizeInBytes = 0;
        for ( uint32_t arrayLayer = 0; arrayLayer < numDstArrayLayers; ++arrayLayer )
        {
            for ( uint32_t mipLevel = 0; mipLevel < numDstMipLevels; ++mipLevel )
            {
                RHI::TextureCopyRegion mipRegion = dstTextureRegion.SkipTo( mipLevel, arrayLayer );

                size_t const numSrcRows = RHI::ComputeFormatNumRows( pDstTexture->m_format, mipRegion.m_height );
                size_t const dstRowStride = RHI::GetTextureCopyRowStride( pDstTexture, mipLevel, arrayLayer );
                sizeInBytes += numSrcRows * dstRowStride * mipRegion.m_depth;
            }
        }

        auto CopyRowByRow = [pDstTexture, dstTextureRegion, numDstMipLevels, numDstArrayLayers, copyFn] ( void* pDstMemory_WriteCombined )
        {
            size_t srcOffset = 0;
            size_t dstOffset = 0;
            for ( uint32_t arrayLayer = 0; arrayLayer < numDstArrayLayers; ++arrayLayer )
            {
                for ( uint32_t mipLevel = 0; mipLevel < numDstMipLevels; ++mipLevel )
                {
                    RHI::TextureCopyRegion copyRegion = dstTextureRegion.SkipTo( mipLevel, arrayLayer );
                    uint32_t               numSrcRows = RHI::ComputeFormatNumRows( pDstTexture->m_format, copyRegion.m_height );
                    uint32_t               srcRowStride = RHI::ComputeFormatRowStride( pDstTexture->m_format, copyRegion.m_width );
                    uint32_t               dstRowStride = RHI::GetTextureCopyRowStride( pDstTexture, mipLevel, arrayLayer );

                    for ( uint32_t slice = 0; slice < copyRegion.m_depth; ++slice )
                    {
                        for ( uint32_t row = 0; row < numSrcRows; ++row )
                        {
                            copyFn( static_cast<uint8_t*>( pDstMemory_WriteCombined ) + dstOffset, srcOffset, srcRowStride, row );
                            srcOffset += srcRowStride;
                            dstOffset += dstRowStride;
                        }
                    }
                }
            }
        };

        EE_ASSERT( dstTextureState != RHI::TextureState::Undefined );

        RHI::CommandBuffer* pCommandBuffer = m_frameCommandBuffers[m_frameIndex];

        RHI::Buffer* pExtraStagingBuffer = nullptr;

        RHI::BufferSubAllocation stagingAllocation = RHI::BufferSubAllocate( m_pStagingBuffer, sizeInBytes, StagingBufferAlignment );
        if ( !stagingAllocation.IsValid() )
        {
            // TODO: Figure out why we need to alloc for the entire texture, validation layers are complaining.
            size_t minimumBufferSize = RHI::ComputeFormatRowStride( pDstTexture->m_format, pDstTexture->m_width );
            minimumBufferSize *= pDstTexture->m_height;

            sizeInBytes = Math::Max( minimumBufferSize, sizeInBytes );

            //EE_LOG_MESSAGE( LogCategory::Render, "DeviceResourceUpdate", "Allocating extra staging buffer for texture update: %.2fMb", float( sizeInBytes ) / ( 1024.0F * 1024.0F ) );

            RHI::BufferParameters extraStagingBufferParameters = {};
            extraStagingBufferParameters.m_bufferSize = sizeInBytes;
            extraStagingBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            extraStagingBufferParameters.m_nodeIndex = m_pTransferQueue->m_nodeIndex;
            extraStagingBufferParameters.m_queueType = RHI::QueueType::Transfer;
            extraStagingBufferParameters.m_flags.SetMultipleFlags( RHI::BufferFlags::NoDescriptors, RHI::BufferFlags::PersistentMap );
            extraStagingBufferParameters.m_debugName = "ExtraStagingBuffer";

            pExtraStagingBuffer = RHI::CreateBuffer( m_pContextRHI, extraStagingBufferParameters );

            CopyRowByRow( pExtraStagingBuffer->m_pMappedAddress_WriteCombined );
        }
        else
        {
            CopyRowByRow( static_cast<uint8_t*>( m_pStagingBuffer->m_pMappedAddress_WriteCombined ) + stagingAllocation.m_offset );
        }

        size_t dstCopyOffset = 0;
        for ( uint32_t arrayLayer = dstTextureRegion.m_arrayLayer; arrayLayer < ( dstTextureRegion.m_arrayLayer + numDstArrayLayers ); ++arrayLayer )
        {
            for ( uint32_t mipLevel = dstTextureRegion.m_mipLevel; mipLevel < ( dstTextureRegion.m_mipLevel + numDstMipLevels ); ++mipLevel )
            {
                RHI::TextureCopyRegion dstCopyRegion = dstTextureRegion.SkipTo( mipLevel, arrayLayer );

                if ( stagingAllocation.IsValid() )
                {
                    RHI::CmdCopyTexture( pCommandBuffer,
                                         pDstTexture, dstCopyRegion,
                                         m_pStagingBuffer, dstCopyOffset + stagingAllocation.m_offset );
                }
                else
                {
                    EE_ASSERT( pExtraStagingBuffer );

                    RHI::CmdCopyTexture( pCommandBuffer,
                                         pDstTexture, dstCopyRegion,
                                         pExtraStagingBuffer, dstCopyOffset );
                }

                size_t const numSrcRows = RHI::ComputeFormatNumRows( pDstTexture->m_format, dstCopyRegion.m_height );
                size_t const dstRowStride = RHI::GetTextureCopyRowStride( pDstTexture, mipLevel, arrayLayer );
                dstCopyOffset += dstRowStride * numSrcRows * dstCopyRegion.m_depth;
            }
        }

        QueueResourceDelete( eastl::move( pExtraStagingBuffer ) );
        if ( stagingAllocation.IsValid() )
        {
            m_resourceDeleteQueue_StagingAllocation.emplace_back( eastl::move( stagingAllocation ), RHI::MaxPendingFrames );
        }

        m_hasCopyBarriers[m_frameIndex] = true;
    }
}
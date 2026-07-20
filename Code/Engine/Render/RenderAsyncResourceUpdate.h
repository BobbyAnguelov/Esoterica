#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Base/Render/HandleAllocator.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Render/RHI.h"
#include "Base/Types/Arrays.h"
#include "EASTL/atomic.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct Mesh;
        struct MeshCluster;
    }

    enum class AsyncResourceUpdateState
    {
        AllocatePending,
        UpdatePending,
        SubmitPending,
        TransferPending,
        CompletePending,
        Completed,
    };

    using Buffer32ByteBlock = uint32_t[8];

    using MeshHandle = PageAllocator<ShaderTypes::Mesh, uint16_t>::Handle;
    using ClustersHandle = PageAllocator<ShaderTypes::MeshCluster, uint32_t>::Handle;
    using ShaderDataHandle = PageAllocator<Buffer32ByteBlock, uint32_t>::Handle;

    struct MeshUpdate
    {
        MeshHandle                              m_meshHandle = {};
        ClustersHandle                          m_clustersHandle = {};
        TArrayView<ShaderTypes::Mesh>           m_deviceMeshes = {};
        TArrayView<ShaderTypes::MeshCluster>    m_deviceClusters = {};
    };

    // Async buffer update, everything in this struct is owned externally
    struct AsyncBufferUpdate
    {
        eastl::atomic<AsyncResourceUpdateState> m_updateState = AsyncResourceUpdateState::AllocatePending;

        uint8_t*                                m_pDstMemory_WriteCombined = nullptr;
        RHI::Buffer*                            m_pDstBuffer = nullptr;
        uint64_t                                m_waitSemaphore = 0;
        RHI::BufferParameters                   m_bufferParameters = {};
        RHI::BufferSubAllocation                m_stagingAllocation = {};
        uint64_t                                m_dstOffset = 0;
        uint64_t                                m_dstSize = 0;
    };

    // Async texture update, everything in this struct is owned externally
    struct AsyncTextureUpdate
    {
        eastl::atomic<AsyncResourceUpdateState> m_updateState = AsyncResourceUpdateState::AllocatePending;

        uint8_t*                                m_pDstMemory_WriteCombined = nullptr;
        RHI::Texture*                           m_pDstTexture = nullptr;
        uint64_t                                m_waitSemaphore = 0;
        RHI::TextureParameters                  m_textureParameters = {};
        RHI::BufferSubAllocation                m_stagingAllocation = {};
        RHI::TextureState                       m_dstTextureState = {};
        RHI::TextureCopyRegion                  m_dstCopyRegion = {};
        uint32_t                                m_numMipLevels = 0;
        uint32_t                                m_numArrayLayers = 0;
    };

    // Async shader data update, everything in this struct is owned externally
    struct AsyncShaderDataUpdate
    {
        eastl::atomic<AsyncResourceUpdateState> m_updateState = AsyncResourceUpdateState::AllocatePending;

        uint32_t                                m_shaderDataSizeInBytes = 0;

        ShaderDataHandle                        m_shaderDataHandle = {};
    };

    // Async shader data update, everything in this struct is owned externally
    struct AsyncMaterialParametersUpdate
    {
        eastl::atomic<AsyncResourceUpdateState> m_updateState = AsyncResourceUpdateState::AllocatePending;

        size_t                                  m_shaderIndex = 0;

        MaterialShaderParametersInstance        m_materialShaderParameters = {};
    };

    // Async mesh update, everything in this struct is owned externally
    struct AsyncMeshUpdate
    {
        eastl::atomic<AsyncResourceUpdateState> m_updateState = AsyncResourceUpdateState::AllocatePending;

        size_t                                  m_numMeshes = 0;
        size_t                                  m_numClustersForAllMeshes = 0;

        MeshUpdate                              m_meshUpdate = {};
    };
}

#pragma once

#include "Base/Math/Math.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_BASE_API Window final
    {
    public:

        inline bool IsValid() const { return m_pSwapchain != nullptr; }

        inline RHI::Swapchain* GetSwapchain() { return m_pSwapchain; }
        inline Int2 GetSwapchainSize() const { return m_swapchainSize; }

        RHI::Texture* GetSwapchainRenderTarget( size_t frameIndex );

        inline uint32_t GetCurrentImageIndex() const { return m_currentImageIndex; }

        inline RHI::Texture* GetCurrentRenderTarget() { return GetSwapchainRenderTarget( m_currentImageIndex ); }

        inline RHI::CommandPool* GetCommandPool( uint32_t frameIndex ) { return m_commandPools[frameIndex]; }

        RHI::CommandBuffer* GetActiveCommandBuffer( uint32_t frameIndex );

        RHI::CommandBuffer* AcquireGraphicsCommandBuffer( RHI::Context* pContextRHI, uint32_t frameIndex );
        RHI::CommandBuffer* AcquireComputeCommandBuffer( RHI::Context* pContextRHI, uint32_t frameIndex );

        void SetNativeWindowHandle( void* pNativeWindowHandle );

        void ResizeSwapchain( RHI::Context* pContextRHI, RHI::Queue* pGraphicsQueue, RHI::Queue* pComputeQueue, Int2 newSize );
        void DestroySwapchain( RHI::Context* pContextRHI );

        void StartFrame( RHI::Context* pContextRHI, uint32_t frameIndex );

    private:

        RHI::Swapchain*                                             m_pSwapchain = nullptr;
        void*                                                       m_pNativeWindowHandle = nullptr;
        Int2                                                        m_swapchainSize = Int2::Zero;
        uint32_t                                                    m_currentImageIndex = ~0U;

        TArray<RHI::CommandPool*, RHI::MaxPendingFrames>            m_commandPools = {};
        TArray<RHI::CommandPool*, RHI::MaxPendingFrames>            m_commandPoolsCompute = {};

        TArray<TVector<RHI::CommandBuffer*>, RHI::MaxPendingFrames> m_commandBuffers = {};
        TArray<uint32_t, RHI::MaxPendingFrames>                     m_commandBufferIndices = {};

        TArray<TVector<RHI::CommandBuffer*>, RHI::MaxPendingFrames> m_commandBuffersCompute = {};
        TArray<uint32_t, RHI::MaxPendingFrames>                     m_commandBufferIndicesCompute = {};
    };
}

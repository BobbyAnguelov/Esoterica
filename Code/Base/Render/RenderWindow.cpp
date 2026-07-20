#include "RenderWindow.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    RHI::Texture* Window::GetSwapchainRenderTarget( size_t frameIndex )
    {
        return m_pSwapchain->m_renderTargets[frameIndex];
    }

    void Window::SetNativeWindowHandle( void* pNativeWindowHandle )
    {
        EE_ASSERT( m_pNativeWindowHandle != pNativeWindowHandle );
        m_pNativeWindowHandle = pNativeWindowHandle;
    }

    void Window::ResizeSwapchain( RHI::Context* pContextRHI, RHI::Queue* pGraphicsQueue, RHI::Queue* pComputeQueue, Int2 newSize )
    {
        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            if ( !m_commandPools[frameIndex] )
            {
                RHI::CommandPoolParameters commandPoolParameters = {};
                commandPoolParameters.m_pQueue = pGraphicsQueue;
                commandPoolParameters.m_debugName.sprintf( "RenderWindow Command Pool %i", frameIndex );

                m_commandPools[frameIndex] = RHI::CreateCommandPool( pContextRHI, commandPoolParameters );

                commandPoolParameters.m_pQueue = pComputeQueue;
                commandPoolParameters.m_debugName.sprintf( "RenderWindow Command Pool Compute %i", frameIndex );

                m_commandPoolsCompute[frameIndex] = RHI::CreateCommandPool( pContextRHI, commandPoolParameters );
            }
        }

        RHI::SwapchainParameters swapchainParameters = {};
        swapchainParameters.m_width = newSize.m_x;
        swapchainParameters.m_height = newSize.m_y;
        swapchainParameters.m_presentQueues = { &pGraphicsQueue, 1 };
        swapchainParameters.m_pNativeWindowHandle = m_pNativeWindowHandle;

        RHI::DestroySwapchain( pContextRHI, eastl::move( m_pSwapchain ) );
        m_pSwapchain = RHI::CreateSwapchain( pContextRHI, swapchainParameters );
        m_swapchainSize = newSize;
    }

    void Window::DestroySwapchain( RHI::Context* pContextRHI )
    {
        RHI::DestroySwapchain( pContextRHI, eastl::move( m_pSwapchain ) );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::DestroyCommandPool( pContextRHI, eastl::move( m_commandPools[frameIndex] ) );
            RHI::DestroyCommandPool( pContextRHI, eastl::move( m_commandPoolsCompute[frameIndex] ) );

            for ( RHI::CommandBuffer*& pCommandBuffer : m_commandBuffers[frameIndex] )
            {
                RHI::DestroyCommandBuffer( pContextRHI, eastl::move( pCommandBuffer ) );
            }

            for ( RHI::CommandBuffer*& pCommandBuffer : m_commandBuffersCompute[frameIndex] )
            {
                RHI::DestroyCommandBuffer( pContextRHI, eastl::move( pCommandBuffer ) );
            }
        }
    }

    void Window::StartFrame( RHI::Context* pContextRHI, uint32_t frameIndex )
    {
        m_currentImageIndex = RHI::AcquireNextImage( pContextRHI, m_pSwapchain );

        m_commandBufferIndices[frameIndex] = 0;
        m_commandBufferIndicesCompute[frameIndex] = 0;

        RHI::CommandPool* pCommandPool = m_commandPools[frameIndex];
        RHI::ResetCommandPool( pContextRHI, pCommandPool );

        RHI::CommandPool* pCommandPoolCompute = m_commandPoolsCompute[frameIndex];
        RHI::ResetCommandPool( pContextRHI, pCommandPoolCompute );

        AcquireGraphicsCommandBuffer( pContextRHI, frameIndex ); // Acquire main command buffer this frame
    }

    RHI::CommandBuffer* Window::GetActiveCommandBuffer( uint32_t frameIndex )
    {
        return m_commandBuffers[frameIndex][m_commandBufferIndices[frameIndex] - 1];
    }

    RHI::CommandBuffer* Window::AcquireGraphicsCommandBuffer( RHI::Context* pContextRHI, uint32_t frameIndex )
    {
        uint32_t& commandBufferIndex = m_commandBufferIndices[frameIndex];
        TVector<RHI::CommandBuffer*>& commandBuffers = m_commandBuffers[frameIndex];

        if ( commandBuffers.size() <= commandBufferIndex )
        {
            RHI::CommandBufferParameters commandBufferParameters = {};
            commandBufferParameters.m_pCommandPool = m_commandPools[frameIndex];
            commandBufferParameters.m_debugName.sprintf( "RenderWindow Command Buffer %i (%i)", frameIndex, commandBufferIndex );

            commandBuffers.emplace_back( RHI::CreateCommandBuffer( pContextRHI, commandBufferParameters ) );
        }

        RHI::CommandBuffer* pCommandBuffer = commandBuffers[commandBufferIndex];
        commandBufferIndex++;

        RHI::BeginCommandBuffer( pCommandBuffer );

        return pCommandBuffer;
    }

    RHI::CommandBuffer* Window::AcquireComputeCommandBuffer( RHI::Context* pContextRHI, uint32_t frameIndex )
    {
        uint32_t& commandBufferIndex = m_commandBufferIndicesCompute[frameIndex];
        TVector<RHI::CommandBuffer*>& commandBuffers = m_commandBuffersCompute[frameIndex];

        if ( commandBuffers.size() <= commandBufferIndex )
        {
            RHI::CommandBufferParameters commandBufferParameters = {};
            commandBufferParameters.m_pCommandPool = m_commandPoolsCompute[frameIndex];
            commandBufferParameters.m_debugName.sprintf( "RenderWindow Command Buffer Compute %i (%i)", frameIndex, commandBufferIndex );

            commandBuffers.emplace_back( RHI::CreateCommandBuffer( pContextRHI, commandBufferParameters ) );
        }

        RHI::CommandBuffer* pCommandBuffer = commandBuffers[commandBufferIndex];
        commandBufferIndex++;

        RHI::BeginCommandBuffer( pCommandBuffer );

        return pCommandBuffer;
    }
}

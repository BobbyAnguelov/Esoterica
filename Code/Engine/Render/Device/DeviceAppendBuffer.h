
#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Render/RHI.h"
#include "Base/Types/String.h"

namespace EE::Render
{
    class RenderSystem;

    struct EE_ENGINE_API DeviceAppendBufferBase
    {
        String                                                      m_bufferName;

        // TODO: Bunch of mutable stuff here, we don't have/need multithreaded command buffer recording right now so it's a later problem.
        // Renderer is recording very small command buffers so it's not a performance issue, all culling work is done on the GPU.
        RHI::Buffer*                                                m_pDeviceCounterBuffer = nullptr;
        RHI::Buffer*                                                m_pDeviceBuffer = nullptr;

        TArray<RHI::Buffer*, RHI::MaxPendingFrames>                 m_hostCounterBuffers = {};
        TArray<RHI::Buffer*, RHI::MaxPendingFrames>                 m_hostBuffers = {};

        uint32_t                                                    m_maxBufferSize = 1;

        //-------------------------------------------------------------------------

        void Initialize( RHI::Context* pContextRHI, StringView name );
        void Shutdown( RHI::Context* pContextRHI );
        uint64_t GetAppendBufferHandle() const;
    };

    template <typename T>
    struct DeviceAppendBuffer final : public DeviceAppendBufferBase
    {
        eastl::conditional_t<eastl::is_same_v<T, void>,
            uint32_t,
            TAlignedVector<T>
        >                                                           m_bufferData;

        //-------------------------------------------------------------------------

        void UpdateBuffers( RenderSystem* pRenderSystem, uint32_t frameIndex, size_t stride, TBitFlags<RHI::DescriptorTypeFlags> descriptorTypeFlags );

        void Clear( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex ) const;
        void CopyResults( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex ) const;
        void Barrier( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex ) const;
    };

    //-------------------------------------------------------------------------

    template <typename T>
    inline void DeviceAppendBuffer<T>::UpdateBuffers( RenderSystem* pRenderSystem, uint32_t frameIndex, size_t stride, TBitFlags<RHI::DescriptorTypeFlags> descriptorTypeFlags )
    {
        if constexpr ( !eastl::is_same_v<T, void> )
        {
            static_assert( ( sizeof( T ) % RHI::BufferAlignment ) == 0, "Structure has to be aligned to RHI::BufferAlignment" );
            EE_ASSERT( stride == sizeof( T ) );
        }

        size_t maxBufferSizeInBytes = m_maxBufferSize * stride;

        if ( !m_pDeviceBuffer || m_pDeviceBuffer->m_size < maxBufferSizeInBytes )
        {
            RHI::BufferParameters bufferParameters = {};
            bufferParameters.m_bufferSize = maxBufferSizeInBytes;
            bufferParameters.m_bufferStride = stride;
            bufferParameters.m_descriptorTypes = descriptorTypeFlags;
            bufferParameters.m_debugName.sprintf( "AppendBuffer %s Device Buffer", m_bufferName.c_str() );

            pRenderSystem->QueueResourceDelete( eastl::move( m_pDeviceBuffer ) );
            m_pDeviceBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), bufferParameters );
        }

        uint32_t resultsCounter = *reinterpret_cast<uint32_t*>( m_hostCounterBuffers[frameIndex]->m_pMappedAddress_WriteCombined );
        m_maxBufferSize = Math::Max( m_maxBufferSize, resultsCounter );

        if constexpr ( !eastl::is_same_v<T, void> )
        {
            if ( m_hostBuffers[frameIndex] )
            {
                size_t hostDataSize = m_hostBuffers[frameIndex]->m_size / m_hostBuffers[frameIndex]->m_stride;
                size_t validDataSize = Math::Min( size_t( resultsCounter ), hostDataSize );

                m_bufferData.resize( validDataSize );

                Memory::CopyFromWriteCombined( m_bufferData.data(), m_hostBuffers[frameIndex]->m_pMappedAddress_WriteCombined, validDataSize * sizeof( T ) );
                Memory::WriteCombinedBarrier();
            }

            if ( !m_hostBuffers[frameIndex] || m_hostBuffers[frameIndex]->m_size < maxBufferSizeInBytes )
            {
                RHI::BufferParameters hostBufferParameters = {};
                hostBufferParameters.m_bufferSize = maxBufferSizeInBytes;
                hostBufferParameters.m_bufferStride = stride;
                hostBufferParameters.m_flags.SetMultipleFlags( RHI::BufferFlags::NoDescriptors, RHI::BufferFlags::PersistentMap );
                hostBufferParameters.m_memoryType = RHI::ResourceMemoryType::DeviceToHost;
                hostBufferParameters.m_debugName.sprintf( "AppendBuffer %s Host Buffer", m_bufferName.c_str() );

                RHI::DestroyBuffer( pRenderSystem->GetContextRHI(), eastl::move( m_hostBuffers[frameIndex] ) );
                m_hostBuffers[frameIndex] = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), hostBufferParameters );
            }
        }
    }

    template <typename T>
    inline void DeviceAppendBuffer<T>::Clear( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex ) const
    {
        RHI::CmdClearBuffer( pCommandBuffer, m_pDeviceCounterBuffer, 0 );
    }

    template <typename T>
    inline void DeviceAppendBuffer<T>::CopyResults( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex ) const
    {
        RHI::CmdBarrier( pCommandBuffer, m_pDeviceCounterBuffer, RHI::PipelineStage::AllShader, RHI::PipelineStage::Copy, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::CopySource );
        if constexpr ( !eastl::is_same_v<T, void> )
        {
            RHI::CmdBarrier( pCommandBuffer, m_pDeviceBuffer, RHI::PipelineStage::AllShader, RHI::PipelineStage::Copy, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::CopySource );
        }

        RHI::CmdCopyBuffer( pCommandBuffer, m_hostCounterBuffers[frameIndex], 0, m_pDeviceCounterBuffer, 0, m_pDeviceCounterBuffer->m_size );

        if constexpr ( !eastl::is_same_v<T, void> )
        {
            RHI::CmdCopyBuffer( pCommandBuffer, m_hostBuffers[frameIndex], 0, m_pDeviceBuffer, 0, m_pDeviceBuffer->m_size );
        }
    }

    template <typename T>
    inline void DeviceAppendBuffer<T>::Barrier( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex ) const
    {
        RHI::CmdBarrier( pCommandBuffer, m_pDeviceCounterBuffer, RHI::PipelineStage::Copy, RHI::PipelineStage::AllShader, RHI::ResourceAccess::CopySource, RHI::ResourceAccess::UnorderedAccess );
        if constexpr ( !eastl::is_same_v<T, void> )
        {
            RHI::CmdBarrier( pCommandBuffer, m_pDeviceBuffer, RHI::PipelineStage::Copy, RHI::PipelineStage::AllShader, RHI::ResourceAccess::CopySource, RHI::ResourceAccess::UnorderedAccess );
        }
    }
}

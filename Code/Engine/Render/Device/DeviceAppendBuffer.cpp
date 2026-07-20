
#include "DeviceAppendBuffer.h"

namespace EE::Render
{
    uint64_t DeviceAppendBufferBase::GetAppendBufferHandle() const
    {
        RHI::BufferHandle counterBufferHandle = RHI::GetBufferHandle( m_deviceCounterBuffer, RHI::DescriptorTypeFlags::RWBuffer );
        RHI::BufferHandle bufferHandle = RHI::GetBufferHandle( m_deviceBuffer, RHI::DescriptorTypeFlags::RWBuffer );

        uint64_t result = 0;

        result |= uint64_t( counterBufferHandle );
        result |= uint64_t( bufferHandle ) << 16;
        result |= uint64_t( m_maxBufferSize ) << 32;

        return result;
    }

    void DeviceAppendBufferBase::Initialize( RHI::Context* pContextRHI, StringView name )
    {
        EE_ASSERT( !m_deviceCounterBuffer );
        EE_ASSERT( m_bufferName.empty() || m_bufferName == name ); // Hotreload shenanigans
        EE_ASSERT( !name.empty() );

        m_bufferName = name;

        RHI::BufferParameters deviceCounterBufferParameters = {};
        deviceCounterBufferParameters.m_bufferSize = sizeof( uint32_t );
        deviceCounterBufferParameters.m_bufferStride = sizeof( uint32_t );
        deviceCounterBufferParameters.m_format = RHI::DataFormat::R32_UInt;
        deviceCounterBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::RWBuffer;
        deviceCounterBufferParameters.m_debugName.sprintf( "AppendBuffer %s Device Counter", m_bufferName.c_str() );

        m_deviceCounterBuffer = RHI::CreateBuffer( pContextRHI, deviceCounterBufferParameters );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::BufferParameters hostCounterBufferParameters = {};
            hostCounterBufferParameters.m_bufferSize = sizeof( uint32_t );
            hostCounterBufferParameters.m_bufferStride = sizeof( uint32_t );
            hostCounterBufferParameters.m_flags.SetMultipleFlags( RHI::BufferFlags::NoDescriptors, RHI::BufferFlags::PersistentMap );
            hostCounterBufferParameters.m_memoryType = RHI::ResourceMemoryType::DeviceToHost;
            hostCounterBufferParameters.m_debugName.sprintf( "AppendBuffer %s Host Counter %d", m_bufferName.c_str(), frameIndex );

            m_hostCounterBuffers[frameIndex] = RHI::CreateBuffer( pContextRHI, hostCounterBufferParameters );
        }
    }

    void DeviceAppendBufferBase::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_deviceCounterBuffer ) );
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_deviceBuffer ) );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_hostCounterBuffers[frameIndex] ) );
            RHI::DestroyBuffer( pContextRHI, eastl::move( m_hostBuffers[frameIndex] ) );
        }
    }
}

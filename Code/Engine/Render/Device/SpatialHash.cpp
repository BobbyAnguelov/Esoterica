
#include "SpatialHash.h"
#include "Engine/Render/RenderSystem.h"

namespace EE::Render
{
    void DeviceSpatialHash::Initialize( RHI::Context* pContextRHI, StringView name, uint32_t tableSize, uint32_t payloadStride )
    {
        EE_ASSERT( !m_keyBuffer.m_buffer );
        EE_ASSERT( !name.empty() );

        tableSize = Math::GetUpperPowerOfTwo( Math::Max( tableSize, 1U ) );

        m_name = name;
        m_tableSize = tableSize;
        m_payloadStride = payloadStride;

        m_keyBuffer.Initialize( pContextRHI );
        m_payloadBuffer.Initialize( pContextRHI );
    }

    void DeviceSpatialHash::Shutdown( RHI::Context* pContextRHI )
    {
        m_keyBuffer.Shutdown( pContextRHI );
        m_payloadBuffer.Shutdown( pContextRHI );
    }

    void DeviceSpatialHash::UpdateBuffers( RenderSystem* pRenderSystem, uint32_t frameIndex, uint32_t payloadStride )
    {
        m_payloadStride = payloadStride;

        m_keyBuffer.UpdateDeviceResources( m_tableSize * sizeof( uint32_t ), [this, pRenderSystem] ( DeviceBufferState&& oldBuffer, size_t newSize )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( oldBuffer ) );

            RHI::BufferParameters keyBufferParameters = {};
            keyBufferParameters.m_bufferSize = newSize;
            keyBufferParameters.m_format = RHI::DataFormat::R32_UInt;
            keyBufferParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            keyBufferParameters.m_debugName.sprintf( "SpatialHash %.*s Keys", m_name.length(), m_name.data() );
            return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), keyBufferParameters );
        } );

        m_payloadBuffer.UpdateDeviceResources( m_tableSize * payloadStride * 2 * sizeof( uint32_t ), [this, pRenderSystem] ( DeviceBufferState&& oldBuffer, size_t newSize )
        {
            pRenderSystem->QueueResourceDelete( eastl::move( oldBuffer ) );

            RHI::BufferParameters payloadBufferParameters = {};
            payloadBufferParameters.m_bufferSize = newSize;
            payloadBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
            payloadBufferParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            payloadBufferParameters.m_debugName.sprintf( "SpatialHash %.*s Payloads", m_name.length(), m_name.data() );
            return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), payloadBufferParameters );
        } );
    }

    void DeviceSpatialHash::Clear( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex )
    {
        RHI::CmdClearBuffer( pCommandBuffer, m_keyBuffer.m_buffer, 0 );
    }

    uint64_t DeviceSpatialHash::GetPackedHandleLow() const
    {
        uint64_t handle = 0;
        handle |= uint64_t( RHI::GetBufferHandle( m_keyBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer ) ) & 0xFFFF;
        handle |= ( uint64_t( RHI::GetBufferHandle( m_payloadBuffer.m_buffer, RHI::DescriptorTypeFlags::RWBuffer ) ) & 0xFFFF ) << 16;

        // log2(tableSize) in 5 bits (max tableSize = 2^31)
        uint32_t log2Size = 0;
        for ( uint32_t t = m_tableSize; t > 1; t >>= 1 ) ++log2Size;
        handle |= uint64_t( log2Size & 0x1FU ) << 32;

        // 5 LOD border values 4 bits each at bits [56:37]
        uint64_t borderBits = 0;
        for ( int lod = 1; lod < MaxLODs; ++lod )
            borderBits |= uint64_t( m_borderCells[lod] & 0xFU ) << ( ( lod - 1 ) * 4 );
        handle |= borderBits << 37;

        // numLODs-1 in 3 bits at [59:57]
        handle |= uint64_t( ( m_numLODs - 1 ) & 0x7U ) << 57;

        return handle;
    }

    uint64_t DeviceSpatialHash::GetPackedHandleHigh( float baseCellSize ) const
    {
        uint32_t cellSizeBits;
        memcpy( &cellSizeBits, &baseCellSize, sizeof( float ) );
        uint64_t handle = uint64_t( m_payloadStride ) & 0xFFFFFFFF;
        handle |= uint64_t( cellSizeBits ) << 32;
        return handle;
    }

    //-------------------------------------------------------------------------

    void DeviceSpatialHash::ComputeDispatchParameters
    (
        uint32_t rootDispatchX, uint32_t rootDispatchY, uint32_t rootDispatchZ,
        uint32_t numLODs, uint32_t const borderCells[MaxLODs],
        DeviceSpatialHashDispatchParameters outDispatches[MaxLODs]
    )
    {
        EE_ASSERT( rootDispatchX > 0 && rootDispatchY > 0 && rootDispatchZ > 0 );
        EE_ASSERT( outDispatches != nullptr );

        // Logical 8x8x8 grid drives subdivision and lookup matching
        int32_t logicalMin[3] = { -4, -4, -4 };
        int32_t logicalMax[3] = {  3,  3,  3 };

        // Physical dispatch starts from the actual root dimensions
        int32_t physicalMin[3] = {};
        int32_t physicalMax[3] = {};
        uint32_t const dims[3] = { rootDispatchX, rootDispatchY, rootDispatchZ };
        for ( int index = 0; index < 3; ++index )
        {
            physicalMin[index] = -int32_t( dims[index] / 2 );
            physicalMax[index] = physicalMin[index] + int32_t( dims[index] ) - 1;
        }

        for ( int lod = int( numLODs - 1 ); lod >= 0; --lod )
        {
            for ( int index = 0; index < 3; ++index )
            {
                outDispatches[lod].m_dispatchOffset[index] = physicalMin[index];
                outDispatches[lod].m_dispatchSize[index]   = uint32_t( physicalMax[index] - physicalMin[index] + 1 );
            }

            if ( lod > 0 )
            {
                int const border = borderCells[lod];

                for ( int index = 0; index < 3; ++index )
                {
                    int32_t lCenterMin = logicalMin[index] + border;
                    int32_t lCenterMax = logicalMax[index] - border;
                    logicalMin[index] = 2 * lCenterMin;
                    logicalMax[index] = 2 * lCenterMax + 1;

                    int32_t pCenterMin = Math::Max( physicalMin[index], lCenterMin );
                    int32_t pCenterMax = Math::Min( physicalMax[index], lCenterMax );
                    if ( pCenterMin <= pCenterMax )
                    {
                        physicalMin[index] = 2 * pCenterMin;
                        physicalMax[index] = 2 * pCenterMax + 1;
                    }
                    else
                    {
                        physicalMin[index] = 0;
                        physicalMax[index] = -1;
                    }
                }
            }
        }
    }
}

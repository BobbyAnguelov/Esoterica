#pragma once

#include "Base/Render/RHI.h"
#include "Engine/Render/Device/DeviceResourceState.h"

namespace EE::Render
{
    struct DeviceResizeBuffer final
    {
        RHI::Buffer*    m_pBuffer = nullptr;

        //-------------------------------------------------------------------------

        void Initialize( RHI::Context* pContextRHI );
        void Shutdown( RHI::Context* pContextRHI );

        template <typename F>
        void UpdateDeviceResources( size_t newBufferSize, F fn );
    };

    //-------------------------------------------------------------------------

    inline void DeviceResizeBuffer::Initialize( RHI::Context* pContextRHI )
    {
        // Nothing
    }

    inline void DeviceResizeBuffer::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_pBuffer ) );
    }

    template<typename F>
    inline void DeviceResizeBuffer::UpdateDeviceResources( size_t newBufferSize, F fn )
    {
        bool needNewBuffer = false;

        if ( !m_pBuffer ) { needNewBuffer = true; }

        if ( m_pBuffer )
        {
            if ( m_pBuffer->m_size < newBufferSize )
            {
                needNewBuffer = true;
            }

            size_t currentBufferSize = m_pBuffer->m_size;
            size_t sizeThreshold = ( newBufferSize / 3 ) * 2;

            if ( currentBufferSize > 4096 && currentBufferSize < sizeThreshold ) // Shrink if needed
            {
                needNewBuffer = true;
            }
        }

        if ( needNewBuffer )
        {
            m_pBuffer = fn( eastl::move( m_pBuffer ), newBufferSize );
        }
    }
}

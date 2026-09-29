#pragma once

#include "Base/Render/RHI.h"
#include "Engine/Render/Device/DeviceResourceState.h"

namespace EE::Render
{
    struct DeviceResizeBuffer final
    {
        void Initialize( RHI::Context* pContextRHI, bool allowShrink );
        void Shutdown( RHI::Context* pContextRHI );

        template <typename F>
        void UpdateDeviceResources( size_t newBufferSize, F fn );

        //-------------------------------------------------------------------------

        RHI::Buffer*    m_pBuffer = nullptr;

    private:

        bool            m_allowShrink = false;
    };

    //-------------------------------------------------------------------------

    inline void DeviceResizeBuffer::Initialize( RHI::Context* pContextRHI, bool allowShrink )
    {
        m_allowShrink = allowShrink;
    }

    inline void DeviceResizeBuffer::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_pBuffer ) );
    }

    template<typename F>
    inline void DeviceResizeBuffer::UpdateDeviceResources( size_t newBufferSize, F fn )
    {
        bool needNewBuffer = false;

        if ( !m_pBuffer )
        {
            needNewBuffer = true;
        }

        if ( m_pBuffer )
        {
            if ( m_pBuffer->m_size < newBufferSize )
            {
                needNewBuffer = true;
            }

            if ( m_allowShrink )
            {
                size_t const currentBufferSize = m_pBuffer->m_size;

                if ( currentBufferSize > 4096 && newBufferSize < ( currentBufferSize / 3 ) * 2 ) // Shrink if needed
                {
                    needNewBuffer = true;
                }
            }
        }

        if ( needNewBuffer )
        {
            m_pBuffer = fn( eastl::move( m_pBuffer ), newBufferSize );
        }
    }
}

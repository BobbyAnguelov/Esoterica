#pragma once

#include "Base/Render/RHI.h"
#include "Engine/Render/Device/DeviceResourceState.h"

namespace EE::Render
{
    template <typename T>
    struct DeviceResizeBufferBase final
    {
        T                           m_buffer = {};

        //---------------------------------------------------------------------------------------------------

        void Initialize( RHI::Context* pContextRHI );
        void Shutdown( RHI::Context* pContextRHI );

        template <typename F>
        void UpdateDeviceResources( size_t newBufferSize, F fn );
    };

    //-------------------------------------------------------------------------------------------------------

    template <typename T>
    inline void DeviceResizeBufferBase<T>::Initialize( RHI::Context* pContextRHI )
    {
        // Nothing
    }

    template <typename T>
    inline void DeviceResizeBufferBase<T>::Shutdown( RHI::Context* pContextRHI )
    {
        RHI::DestroyBuffer( pContextRHI, eastl::move( m_buffer ) );
    }

    template <typename T> template<typename F>
    inline void DeviceResizeBufferBase<T>::UpdateDeviceResources( size_t newBufferSize, F fn )
    {
        bool needNewBuffer = false;

        if ( !m_buffer ) { needNewBuffer = true; }

        if ( m_buffer )
        {
            if ( m_buffer->m_size < newBufferSize )
            {
                needNewBuffer = true;
            }

            size_t currentBufferSize = m_buffer->m_size;
            size_t sizeThreshold = ( newBufferSize / 3 ) * 2;

            if ( currentBufferSize > 4096 && currentBufferSize < sizeThreshold ) // Shrink if needed
            {
                needNewBuffer = true;
            }
        }

        if ( needNewBuffer )
        {
            m_buffer = fn( eastl::move( m_buffer ), newBufferSize );
        }
    }

    //-------------------------------------------------------------------------------------------------------

    using DeviceResizeBuffer = DeviceResizeBufferBase<RHI::Buffer*>;
    using DeviceResizeBufferState = DeviceResizeBufferBase<DeviceBufferState>;
}

#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Base/Render/RHI.h"
#include "Base/Types/String.h"

namespace EE::Render
{
    class RenderSystem;

    //-------------------------------------------------------------------------

    struct DeviceSpatialHashDispatchParameters
    {
        uint32_t    m_dispatchSize[3] = {};
        int32_t     m_dispatchOffset[3] = {};
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API DeviceSpatialHash
    {
    public:

        static constexpr uint32_t   s_maxLODs = 6;   // hard maximum, must match g_maxLodLevels in SpatialHash.esh

        //-------------------------------------------------------------------------

    public:

        static void ComputeDispatchParameters
        (
            uint32_t rootDispatchX, uint32_t rootDispatchY, uint32_t rootDispatchZ,
            uint32_t numLODs, uint32_t const borderCells[s_maxLODs],
            DeviceSpatialHashDispatchParameters outDispatches[s_maxLODs]
        );

        static uint32_t ComputePayloadStride( uint32_t numPointLightPages, uint32_t numSpotLightPages, uint32_t numDecalPages )
        {
            return ( 3 * 2 ) + ( 3 * Math::Max( Math::Max( numPointLightPages, numSpotLightPages ), numDecalPages ) );
        }

        //-------------------------------------------------------------------------

    public:

        void Initialize( RHI::Context* pContextRHI, StringView name, uint32_t tableSize, uint32_t payloadStride );
        void Shutdown( RHI::Context* pContextRHI );

        void UpdateBuffers( RenderSystem* pRenderSystem, uint32_t frameIndex, uint32_t payloadStride );
        void Clear( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex );

        uint64_t GetPackedHandleLow() const;
        uint64_t GetPackedHandleHigh( float baseCellSize ) const;

        //-------------------------------------------------------------------------

    public:

        String                      m_name;

        DeviceResizeBuffer          m_keyBuffer = {};
        DeviceResizeBuffer          m_payloadBuffer = {};

        uint32_t                    m_tableSize = 1024;                             // power of 2
        uint32_t                    m_payloadStride = 0;
        uint32_t                    m_numLODs = s_maxLODs;
        uint32_t                    m_borderCells[s_maxLODs] = { 0, 1, 1, 1, 2, 2 };  // 4 bits each, runtime configurable
    };
}


#pragma once

#include "Engine/Render/Shaders/EngineShader.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"

namespace EE::Render
{
    class DeviceRenderWorld;
    class RenderSystem;

    //-------------------------------------------------------------------------

    struct MaterialShaderRenderBucket
    {
        StringID                m_bucketName;
        DeviceBufferState       m_clusterVisibleCounterBuffer = {};
        DeviceResizeBufferState m_clusterVisibleBuffer = {};
        DeviceBufferState       m_drawCounterBuffer = {};
        DeviceResizeBufferState m_drawArgumentBuffer = {};

        void Initialize( RenderSystem* pRenderSystem );
        void Shutdown( RenderSystem* pRenderSystem );
    };

    //-------------------------------------------------------------------------

    struct DeviceRenderViewBucket
    {
        MaterialShaderRenderBucket m_opaqueBucket;
        MaterialShaderRenderBucket m_alphaTestBucket;
        MaterialShaderRenderBucket m_alphaBlendBucket; // TODO: Some render views don't care about alpha blend buckets, need to find a way to not allocate them when not needed

        void Initialize( RenderSystem* pRenderSystem );
        void Shutdown( RenderSystem* pRenderSystem );

        template<typename F>
        inline void ForEachRenderBucket( F fn )
        {
            fn( m_opaqueBucket );
            fn( m_alphaTestBucket );
            fn( m_alphaBlendBucket );
        }
    };

    // RenderView represents rendered world from a specific camera perspective, for instance main view, shadow maps, etc
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API DeviceRenderView
    {
        // TODO: This is ugly, need to keep in sync with the actual amount of buckets.
        // Make sure it is in sync with uint ShaderIndexToBucketIndex( uint shaderIndex ) shader function!
        static constexpr size_t s_NumRenderBucketsPerViewBucket = 3;

        TVector<DeviceRenderViewBucket> m_renderViewBuckets;

        template<typename F>
        inline void ForEachRenderBucket( F fn )
        {
            for ( DeviceRenderViewBucket& bucket : m_renderViewBuckets )
            {
                bucket.ForEachRenderBucket( fn );
            }
        }

        void Initialize( RenderSystem* pRenderSystem, size_t numMaterialShaderPipelineBuckets );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateDeviceResources( RenderSystem* pRenderSystem, TArrayView<uint32_t const> clusterCapacityPerShader );
    };
}

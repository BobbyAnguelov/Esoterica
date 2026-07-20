
#include "DeviceRenderView.h"
#include "DeviceRenderWorld.h"
#include "base/Render/RHI.h"

#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"

namespace EE::Render
{
    void MaterialShaderRenderBucket::Initialize( RenderSystem* pRenderSystem )
    {
        m_clusterVisibleBuffer.Initialize( pRenderSystem->GetContextRHI() );
        m_drawArgumentBuffer.Initialize( pRenderSystem->GetContextRHI() );
    }

    void MaterialShaderRenderBucket::Shutdown( RenderSystem* pRenderSystem )
    {
        RHI::DestroyBuffer( pRenderSystem->GetContextRHI(), eastl::move( m_clusterVisibleCounterBuffer ) );
        RHI::DestroyBuffer( pRenderSystem->GetContextRHI(), eastl::move( m_drawCounterBuffer ) );

        m_clusterVisibleBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_drawArgumentBuffer.Shutdown( pRenderSystem->GetContextRHI() );
    }

    //-------------------------------------------------------------------------------------------------------

    void DeviceRenderViewBucket::Initialize( RenderSystem* pRenderSystem )
    {
        m_opaqueBucket.Initialize( pRenderSystem );
        m_alphaTestBucket.Initialize( pRenderSystem );
        m_alphaBlendBucket.Initialize( pRenderSystem );
    }

    void DeviceRenderViewBucket::Shutdown( RenderSystem* pRenderSystem )
    {
        m_opaqueBucket.Shutdown( pRenderSystem );
        m_alphaTestBucket.Shutdown( pRenderSystem );
        m_alphaBlendBucket.Shutdown( pRenderSystem );
    }

    //-------------------------------------------------------------------------------------------------------

    void DeviceRenderView::Initialize( RenderSystem* pRenderSystem, size_t numMaterialShaderPipelineBuckets )
    {
        m_renderViewBuckets.reserve( numMaterialShaderPipelineBuckets );
        for ( size_t bucketIndex = 0; bucketIndex < numMaterialShaderPipelineBuckets; ++bucketIndex )
        {
            DeviceRenderViewBucket renderViewBucket = {};
            renderViewBucket.Initialize( pRenderSystem );

            renderViewBucket.m_opaqueBucket.m_bucketName = StringID( "Opaque" );
            renderViewBucket.m_alphaTestBucket.m_bucketName = StringID( "AlphaTest" );
            renderViewBucket.m_alphaBlendBucket.m_bucketName = StringID( "AlphaBlend" );

            m_renderViewBuckets.emplace_back( eastl::move( renderViewBucket ) );
        }
    }

    void DeviceRenderView::Shutdown( RenderSystem* pRenderSystem )
    {
        for ( DeviceRenderViewBucket& renderViewBucket : m_renderViewBuckets )
        {
            renderViewBucket.Shutdown( pRenderSystem );
        }
        m_renderViewBuckets.clear();
    }

    void DeviceRenderView::UpdateDeviceResources( RenderSystem* pRenderSystem, TArrayView<uint32_t const> clusterCapacityPerShader )
    {
        size_t renderBucketIndex = 0;
        for ( size_t shaderIndex = 0; shaderIndex < m_renderViewBuckets.size(); ++shaderIndex )
        {
            DeviceRenderViewBucket& bucket = m_renderViewBuckets[shaderIndex];

            bucket.ForEachRenderBucket( [&renderBucketIndex, shaderIndex, &clusterCapacityPerShader, pRenderSystem] ( MaterialShaderRenderBucket& renderBucket )
            {
                uint32_t const clustersCapacity = clusterCapacityPerShader[shaderIndex];
                size_t const clusterVisibleBufferSizeWorstCase = clustersCapacity * sizeof( uint2 );
                size_t const drawArgumentBufferSizeWorstCase = Math::IntegerDivideCeiling<uint32_t>( clustersCapacity, RHI::MaxDispatchSize ) * sizeof( ShaderTypes::DrawArgument );

                //-------------------------------------------------------------------------------------------

                auto UpdateBuffer_ClusterVisible = [pRenderSystem, &renderBucket, renderBucketIndex] ( DeviceBufferState&& oldBuffer, size_t newBufferSize )
                {
                    pRenderSystem->QueueResourceDelete( eastl::move( oldBuffer ) );

                    RHI::BufferParameters clusterVisibleBufferParameters = {};
                    clusterVisibleBufferParameters.m_bufferSize = newBufferSize;
                    clusterVisibleBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
                    clusterVisibleBufferParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
                    clusterVisibleBufferParameters.m_debugName.sprintf( "%s ClusterVisible Buffer %i", renderBucket.m_bucketName.c_str(), renderBucketIndex );

                    return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), clusterVisibleBufferParameters );
                };

                renderBucket.m_clusterVisibleBuffer.UpdateDeviceResources( clusterVisibleBufferSizeWorstCase, UpdateBuffer_ClusterVisible );

                //-------------------------------------------------------------------------------------------

                if ( !renderBucket.m_drawCounterBuffer )
                {
                    RHI::BufferParameters countBufferParameters = {};
                    countBufferParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
                    countBufferParameters.m_bufferSize = sizeof( uint32_t );
                    countBufferParameters.m_format = RHI::DataFormat::R32_UInt;
                    countBufferParameters.m_debugName.sprintf( "%s DrawCounter Buffer %i", renderBucket.m_bucketName.c_str(), renderBucketIndex );

                    renderBucket.m_drawCounterBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), countBufferParameters );

                    countBufferParameters.m_debugName.sprintf( "%s ClusterVisibleCounter Buffer %i", renderBucket.m_bucketName.c_str(), renderBucketIndex );

                    renderBucket.m_clusterVisibleCounterBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), countBufferParameters );
                }

                //-------------------------------------------------------------------------------------------

                auto UpdateBuffer_DrawArgument = [pRenderSystem, &renderBucket, renderBucketIndex] ( DeviceBufferState&& oldBuffer, size_t newBufferSize )
                {
                    pRenderSystem->QueueResourceDelete( eastl::move( oldBuffer ) );

                    RHI::BufferParameters drawArgumentBufferParameters = {};
                    drawArgumentBufferParameters.m_alignment = RHI::IndirectCommandAlignment;
                    drawArgumentBufferParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::RWBuffer );
                    drawArgumentBufferParameters.m_bufferSize = newBufferSize;
                    drawArgumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::DrawArgument );
                    drawArgumentBufferParameters.m_debugName.sprintf( "%s DrawArgument Buffer %i", renderBucket.m_bucketName.c_str(), renderBucketIndex );

                    return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), drawArgumentBufferParameters );
                };

                renderBucket.m_drawArgumentBuffer.UpdateDeviceResources( drawArgumentBufferSizeWorstCase, UpdateBuffer_DrawArgument );

                //-------------------------------------------------------------------------------------------

                renderBucketIndex++;
            } );
        }
    }
}

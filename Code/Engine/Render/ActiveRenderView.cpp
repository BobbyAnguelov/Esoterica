#include "ActiveRenderView.h"

#include "Base/Esoterica.h"
#include "Base/Profiling.h"
#include "Base/Render/RHI.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void ActiveRenderViewMaterialShaderBucket::Initialize( RHI::Context* pContextRHI )
    {
        m_drawArgumentBuffer.Initialize( pContextRHI, false );
    }

    void ActiveRenderViewMaterialShaderBucket::Shutdown( RenderSystem* pRenderSystem )
    {
        pRenderSystem->QueueResourceDelete( eastl::move( m_pDrawCounterBuffer ), eastl::move( m_drawArgumentBuffer.m_pBuffer ) );
    }

    //-------------------------------------------------------------------------

    void ActiveRenderViewBucket::Initialize( RHI::Context* pContextRHI )
    {
        m_opaqueBucket.Initialize( pContextRHI );
        m_alphaTestBucket.Initialize( pContextRHI );
        m_alphaBlendBucket.Initialize( pContextRHI );
    }

    void ActiveRenderViewBucket::Shutdown( RenderSystem* pRenderSystem )
    {
        m_opaqueBucket.Shutdown( pRenderSystem );
        m_alphaTestBucket.Shutdown( pRenderSystem );
        m_alphaBlendBucket.Shutdown( pRenderSystem );
    }

    //-------------------------------------------------------------------------

    void ActiveRenderView::Initialize( RHI::Context* pContextRHI, size_t numMaterialShaderPipelineBuckets )
    {
        m_renderViewBuckets.reserve( numMaterialShaderPipelineBuckets );
        for ( size_t bucketIndex = 0; bucketIndex < numMaterialShaderPipelineBuckets; ++bucketIndex )
        {
            ActiveRenderViewBucket renderViewBucket = {};
            renderViewBucket.Initialize( pContextRHI );

            m_renderViewBuckets.emplace_back( eastl::move( renderViewBucket ) );
        }
    }

    void ActiveRenderView::Shutdown( RenderSystem* pRenderSystem )
    {
        for ( ActiveRenderViewBucket& renderViewBucket : m_renderViewBuckets )
        {
            renderViewBucket.Shutdown( pRenderSystem );
        }
        m_renderViewBuckets.clear();
    }

    void ActiveRenderView::UpdateDeviceResources( RenderSystem* pRenderSystem, DeviceRenderWorld const& deviceRenderWorld )
    {
        size_t renderBucketIndex = 0;
        for ( size_t shaderIndex = 0; shaderIndex < m_renderViewBuckets.size(); ++shaderIndex )
        {
            ActiveRenderViewBucket& bucket = m_renderViewBuckets[shaderIndex];

            bucket.ForEachRenderBucket( [&renderBucketIndex, shaderIndex, pRenderSystem, &deviceRenderWorld] ( ActiveRenderViewMaterialShaderBucket& renderBucket )
            {
                uint32_t const clustersCapacity = deviceRenderWorld.GetClusterCapacity( shaderIndex );
                EE_ASSERT( clustersCapacity > 0 );

                uint32_t const maxNumDrawArguments = ( clustersCapacity + RHI::Limits::MaxDispatchSize - 1 ) / RHI::Limits::MaxDispatchSize;
                size_t const drawArgumentBufferSizeWorstCase = maxNumDrawArguments * sizeof( ShaderTypes::DrawArgument );

                //-------------------------------------------------------------------------

                if ( !renderBucket.m_pDrawCounterBuffer )
                {
                    RHI::BufferParameters countBufferParameters = {};
                    countBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer, RHI::DescriptorTypeFlags::Raw };
                    countBufferParameters.m_bufferSize = sizeof( uint32_t );
                    countBufferParameters.m_bufferStride = sizeof( uint32_t );
                    countBufferParameters.m_debugName.sprintf( "%s DrawCounter Buffer %i", renderBucket.m_bucketName.c_str(), renderBucketIndex );

                    renderBucket.m_pDrawCounterBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), countBufferParameters );
                }

                //-------------------------------------------------------------------------

                auto UpdateBuffer_DrawArgument = [pRenderSystem, &renderBucket, renderBucketIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
                {
                    pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                    RHI::BufferParameters drawArgumentBufferParameters = {};
                    drawArgumentBufferParameters.m_alignment = RHI::g_indirectCommandAlignment;
                    drawArgumentBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::IndirectArgumentBuffer, RHI::DescriptorTypeFlags::RWBuffer };
                    drawArgumentBufferParameters.m_bufferSize = newBufferSize;
                    drawArgumentBufferParameters.m_bufferStride = sizeof( ShaderTypes::DrawArgument );
                    drawArgumentBufferParameters.m_debugName.sprintf( "%s DrawArgument Buffer %i", renderBucket.m_bucketName.c_str(), renderBucketIndex );

                    return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), drawArgumentBufferParameters );
                };

                renderBucket.m_drawArgumentBuffer.UpdateDeviceResources( drawArgumentBufferSizeWorstCase, UpdateBuffer_DrawArgument );

                //-------------------------------------------------------------------------

                renderBucketIndex++;
            } );
        }
    }

    void ActiveRenderViewSelection::SelectActiveRenderViews
    (
        DeviceRenderWorld const&    deviceRenderWorld,
        ActiveRenderViewList&       activeRenderViewList,
        uint32_t                    mainRenderViewIndex,
        uint32_t                    editorOutlineRenderViewIndex
    )
    {
        uint32_t const renderViewCapacity = deviceRenderWorld.GetRenderViewCapacity();

        //-------------------------------------------------------------------------

        uint16_t* const pDeviceRenderViewIndices = activeRenderViewList.m_deviceRenderViewIndicesPerActiveRenderView;
        uint32_t numActiveRenderViews = 0;

        auto AppendRenderView = [pDeviceRenderViewIndices, &numActiveRenderViews] ( uint32_t deviceRenderViewIndex )
        {
            if ( numActiveRenderViews >= EE_MAX_CULLING_VIEWS )
            {
                return;
            }

            pDeviceRenderViewIndices[numActiveRenderViews] = uint16_t( deviceRenderViewIndex );
            numActiveRenderViews++;
        };

        //-------------------------------------------------------------------------

        bool const hasEditorOutlineRenderView = ( editorOutlineRenderViewIndex != ActiveRenderViewList::s_InvalidActiveRenderViewIndex );

        activeRenderViewList.m_mainRenderViewActiveIndex = numActiveRenderViews;
        AppendRenderView( mainRenderViewIndex );

        if ( hasEditorOutlineRenderView )
        {
            activeRenderViewList.m_editorOutlineRenderViewActiveIndex = numActiveRenderViews;
            AppendRenderView( editorOutlineRenderViewIndex );
        }

        uint32_t const numAlwaysActiveViews = numActiveRenderViews;

        uint32_t numReservedShadowViews = 0;
        deviceRenderWorld.ForEachRenderViewGroup( DeviceRenderViewType::CascadedShadowMap, [&] ( TArrayView<DeviceRenderView const> shadowViews )
        {
            numReservedShadowViews += uint32_t( shadowViews.size() );

            for ( uint32_t viewIndex = 0; viewIndex < shadowViews.size(); ++viewIndex )
            {
                AppendRenderView( deviceRenderWorld.GetDeviceRenderViewIndex( shadowViews.data() + viewIndex ) );
            }
        } );

        //-------------------------------------------------------------------------

        EE_ASSERT( numActiveRenderViews == numAlwaysActiveViews + numReservedShadowViews );

        if ( renderViewCapacity > 0 )
        {
            uint32_t const rotationStartViewIndex = m_rotationStartViewIndex % renderViewCapacity;
            uint32_t numFreeSlots = EE_MAX_CULLING_VIEWS - numActiveRenderViews;

            m_rotationStartViewIndex = rotationStartViewIndex;

            auto AdmitRotatableGroups = [&] ( bool admitGroupsAfterRotationStart )
            {
                uint32_t nextGroupStartIndex = 0;

                for ( uint32_t renderViewIndex = 0; renderViewIndex < renderViewCapacity && numFreeSlots > 0; ++renderViewIndex )
                {
                    if ( renderViewIndex < nextGroupStartIndex )
                    {
                        continue;
                    }

                    DeviceRenderView const* const pRenderView = deviceRenderWorld.GetDeviceRenderView( renderViewIndex );
                    DeviceRenderViewType const deviceRenderViewType = pRenderView->m_deviceRenderViewType;

                    uint32_t const numViewsInGroup = GetRenderViewGroupSize( deviceRenderViewType );
                    nextGroupStartIndex = renderViewIndex + numViewsInGroup;

                    if ( !pRenderView->IsValid() || ( pRenderView->m_colorTexture.m_pTexture == nullptr && pRenderView->m_depthTexture.m_pTexture == nullptr ) )
                    {
                        continue;
                    }

                    if ( deviceRenderViewType == DeviceRenderViewType::CascadedShadowMap || deviceRenderViewType == DeviceRenderViewType::Main )
                    {
                        continue;
                    }

                    #if EE_DEVELOPMENT_TOOLS
                    if ( deviceRenderViewType == DeviceRenderViewType::EditorOutline )
                    {
                        continue;
                    }
                    #endif

                    if ( numViewsInGroup > numFreeSlots )
                    {
                        continue;
                    }

                    if ( admitGroupsAfterRotationStart != ( renderViewIndex >= rotationStartViewIndex ) )
                    {
                        continue;
                    }

                    for ( uint32_t viewIndex = 0; viewIndex < numViewsInGroup; ++viewIndex )
                    {
                        AppendRenderView( renderViewIndex + viewIndex );
                    }

                    numFreeSlots -= numViewsInGroup;
                    m_rotationStartViewIndex = renderViewIndex + numViewsInGroup;
                }
            };

            AdmitRotatableGroups( true );
            AdmitRotatableGroups( false );
        }

        EE_ASSERT( numActiveRenderViews <= EE_MAX_CULLING_VIEWS );
        EE_ASSERT( activeRenderViewList.m_mainRenderViewActiveIndex < numActiveRenderViews );

        //-------------------------------------------------------------------------

        for ( uint32_t slotIndex = numActiveRenderViews; slotIndex < EE_MAX_CULLING_VIEWS; ++slotIndex )
        {
            pDeviceRenderViewIndices[slotIndex] = ActiveRenderViewList::s_InvalidActiveRenderViewIndex;
        }

        activeRenderViewList.m_numActiveRenderViews = numActiveRenderViews;
    }
}

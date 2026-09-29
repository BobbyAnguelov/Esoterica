#pragma once

#include "Base/Esoterica.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Render/RHI.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/StringID.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class DeviceRenderWorld;
    class RenderSystem;

    //-------------------------------------------------------------------------

    struct ActiveRenderViewMaterialShaderBucket final
    {
        void Initialize( RHI::Context* pContextRHI );
        void Shutdown( RenderSystem* pRenderSystem );

        //-------------------------------------------------------------------------

        StringID                                                            m_bucketName;

        RHI::Buffer*                                                        m_pDrawCounterBuffer = nullptr;
        DeviceResizeBuffer                                                  m_drawArgumentBuffer = {};
    };

    //-------------------------------------------------------------------------

    struct ActiveRenderViewBucket final
    {
        void Initialize( RHI::Context* pContextRHI );
        void Shutdown( RenderSystem* pRenderSystem );

        template<typename F>
        inline void ForEachRenderBucket( F fn )
        {
            fn( m_opaqueBucket );
            fn( m_alphaTestBucket );
            fn( m_alphaBlendBucket );
        }

        template<typename F>
        inline void ForEachRenderBucket( F fn ) const
        {
            fn( m_opaqueBucket );
            fn( m_alphaTestBucket );
            fn( m_alphaBlendBucket );
        }

        //-------------------------------------------------------------------------

        ActiveRenderViewMaterialShaderBucket                                m_opaqueBucket;
        ActiveRenderViewMaterialShaderBucket                                m_alphaTestBucket;
        ActiveRenderViewMaterialShaderBucket                                m_alphaBlendBucket; // TODO: Some render views don't care about alpha blend buckets, need to find a way to not allocate them when not needed
    };

    //-------------------------------------------------------------------------

    struct ActiveRenderView final
    {
        template<typename F>
        inline void ForEachRenderBucket( F fn )
        {
            for ( ActiveRenderViewBucket& bucket : m_renderViewBuckets )
            {
                bucket.ForEachRenderBucket( fn );
            }
        }

        template<typename F>
        inline void ForEachRenderBucket( F fn ) const
        {
            for ( ActiveRenderViewBucket const& bucket : m_renderViewBuckets )
            {
                bucket.ForEachRenderBucket( fn );
            }
        }

        void Initialize( RHI::Context* pContextRHI, size_t numMaterialShaderPipelineBuckets );
        void Shutdown( RenderSystem* pRenderSystem );

        void UpdateDeviceResources( RenderSystem* pRenderSystem, DeviceRenderWorld const& deviceRenderWorld );

        //-------------------------------------------------------------------------

        // TODO: This is ugly, need to keep in sync with the actual amount of buckets.
        // Must match the sub-bucket count used by GetBucketTypeForView() in RendererSurfaceShaderLayout.esh!
        static constexpr size_t                                             s_NumRenderBucketsPerViewBucket = 3;

        //-------------------------------------------------------------------------

        TVector<ActiveRenderViewBucket>                                     m_renderViewBuckets;
    };

    //-------------------------------------------------------------------------

    struct ActiveRenderViewList final
    {
        inline uint16_t GetDeviceRenderViewIndex( uint32_t activeRenderViewIndex ) const
        {
            EE_ASSERT( activeRenderViewIndex < m_numActiveRenderViews );
            return m_deviceRenderViewIndicesPerActiveRenderView[activeRenderViewIndex];
        }

        inline ActiveRenderView const& GetActiveRenderView( uint32_t activeRenderViewIndex ) const
        {
            EE_ASSERT( activeRenderViewIndex < m_numActiveRenderViews );
            return m_activeRenderViews[activeRenderViewIndex];
        }

        inline uint32_t FindActiveRenderViewIndex( uint32_t deviceRenderViewIndex ) const
        {
            if ( deviceRenderViewIndex >= m_activeRenderViewIndicesByDeviceRenderView.size() )
            {
                return s_InvalidActiveRenderViewIndex;
            }

            return m_activeRenderViewIndicesByDeviceRenderView[deviceRenderViewIndex];
        }

        inline bool IsActive( uint32_t deviceRenderViewIndex ) const
        {
            return FindActiveRenderViewIndex( deviceRenderViewIndex ) != s_InvalidActiveRenderViewIndex;
        }

        //-------------------------------------------------------------------------

        static constexpr uint16_t                                           s_InvalidActiveRenderViewIndex = 0xFFFF;

        //-------------------------------------------------------------------------

        uint32_t                                                            m_numRenderViewBucketsPerView = 0;
        uint32_t                                                            m_numRenderBuckets = 0;

        //-------------------------------------------------------------------------

        uint32_t                                                            m_mainRenderViewActiveIndex = 0;
        uint32_t                                                            m_editorOutlineRenderViewActiveIndex = 0;

        //-------------------------------------------------------------------------

        TArrayView<ActiveRenderView const>                                  m_activeRenderViews = {};

        uint16_t                                                            m_deviceRenderViewIndicesPerActiveRenderView[EE_MAX_CULLING_VIEWS] = {};

        TVector<uint16_t>                                                   m_activeRenderViewIndicesByDeviceRenderView;
        uint32_t                                                            m_numActiveRenderViews = 0;
    };

    //-------------------------------------------------------------------------

    struct ActiveRenderViewSelection final
    {
    public:

        void SelectActiveRenderViews
        (
            DeviceRenderWorld const&    deviceRenderWorld,
            ActiveRenderViewList&       activeRenderViewList,
            uint32_t                    mainRenderViewIndex,
            uint32_t                    editorOutlineRenderViewIndex
        );

    private:

        // Device render view index the next rotation starts from. Always the first view of a group, because
        // the rotation only ever enters an allocation at its first view.
        uint32_t                                                            m_rotationStartViewIndex = 0;
    };
}

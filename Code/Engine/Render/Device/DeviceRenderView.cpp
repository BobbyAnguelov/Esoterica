
#include "DeviceRenderView.h"
#include "Engine/Render/RenderSystem.h"

#include "Base/Render/RHI.h"

namespace EE::Render
{
    void DeviceRenderView::Initialize( RenderSystem* pRenderSystem )
    {
        EE_ASSERT( m_deviceRenderViewType == DeviceRenderViewType::Invalid );
        EE_ASSERT( m_colorTexture == nullptr );
        EE_ASSERT( m_depthTexture == nullptr );
        EE_ASSERT( m_pEnvironmentMapRadianceTexture == nullptr );
        EE_ASSERT( m_pSHCoefficientsBuffer == nullptr );
    }

    void DeviceRenderView::Shutdown( RenderSystem* pRenderSystem )
    {
        pRenderSystem->QueueResourceDelete
        (
            eastl::move( m_colorTexture ),
            eastl::move( m_depthTexture ),
            eastl::move( m_pEnvironmentMapRadianceTexture ),
            eastl::move( m_pSHCoefficientsBuffer )
        );

        m_deviceRenderViewType = DeviceRenderViewType::Invalid;
    }
}

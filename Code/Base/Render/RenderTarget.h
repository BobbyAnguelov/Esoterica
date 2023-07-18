#pragma once

#include "Base/_Module/API.h"
#include "RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct PickingID
    {
        PickingID() = default;
        inline bool IsSet() const { return m_0 != 0 || m_1 != 0; }

        uint64_t m_0 = 0;
        uint64_t m_1 = 0;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API RenderTarget
    {
        friend class RenderDevice;

    public:

        RenderTarget() = default;
        ~RenderTarget() { EE_ASSERT( !m_RT.IsValid() ); }

        inline bool IsValid() const { return m_RT.IsValid(); }
        inline Int2 const& GetDimensions() const { EE_ASSERT( IsValid() ); return m_RT.GetDimensions(); }
        inline bool HasDepthStencil() const { return m_DS.IsValid(); }
        inline bool HasPickingRT() const { return m_pickingRT.IsValid(); }

        inline bool operator==( RenderTarget const& rhs ) const { return m_RT == rhs.m_RT && m_DS == rhs.m_DS; }
        inline bool operator!=( RenderTarget const& rhs ) const { return m_RT != rhs.m_RT && m_DS != rhs.m_DS; }

        inline ViewSRVHandle const& GetRenderTargetShaderResourceView() const { EE_ASSERT( m_RT.IsValid() ); return m_RT.GetShaderResourceView(); }
        inline ViewRTHandle const& GetRenderTargetHandle() const { EE_ASSERT( m_RT.IsValid() ); return m_RT.GetRenderTargetView(); }
        inline ViewRTHandle const& GetPickingRenderTargetHandle() const { EE_ASSERT( m_pickingRT.IsValid() ); return m_pickingRT.GetRenderTargetView(); }
        inline ViewDSHandle const& GetDepthStencilHandle() const { EE_ASSERT( m_DS.IsValid() ); return m_DS.GetDepthStencilView(); }

    protected:

        Texture                     m_RT;
        Texture                     m_DS;
        Texture                     m_pickingRT;
        Texture                     m_pickingStagingTexture;
    };
}
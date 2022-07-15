#pragma once
#include "RenderTarget.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_SYSTEM_API RenderWindow
    {
        friend class RenderDevice;
        friend class RenderContext;

    public:

        RenderWindow() = default;
        ~RenderWindow() 
        { 
            EE_ASSERT( m_pSwapChain == nullptr );
            EE_ASSERT( !m_renderTarget.IsValid() );
        }

        RenderTarget const* GetRenderTarget() const { return &m_renderTarget; }
        inline bool IsValid() const { return m_pSwapChain != nullptr; }

    protected:

        void*               m_pSwapChain = nullptr;
        RenderTarget        m_renderTarget;
    };
}
#pragma once

#include "IRenderer.h"
#include "System/Types/Arrays.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API RendererRegistry : public ISystem
    {
    public:

        EE_SYSTEM( RendererRegistry );

    public:

        RendererRegistry() {}
        ~RendererRegistry();

        void RegisterRenderer( IRenderer* pRenderer );
        void UnregisterRenderer( IRenderer* pRenderer );

        template<typename T>
        inline T* GetRenderer() const
        {
            for ( auto pRenderer : m_registeredRenderers )
            {
                if ( pRenderer->GetRendererID() == T::RendererID )
                {
                    return reinterpret_cast<T*>( pRenderer );
                }
            }

            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        inline TInlineVector<IRenderer*, 20>& GetRegisteredRenderers() { return m_registeredRenderers; }

    private:

        TInlineVector<IRenderer*, 20>     m_registeredRenderers;
    };
}
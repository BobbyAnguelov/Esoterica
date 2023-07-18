#pragma once

#include "Engine/Render/RendererRegistry.h"
#include "Base/Render/RenderViewport.h"
#include "Base/Systems.h"

//-------------------------------------------------------------------------
// EE Renderer System
//-------------------------------------------------------------------------
// This class allows us to define the exact rendering order and task scheduling for a frame

namespace EE
{
    class UpdateContext;
    class EntityWorldManager;

    //-------------------------------------------------------------------------

    namespace Render
    {
        class RenderDevice;
        class WorldRenderer;
        class DebugRenderer;
        class ImguiRenderer;
        class RenderTarget;

        //-------------------------------------------------------------------------

        class RenderingSystem : public ISystem
        {
            struct ViewportRenderTarget
            {
                ViewportRenderTarget( UUID const& viewportID, RenderTarget* pRT )
                    : m_viewportID( viewportID )
                    , m_pRenderTarget( pRT )
                {
                    EE_ASSERT( pRT != nullptr );
                }

                UUID                    m_viewportID;
                RenderTarget*           m_pRenderTarget = nullptr;
            };

        public:

            EE_SYSTEM( RenderingSystem );

        public:

            void Initialize( RenderDevice* pRenderDevice, Float2 primaryWindowDimensions, RendererRegistry* pRegistry, EntityWorldManager* pWorldManager );
            void Shutdown();

            void ResizePrimaryRenderTarget( Int2 newMainWindowDimensions );
            void Update( UpdateContext const& ctx );

            //-------------------------------------------------------------------------

            void CreateCustomRenderTargetForViewport( Viewport const* pViewport, bool requiresPickingBuffer = false );
            void DestroyCustomRenderTargetForViewport( Viewport const* pViewport );

            ViewSRVHandle const& GetRenderTargetTextureForViewport( Viewport const* pViewport ) const;
            Render::PickingID GetViewportPickingID( Viewport const* pViewport, Int2 const& pixelCoords ) const;

        private:

            ViewportRenderTarget* FindRenderTargetForViewport( Viewport const* pViewport );
            inline ViewportRenderTarget const* FindRenderTargetForViewport( Viewport const* pViewport ) const { return const_cast<RenderingSystem*>( this )->FindRenderTargetForViewport( pViewport ); }

        private:

            RenderDevice*                                   m_pRenderDevice = nullptr;
            EntityWorldManager*                             m_pWorldManager = nullptr;
            WorldRenderer*                                  m_pWorldRenderer = nullptr;
            TVector<IRenderer*>                             m_customRenderers;

            TInlineVector<ViewportRenderTarget, 5>          m_viewportRenderTargets;

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            DebugRenderer*                                  m_pDebugRenderer = nullptr;
            ImguiRenderer*                                  m_pImguiRenderer = nullptr;
            Viewport                                        m_toolsViewport;
            #endif
        };
    }
}
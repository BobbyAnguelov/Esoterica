#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/Renderers/DebugRenderStates.h"
#include "Engine/Render/IRenderer.h"
#include "Base/Render/RenderDevice.h"

//-------------------------------------------------------------------------

namespace physx
{
    struct PxDebugPoint;
    struct PxDebugLine;
    struct PxDebugTriangle;
    struct PxDebugText;
}

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    class PhysicsSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsRenderer final : public Render::IRenderer
    {
        EE_RENDERER_ID( PhysicsRenderer, Render::RendererPriorityLevel::Debug );

    public:

        bool WasInitialized() const { return m_initialized; }
        bool Initialize( Render::RenderDevice* pRenderDevice );
        void Shutdown();
        void RenderWorld( Seconds const deltaTime, Render::Viewport const& viewport, Render::RenderTarget const& renderTarget, EntityWorld* pWorld ) override final;

    private:

        void DrawPoints( Render::RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugPoint const* pPoints, uint32_t numPoints );
        void DrawLines( Render::RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugLine const* pLines, uint32_t numLines );
        void DrawTriangles( Render::RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugTriangle const* pTriangles, uint32_t numTriangles );
        void DrawStrings( Render::RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugText const* pStrings, uint32_t numStrings );

    private:

        Render::RenderDevice*                       m_pRenderDevice = nullptr;

        Render::DebugLineRenderState                m_lineRS;
        Render::DebugPointRenderState               m_pointRS;
        Render::DebugPrimitiveRenderState           m_primitiveRS;
        Render::DebugTextRenderState                m_textRS;

        bool                                        m_initialized = false;
    };
}
#endif
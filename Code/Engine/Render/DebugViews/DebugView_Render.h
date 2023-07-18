#pragma once

#include "Engine/_Module/API.h"
#include "Engine/DebugViews/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class RendererWorldSystem;
    class LightComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderDebugView : public DebugView
    {
        EE_REFLECT_TYPE( RenderDebugView );

    public:

        static void DrawRenderVisualizationModesMenu( EntityWorld const* pWorld );

    public:

        RenderDebugView() : DebugView( "Engine/Render" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

    private:

        RendererWorldSystem*            m_pWorldRendererSystem = nullptr;
    };
}
#endif
#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class RendererWorldSystem;
    class LightComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderDebugView : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( RenderDebugView );

    public:

        static void DrawRenderVisualizationModesMenu( EntityWorld const* pWorld );

    public:

        RenderDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;
        virtual void DrawOverlayElements( EntityWorldUpdateContext const& context ) override;

        void DrawRenderMenu( EntityWorldUpdateContext const& context );

    private:

        RendererWorldSystem*            m_pWorldRendererSystem = nullptr;
    };
}
#endif
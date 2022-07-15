#include "DebugView_Render.h"
#include "Engine/Render/Systems/WorldSystem_Renderer.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    void RenderDebugView::DrawRenderVisualizationModesMenu( EntityWorld const* pWorld )
    {
        EE_ASSERT( pWorld != nullptr );
        auto pWorldRendererSystem = pWorld->GetWorldSystem<Render::RendererWorldSystem>();

        int32_t debugMode = (int32_t) pWorldRendererSystem->GetVisualizationMode();

        bool stateUpdated = false;
        stateUpdated |= ImGui::RadioButton( "Render Lighting", &debugMode, (int32_t) RendererWorldSystem::VisualizationMode::Lighting );
        stateUpdated |= ImGui::RadioButton( "Render Albedo", &debugMode, (int32_t) RendererWorldSystem::VisualizationMode::Albedo );
        stateUpdated |= ImGui::RadioButton( "Render Normals", &debugMode, (int32_t) RendererWorldSystem::VisualizationMode::Normals );
        stateUpdated |= ImGui::RadioButton( "Render Metalness", &debugMode, (int32_t) RendererWorldSystem::VisualizationMode::Metalness );
        stateUpdated |= ImGui::RadioButton( "Render Roughness", &debugMode, (int32_t) RendererWorldSystem::VisualizationMode::Roughness );
        stateUpdated |= ImGui::RadioButton( "Render Ambient Occlusion", &debugMode, (int32_t) RendererWorldSystem::VisualizationMode::AmbientOcclusion );

        if ( stateUpdated )
        {
            pWorldRendererSystem->SetVisualizationMode( (RendererWorldSystem::VisualizationMode) debugMode );
        }
    }

    //-------------------------------------------------------------------------

    RenderDebugView::RenderDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Render", [this] ( EntityWorldUpdateContext const& context ) { DrawRenderMenu( context ); } ) );
    }

    void RenderDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        EntityWorldDebugView::Initialize( systemRegistry, pWorld );
        m_pWorldRendererSystem = pWorld->GetWorldSystem<RendererWorldSystem>();
    }

    void RenderDebugView::Shutdown()
    {
        m_pWorldRendererSystem = nullptr;
        EntityWorldDebugView::Shutdown();
    }

    void RenderDebugView::DrawRenderMenu( EntityWorldUpdateContext const& context )
    {
        DrawRenderVisualizationModesMenu( m_pWorld );
    }

    void RenderDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
    }

    void RenderDebugView::DrawOverlayElements( EntityWorldUpdateContext const& context )
    {
    }
}
#endif
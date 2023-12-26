#include "DebugView_Render.h"
#include "Engine/Render/Systems/WorldSystem_Renderer.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Imgui/ImguiX.h"

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

    void RenderDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pWorldRendererSystem = pWorld->GetWorldSystem<RendererWorldSystem>();
    }

    void RenderDebugView::Shutdown()
    {
        m_pWorldRendererSystem = nullptr;
        DebugView::Shutdown();
    }

    void RenderDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        ImGui::SeparatorText( "Visualization" );

        DrawRenderVisualizationModesMenu( m_pWorld );

        ImGui::SeparatorText( "Static Meshes" );

        ImGui::Checkbox( "Show Static Mesh Bounds", &m_pWorldRendererSystem->m_showStaticMeshBounds );

        ImGui::SeparatorText( "Skeletal Meshes" );

        ImGui::Checkbox( "Show Skeletal Mesh Bounds", &m_pWorldRendererSystem->m_showSkeletalMeshBounds );
        ImGui::Checkbox( "Show Skeletal Mesh Bones", &m_pWorldRendererSystem->m_showSkeletalMeshBones );
        ImGui::Checkbox( "Show Skeletal Bind Poses", &m_pWorldRendererSystem->m_showSkeletalMeshBindPoses );
    }
}
#endif
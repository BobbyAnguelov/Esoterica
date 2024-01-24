#include "DebugView_Render.h"
#include "Engine/Render/Systems/WorldSystem_Renderer.h"
#include "Engine/Render/Settings/WorldSettings_Render.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    void RenderDebugView::DrawRenderVisualizationModesMenu( EntityWorld const* pWorld )
    {
        EE_ASSERT( pWorld != nullptr );

        auto* pRenderSettings = pWorld->GetMutableSettings<Render::RenderWorldSettings>();

        int32_t debugMode = (int32_t) pRenderSettings->m_visualizationMode;

        bool stateUpdated = false;
        stateUpdated |= ImGui::RadioButton( "Render Lighting", &debugMode, (int32_t) DebugVisualizationMode::Lighting );
        stateUpdated |= ImGui::RadioButton( "Render Albedo", &debugMode, (int32_t) DebugVisualizationMode::Albedo );
        stateUpdated |= ImGui::RadioButton( "Render Normals", &debugMode, (int32_t) DebugVisualizationMode::Normals );
        stateUpdated |= ImGui::RadioButton( "Render Metalness", &debugMode, (int32_t) DebugVisualizationMode::Metalness );
        stateUpdated |= ImGui::RadioButton( "Render Roughness", &debugMode, (int32_t) DebugVisualizationMode::Roughness );
        stateUpdated |= ImGui::RadioButton( "Render Ambient Occlusion", &debugMode, (int32_t) DebugVisualizationMode::AmbientOcclusion );

        if ( stateUpdated )
        {
            pRenderSettings->m_visualizationMode = (DebugVisualizationMode) debugMode;
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
        auto* pRenderSettings = m_pWorld->GetMutableSettings<Render::RenderWorldSettings>();

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Visualization" );

        DrawRenderVisualizationModesMenu( m_pWorld );

        ImGui::SeparatorText( "Static Meshes" );

        ImGui::Checkbox( "Show Static Mesh Bounds", &pRenderSettings->m_showStaticMeshBounds );

        ImGui::SeparatorText( "Skeletal Meshes" );

        ImGui::Checkbox( "Show Skeletal Mesh Bounds", &pRenderSettings->m_showSkeletalMeshBounds );
        ImGui::Checkbox( "Show Skeletal Mesh Bones", &pRenderSettings->m_showSkeletalMeshBones );
        ImGui::Checkbox( "Show Skeletal Bind Poses", &pRenderSettings->m_showSkeletalMeshBindPoses );
    }
}
#endif
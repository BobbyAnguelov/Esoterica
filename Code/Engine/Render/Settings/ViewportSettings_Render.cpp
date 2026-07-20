#include "ViewportSettings_Render.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    void RenderViewportSettings::DrawMenu( EntityWorld* pWorld )
    {
        ImGui::SeparatorText( "Visualization Modes" );

        int32_t debugMode = (int32_t) m_visualizationMode;

        bool stateUpdated = false;
        stateUpdated |= ImGui::RadioButton( "Render Final", &debugMode, int32_t( DebugVisualizationMode::None ) );
        stateUpdated |= ImGui::RadioButton( "Render Direct Light", &debugMode, int32_t( DebugVisualizationMode::DirectLight ) );
        stateUpdated |= ImGui::RadioButton( "Render Environment Map Light", &debugMode, int32_t( DebugVisualizationMode::EnvironmentMapLight ) );
        stateUpdated |= ImGui::RadioButton( "Render Shadows", &debugMode, int32_t( DebugVisualizationMode::Shadows ) );
        stateUpdated |= ImGui::RadioButton( "Render Albedo", &debugMode, int32_t( DebugVisualizationMode::Albedo ) );
        stateUpdated |= ImGui::RadioButton( "Render Normals", &debugMode, int32_t( DebugVisualizationMode::Normals ) );
        stateUpdated |= ImGui::RadioButton( "Render Metalness", &debugMode, int32_t( DebugVisualizationMode::Metalness ) );
        stateUpdated |= ImGui::RadioButton( "Render Roughness", &debugMode, int32_t( DebugVisualizationMode::Roughness ) );
        stateUpdated |= ImGui::RadioButton( "Render Ambient Occlusion", &debugMode, int32_t( DebugVisualizationMode::AmbientOcclusion ) );
        stateUpdated |= ImGui::RadioButton( "Render Light Complexity", &debugMode, int32_t( DebugVisualizationMode::LightComplexity ) );
        stateUpdated |= ImGui::RadioButton( "Render SSAO", &debugMode, int32_t( DebugVisualizationMode::SSAO ) );

        if ( stateUpdated )
        {
            m_visualizationMode = (DebugVisualizationMode) debugMode;
        }

        //-------------------------------------------------------------------------

        ImGui::Checkbox( "Show Wireframe", &m_showWireframe );
        ImGui::Checkbox( "Show Shadow Cascades", &m_showShadowCascades );

        ImGui::SeparatorText( "Static Meshes" );

        ImGui::Checkbox( "Show Static Mesh Bounds", &m_showStaticMeshBounds );

        ImGui::SeparatorText( "Skeletal Meshes" );

        ImGui::Checkbox( "Show Skeletal Mesh Bounds", &m_showSkeletalMeshBounds );
        ImGui::Checkbox( "Show Skeletal Mesh Bones", &m_showSkeletalMeshBones );
        ImGui::Checkbox( "Show Skeletal Bind Poses", &m_showSkeletalMeshBindPoses );
    }
}
#endif

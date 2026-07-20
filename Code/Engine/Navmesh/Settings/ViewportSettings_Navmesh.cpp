#include "ViewportSettings_Navmesh.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Navmesh
{
    void NavmeshViewportSettings::DrawMenu( EntityWorld* pWorld )
    {
        auto pNavmeshWorldSystem = pWorld->GetWorldSystem<NavmeshWorldSystem>();
        if ( pNavmeshWorldSystem == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Viewport
        //-------------------------------------------------------------------------

        ImGuiX::Checkbox( "Enable Navmesh Debug", &m_drawDebug );

        //-------------------------------------------------------------------------
        // World
        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        auto CreatePlannerCheckboxForFlag = [pNavmeshWorldSystem] ( char const* pLabel, bfx::PlannerDebugFlag flag )
        {
            bool isEnabled = bfx::GetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, flag );
            if ( ImGuiX::Checkbox( pLabel, &isEnabled ) )
            {
                bfx::SetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, flag, isEnabled );
            }
        };

        //-------------------------------------------------------------------------

        bool isDrawingNavpowerStats = bfx::GetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, bfx::BFX_DRAW_STATS );
        if ( ImGuiX::Checkbox( "Draw NavPower Stats", &isDrawingNavpowerStats ) )
        {
            bfx::SetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, bfx::BFX_DRAW_STATS, isDrawingNavpowerStats );
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( "Areas" ) )
        {
            CreatePlannerCheckboxForFlag( "Draw Area Edges", bfx::BFX_DRAW_PLANNER_AREAS );
            CreatePlannerCheckboxForFlag( "Draw Area Normals", bfx::BFX_DRAW_AREA_NORMALS );
            CreatePlannerCheckboxForFlag( "Draw Areas", bfx::BFX_DRAW_PLANNER_AREAS_SOLID );
            CreatePlannerCheckboxForFlag( "Draw Area Connectivity", bfx::BFX_DRAW_PLANNER_CONNECTIVITY );

            CreatePlannerCheckboxForFlag( "Draw Area Usage Flags", bfx::BFX_DRAW_AREA_USAGE_FLAGS );
            CreatePlannerCheckboxForFlag( "Draw Area Penalties", bfx::BFX_DRAW_AREA_PENALTY_MULTS );
            CreatePlannerCheckboxForFlag( "Draw Area Sharing Penalties", bfx::BFX_DRAW_SHARING_PENALTY_COUNTS );

            CreatePlannerCheckboxForFlag( "Colorize Areas Usage Flags", bfx::BFX_COLORIZE_AREA_USAGE_FLAGS );

            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( "Obstacles" ) )
        {
            CreatePlannerCheckboxForFlag( "Draw Obstacles", bfx::BFX_DRAW_OBSTACLES );
            CreatePlannerCheckboxForFlag( "Draw Obstacle Flags", bfx::BFX_DRAW_OBSTACLE_FLAGS );

            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( "Links" ) )
        {
            CreatePlannerCheckboxForFlag( "Draw line usage distances", bfx::BFX_DRAW_LINK_USAGE_DISTANCES );
            CreatePlannerCheckboxForFlag( "Draw link usage flags", bfx::BFX_DRAW_LINK_USAGE_FLAGS );
            CreatePlannerCheckboxForFlag( "Draw link user data", bfx::BFX_DRAW_LINK_USER_DATA );
            CreatePlannerCheckboxForFlag( "Draw original link locations", bfx::BFX_DRAW_ORIGINAL_LINK_LOCATIONS );
            CreatePlannerCheckboxForFlag( "Disable all link drawing", bfx::BFX_DISABLE_ALL_LINK_DRAWING );

            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( "Queries" ) )
        {
            CreatePlannerCheckboxForFlag( "Draw Probes", bfx::BFX_DRAW_RECENT_NAVPROBES );
            CreatePlannerCheckboxForFlag( "Draw Box Fit Queries", bfx::BFX_DRAW_RECENT_CHECKBOXFIT );
            CreatePlannerCheckboxForFlag( "Draw Polyline Paths", bfx::BFX_DRAW_RECENT_CREATEPOLYLINEPATHS );
            CreatePlannerCheckboxForFlag( "Draw Spline Paths", bfx::BFX_DRAW_RECENT_CREATESPLINEPATHS );

            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu( "Drawing Options" ) )
        {
            bool isDepthTestEnabled = !pNavmeshWorldSystem->IsDebugRendererDepthTestEnabled();
            if ( ImGuiX::Checkbox( "Enable Depth Test", &isDepthTestEnabled ) )
            {
                pNavmeshWorldSystem->SetDebugRendererDepthTestState( isDepthTestEnabled );
            }

            ImGui::EndMenu();
        }
        #endif
    }

    void NavmeshViewportSettings::ShowAreas( EntityWorld* pWorld )
    {
        auto pNavmeshWorldSystem = pWorld->GetWorldSystem<NavmeshWorldSystem>();
        if ( pNavmeshWorldSystem == nullptr )
        {
            return;
        }

        m_drawDebug = true;

        #if EE_ENABLE_NAVPOWER
        bfx::SetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, bfx::BFX_DRAW_PLANNER_AREAS, true );
        #endif
    }
}
#endif
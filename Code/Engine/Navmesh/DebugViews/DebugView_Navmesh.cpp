#include "DebugView_Navmesh.h"
#include "Engine/Navmesh/Systems/WorldSystem_Navmesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Navmesh
{
    void NavmeshDebugView::DrawNavmeshRuntimeSettings( NavmeshWorldSystem* pNavmeshWorldSystem )
    {
        #if EE_ENABLE_NAVPOWER
        auto CreatePlannerCheckboxForFlag = [pNavmeshWorldSystem] ( char const* pLabel, bfx::PlannerDebugFlag flag )
        {
            bool isEnabled = bfx::GetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, flag );
            if ( ImGui::Checkbox( pLabel, &isEnabled ) )
            {
                bfx::SetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, flag, isEnabled );
            }
        };

        //-------------------------------------------------------------------------

        bool isDrawingNavpowerStats = bfx::GetGlobalDebugFlag( pNavmeshWorldSystem->m_pInstance, bfx::BFX_DRAW_STATS );
        if ( ImGui::MenuItem( "Draw NavPower Stats", nullptr, &isDrawingNavpowerStats ) )
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
            bool isDepthTestEnabled = !pNavmeshWorldSystem->m_renderer.IsDepthTestEnabled();
            if ( ImGui::Checkbox( "Enable Depth Test", &isDepthTestEnabled ) )
            {
                pNavmeshWorldSystem->m_renderer.SetDepthTestState( isDepthTestEnabled );
            }

            ImGui::EndMenu();
        }
        #endif
    }

    //-------------------------------------------------------------------------

    NavmeshDebugView::NavmeshDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Navmesh", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void NavmeshDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pNavmeshWorldSystem = pWorld->GetWorldSystem<NavmeshWorldSystem>();
    }

    void NavmeshDebugView::Shutdown()
    {
        m_pNavmeshWorldSystem = nullptr;
    }

    //-------------------------------------------------------------------------

    void NavmeshDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        DrawNavmeshRuntimeSettings( m_pNavmeshWorldSystem );
    }

    void NavmeshDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {}
}
#endif
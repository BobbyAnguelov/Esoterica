#include "DebugView_Physics.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    bool PhysicsDebugView::DrawMaterialDatabaseView( UpdateContext const& context )
    {
        auto pMaterialRegistry = context.GetSystem<MaterialRegistry>();

        //-------------------------------------------------------------------------

        bool isMaterialDatabaseWindowOpen = true;

        ImGui::SetNextWindowBgAlpha( 0.75f );
        if ( ImGui::Begin( "Physics Material Database", &isMaterialDatabaseWindowOpen ) )
        {
            if ( ImGui::BeginTable( "Resource Request History Table", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableSetupColumn( "Material ID", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Static Friction", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
                ImGui::TableSetupColumn( "Dynamic Friction", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
                ImGui::TableSetupColumn( "Restitution", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
                ImGui::TableSetupColumn( "Friction Combine", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 128 );
                ImGui::TableSetupColumn( "Restitution Combine", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 128 );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                for ( auto const& materialPair : pMaterialRegistry->m_materials )
                {
                    physx::PxMaterial const* pMaterial = materialPair.second.m_pMaterial;

                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    static char const* const combineText[] = { "Average", "Min", "Multiply", "Max" };
                    auto GetCombineText = [] ( physx::PxCombineMode::Enum combineMode )
                    {
                        return combineText[(uint8_t) combineMode];
                    };

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( materialPair.second.m_ID.c_str() );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%.3f", pMaterial->getStaticFriction() );

                    ImGui::TableSetColumnIndex( 2 );
                    ImGui::Text( "%.3f", pMaterial->getDynamicFriction() );

                    ImGui::TableSetColumnIndex( 3 );
                    ImGui::Text( "%.3f", pMaterial->getRestitution() );

                    ImGui::TableSetColumnIndex( 4 );
                    ImGui::Text( GetCombineText( pMaterial->getFrictionCombineMode() ) );

                    ImGui::TableSetColumnIndex( 5 );
                    ImGui::Text( GetCombineText( pMaterial->getRestitutionCombineMode() ) );
                }

                //-------------------------------------------------------------------------

                ImGui::EndTable();
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        return isMaterialDatabaseWindowOpen;
    }

    //-------------------------------------------------------------------------

    PhysicsDebugView::PhysicsDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Engine/Physics", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void PhysicsDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pPhysicsWorldSystem = pWorld->GetWorldSystem<PhysicsWorldSystem>();
    }

    void PhysicsDebugView::Shutdown()
    {
        m_pPhysicsWorldSystem = nullptr;
    }

    //-------------------------------------------------------------------------

    void PhysicsDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        if ( m_isMaterialDatabaseWindowOpen )
        {
            if ( pWindowClass != nullptr )
            {
                ImGui::SetNextWindowClass( pWindowClass );
            }

            if ( m_isMaterialDatabaseWindowOpen )
            {
                m_isMaterialDatabaseWindowOpen = DrawMaterialDatabaseView( context );
            }
        }
    }

    void PhysicsDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        //-------------------------------------------------------------------------
        // Scene
        //-------------------------------------------------------------------------

        auto pWorld = m_pPhysicsWorldSystem->GetWorld();

        uint32_t debugFlags = pWorld->GetDebugFlags();
        float drawDistance = pWorld->GetDebugDrawDistance();

        bool stateUpdated = false;

        stateUpdated |= ImGui::CheckboxFlags( "Enable Debug Visualization", &debugFlags, 1 << physx::PxVisualizationParameter::eSCALE );
        stateUpdated |= ImGui::SliderFloat( "Visualization Distance", &drawDistance, 1.0f, 100.0f );

        ImGui::Separator();

        stateUpdated |= ImGui::CheckboxFlags( "Collision AABBs", &debugFlags, 1 << physx::PxVisualizationParameter::eCOLLISION_AABBS );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Shapes", &debugFlags, 1 << physx::PxVisualizationParameter::eCOLLISION_SHAPES );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Axes", &debugFlags, 1 << physx::PxVisualizationParameter::eCOLLISION_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Face Normals", &debugFlags, 1 << physx::PxVisualizationParameter::eCOLLISION_FNORMALS );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Edges", &debugFlags, 1 << physx::PxVisualizationParameter::eCOLLISION_EDGES );

        ImGui::Separator();

        stateUpdated |= ImGui::CheckboxFlags( "Contact Points", &debugFlags, 1 << physx::PxVisualizationParameter::eCONTACT_POINT );
        stateUpdated |= ImGui::CheckboxFlags( "Contact Normals", &debugFlags, 1 << physx::PxVisualizationParameter::eCONTACT_NORMAL );
        stateUpdated |= ImGui::CheckboxFlags( "Contact Error", &debugFlags, 1 << physx::PxVisualizationParameter::eCONTACT_ERROR );
        stateUpdated |= ImGui::CheckboxFlags( "Contact Force", &debugFlags, 1 << physx::PxVisualizationParameter::eCONTACT_FORCE );

        ImGui::Separator();

        stateUpdated |= ImGui::CheckboxFlags( "Actor Axes", &debugFlags, 1 << physx::PxVisualizationParameter::eACTOR_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Body Axes", &debugFlags, 1 << physx::PxVisualizationParameter::eBODY_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Linear Velocity", &debugFlags, 1 << physx::PxVisualizationParameter::eBODY_LIN_VELOCITY );
        stateUpdated |= ImGui::CheckboxFlags( "Angular Velocity", &debugFlags, 1 << physx::PxVisualizationParameter::eBODY_ANG_VELOCITY );
        stateUpdated |= ImGui::CheckboxFlags( "Mass Axes", &debugFlags, 1 << physx::PxVisualizationParameter::eBODY_MASS_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Joint Limits", &debugFlags, 1 << physx::PxVisualizationParameter::eJOINT_LIMITS );
        stateUpdated |= ImGui::CheckboxFlags( "Joint Local Frames", &debugFlags, 1 << physx::PxVisualizationParameter::eJOINT_LOCAL_FRAMES );

        if ( stateUpdated )
        {
            pWorld->SetDebugFlags( debugFlags );
            pWorld->SetDebugDrawDistance( drawDistance );
        }

        //-------------------------------------------------------------------------
        // Materials
        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( ImGui::Button( "Show Material Database", ImVec2( -1, 0 ) ) )
        {
            m_isMaterialDatabaseWindowOpen = true;
        }
    }
}
#endif
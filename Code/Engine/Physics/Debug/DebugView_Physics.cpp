#include "DebugView_Physics.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    void PhysicsDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pPhysicsWorldSystem = pWorld->GetWorldSystem<PhysicsWorldSystem>();
        m_windows.emplace_back( "Physics Material Database", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawMaterialDatabaseView( context ); } );
    }

    void PhysicsDebugView::Shutdown()
    {
        m_pPhysicsWorldSystem = nullptr;
        DebugView::Shutdown();
    }

    void PhysicsDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( m_pPhysicsWorldSystem->IsRecording() )
        {
            if ( ImGui::MenuItem( EE_ICON_STOP" Stop Recording" ) )
            {
                m_pPhysicsWorldSystem->StopRecording();
            }
        }
        else
        {
            if( ImGui::MenuItem( EE_ICON_RECORD" Start Recording" ) )
            {
                m_pPhysicsWorldSystem->StartRecording();
            }
        }

        if ( ImGui::MenuItem( "Show Material Database" ) )
        {
            m_windows[0].m_isOpen = true;
        }
    }

    void PhysicsDebugView::DrawMaterialDatabaseView( UpdateContext const& context )
    {
        auto pMaterialRegistry = context.GetSystem<MaterialRegistry>();

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "Material Table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
        {
            ImGui::TableSetupColumn( "Material ID", ImGuiTableColumnFlags_WidthStretch, 300 );
            ImGui::TableSetupColumn( "Friction", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
            ImGui::TableSetupColumn( "Restitution", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
            ImGui::TableSetupColumn( "Rolling Resistance", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 128 );
            ImGui::TableSetupColumn( "Tangent Velocity", ImGuiTableColumnFlags_WidthStretch, 128 );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            for ( auto const& materialPair : pMaterialRegistry->m_materials )
            {
                ImGui::TableNextRow();

                //-------------------------------------------------------------------------

                ImGui::TableSetColumnIndex( 0 );
                ImGui::Text( materialPair.first.c_str() );

                ImGui::TableSetColumnIndex( 1 );
                ImGui::Text( "%.3f", materialPair.second.friction );

                ImGui::TableSetColumnIndex( 2 );
                ImGui::Text( "%.3f", materialPair.second.restitution );

                ImGui::TableSetColumnIndex( 3 );
                ImGui::Text( "%.3f", materialPair.second.rollingResistance );

                ImGui::TableSetColumnIndex( 4 );
                ImGuiX::DrawFloat3( FromBox3D( materialPair.second.tangentVelocity ) );
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }
}
#endif
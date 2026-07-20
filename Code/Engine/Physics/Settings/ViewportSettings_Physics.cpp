#include "ViewportSettings_Physics.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    void PhysicsViewportSettings::DrawMenu( EntityWorld* pWorld )
    {
        auto pPhysicsWorldSystem = pWorld->GetWorldSystem<PhysicsWorldSystem>();
        if ( pPhysicsWorldSystem == nullptr )
        {
            return;
        }

        PhysicsWorld* pPhysicsWorld = pPhysicsWorldSystem->GetPhysicsWorld();
        if ( pPhysicsWorld == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Viewport
        //-------------------------------------------------------------------------

        ImGuiX::Checkbox( "Enable Physics Debug", &m_drawDebug );

        ImGui::Separator();

        ImGui::SliderFloat( "Force Scale", &m_forceScale, 0.0f, 5.0f );
        ImGui::SliderFloat( "Joint Scale", &m_jointScale, 0.0f, 5.0f );

        ImGui::Separator();

        ImGuiX::Checkbox( "Shapes", &m_drawShapes );
        ImGuiX::Checkbox( "Joints", &m_drawJoints );
        ImGuiX::Checkbox( "Joint Extras", &m_drawJointExtras );
        ImGuiX::Checkbox( "Bounds", &m_drawBounds );
        ImGuiX::Checkbox( "Mass", &m_drawMass );
        ImGuiX::Checkbox( "Sleep", &m_drawSleep );
        ImGuiX::Checkbox( "Body Names", &m_drawBodyNames );

        ImGui::Separator();

        ImGuiX::Checkbox( "Contacts", &m_drawContacts );
        ImGuiX::Checkbox( "Contact Draw Anchor A", &m_drawAnchorA );
        ImGuiX::Checkbox( "Contact Features", &m_drawContactFeatures );
        ImGuiX::Checkbox( "Contact Normals", &m_drawContactNormals );
        ImGuiX::Checkbox( "Contact Forces", &m_drawContactForces );

        ImGui::Separator();

        ImGuiX::Checkbox( "Graph Colors", &m_drawGraphColors );
        ImGuiX::Checkbox( "Islands", &m_drawIslands );
    }
}
#endif
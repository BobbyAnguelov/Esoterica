#include "DebugView_Camera.h"
#include "Base/Imgui/ImguiX.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Camera/Components/Component_OrbitCamera.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void CameraDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pCameraManager = pWorld->GetWorldSystem<CameraManager>();
        m_windows.emplace_back( "Camera Debug", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawCameraWindow( context ); } );
    }

    void CameraDebugView::Shutdown()
    {
        m_pCameraManager = nullptr;
        DebugView::Shutdown();
    }

    void CameraDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Camera" ) )
        {
            m_windows[0].m_isOpen = true;
        }
    }
   
    void CameraDebugView::DrawCameraWindow( EntityWorldUpdateContext const& context )
    {
        auto DrawCameraDetails = [] ( CameraComponent* pCamera )
        {
            auto pFreeLookCamera = TryCast<FreeLookCameraComponent>( pCamera );
            auto pOrbitCamera = TryCast<OrbitCameraComponent>( pCamera );

            if ( pFreeLookCamera != nullptr )
            {
                ImGui::Text( "Camera Type: Free Look" );
                ImGui::Text( "Pitch: %.2f deg", pFreeLookCamera->m_pitch.ToDegrees().ToFloat() );
                ImGui::Text( "Yaw: %.2f deg ", pFreeLookCamera->m_yaw.ToDegrees().ToFloat() );
            }
            else if ( pOrbitCamera != nullptr )
            {
                ImGui::Text( "Camera Type: Orbit" );
                ImGui::Text( "Orbit Offset: %s", Math::ToString( pOrbitCamera->m_orbitTargetOffset ).c_str() );
                ImGui::Text( "Orbit Distance: %.2fm", pOrbitCamera->m_orbitDistance );
            }

            auto const& camTransform = pCamera->GetWorldTransform();
            ImGui::Text( "Forward: %s", Math::ToString( camTransform.GetForwardVector() ).c_str() );
            ImGui::Text( "Right: %s", Math::ToString( camTransform.GetRightVector() ).c_str() );
            ImGui::Text( "Up: %s", Math::ToString( camTransform.GetUpVector() ).c_str() );
            ImGui::Text( "Pos: %s", Math::ToString( camTransform.GetTranslation() ).c_str() );

            if ( ImGui::Button( "Copy Camera Transform", ImVec2( -1, 0 ) ) )
            {
                auto const& camTranslation = camTransform.GetTranslation();
                auto const camRotation = camTransform.GetRotation().ToEulerAngles().GetAsDegrees();

                String const camTransformStr( String::CtorSprintf(), "%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, 1.0f, 1.0f, 1.0f", camRotation.m_x, camRotation.m_y, camRotation.m_z, camTranslation.GetX(), camTranslation.GetY(), camTranslation.GetZ() );
                ImGui::SetClipboardText( camTransformStr.c_str() );
            }
        };

        //-------------------------------------------------------------------------

        if ( m_pCameraManager->HasActiveCamera() )
        {
            ImGui::PushFont( ImGuiX::GetFont( ImGuiX::Font::Large ) );
            ImGui::Text( "Active Camera" );
            ImGui::Separator();
            ImGui::PopFont();

            CameraComponent* pCamera = m_pCameraManager->GetActiveCamera();
            DrawCameraDetails( pCamera );
        }

        //-------------------------------------------------------------------------

        if ( m_pCameraManager->IsDebugCameraEnabled() )
        {
            ImGui::NewLine();

            ImGui::PushFont( ImGuiX::GetFont( ImGuiX::Font::Large ) );
            ImGui::Text( "Debug Camera" );
            ImGui::Separator();
            ImGui::PopFont();
            DrawCameraDetails( m_pCameraManager->GetDebugCamera() );
        }
    }
}
#endif
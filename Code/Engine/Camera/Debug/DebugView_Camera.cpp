#include "DebugView_Camera.h"
#include "Base/Imgui/ImguiX.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Engine/Camera/Components/Component_Camera.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void CameraDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pCameraManager = pWorld->GetWorldSystem<CameraSystem>();
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
        if ( m_pCameraManager->HasActiveCamera() )
        {
            CameraComponent* pCamera = m_pCameraManager->GetActiveCamera();

            ImGui::Text( "Active Camera: %s", pCamera->GetNameID().c_str() );

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Transform" );

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

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Camera Details" );
            pCamera->DrawDebugUI();
        }
    }
}
#endif
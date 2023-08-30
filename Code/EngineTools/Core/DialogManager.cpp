#include "DialogManager.h"

//-------------------------------------------------------------------------

namespace EE
{
    DialogManager::~DialogManager()
    {
        if ( m_pActiveDialog != nullptr )
        {
            EE::Delete( m_pActiveDialog );
        }
    }

    bool DialogManager::DrawDialog( UpdateContext const& context )
    {
        bool hasOpenModalDialog = false;

        //-------------------------------------------------------------------------

        if ( m_pActiveDialog == nullptr )
        {
            return hasOpenModalDialog;
        }

        //-------------------------------------------------------------------------

        if ( !m_pActiveDialog->m_isOpen )
        {
            EE::Delete( m_pActiveDialog );
            return hasOpenModalDialog;
        }

        //-------------------------------------------------------------------------

        if ( m_pActiveDialog->HasWindowConstraints() )
        {
            ImGui::SetNextWindowSizeConstraints( m_pActiveDialog->m_windowConstraints[0], m_pActiveDialog->m_windowConstraints[1] );
        }

        if ( ImGuiX::BeginViewportPopupModal( m_pActiveDialog->m_title.c_str(), &m_pActiveDialog->m_isOpen, m_pActiveDialog->m_windowSize, m_pActiveDialog->m_windowSizeCond, m_pActiveDialog->m_windowFlags ) )
        {
            hasOpenModalDialog = true;

            m_pActiveDialog->m_isOpen = m_pActiveDialog->m_drawFunction( context );

            if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
            {
                m_pActiveDialog->m_isOpen = false;
            }

            if ( !m_pActiveDialog->m_isOpen )
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return hasOpenModalDialog;
    }

    void DialogManager::CreateModalDialog( String const& title, TFunction<bool( UpdateContext const& )> const& drawFunction, ImVec2 const& windowSize, bool isResizable )
    {
        EE_ASSERT( m_pActiveDialog == nullptr );
        m_pActiveDialog = EE::New<ModalDialog>();
        m_pActiveDialog->m_title = title;
        m_pActiveDialog->m_drawFunction = drawFunction;
        m_pActiveDialog->m_windowSize = windowSize;
        m_pActiveDialog->m_windowSizeCond = isResizable ? ImGuiCond_FirstUseEver : ImGuiCond_Always;
        m_pActiveDialog->m_windowFlags = isResizable ? ImGuiWindowFlags_NoSavedSettings : ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize;
        m_pActiveDialog->m_isOpen = true;
    }

    void DialogManager::SetActiveModalDialogSizeConstraints( ImVec2 const& min, ImVec2 const& max )
    {
        EE_ASSERT( min.x >= 0 && min.x <= max.x );
        EE_ASSERT( min.y >= 0 && min.y <= max.y );
        m_pActiveDialog->m_windowConstraints[0] = min;
        m_pActiveDialog->m_windowConstraints[1] = max;
    }
}
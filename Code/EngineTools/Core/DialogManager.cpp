#include "DialogManager.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE
{
    DialogManager::~DialogManager()
    {
        if ( m_pModalDialog != nullptr )
        {
            EE::Delete( m_pModalDialog );
        }
    }

    void DialogManager::StopDialog()
    {
        m_hasActiveDialog = false;

        if ( m_pModalDialog != nullptr )
        {
            EE::Delete( m_pModalDialog );
        }
    }

    bool DialogManager::DrawDialog()
    {
        bool hasOpenModalDialog = false;

        //-------------------------------------------------------------------------

        if ( !HasActiveModalDialog() )
        {
            return hasOpenModalDialog;
        }

        //-------------------------------------------------------------------------

        if ( !m_isOpen )
        {
            StopDialog();
            return hasOpenModalDialog;
        }

        //-------------------------------------------------------------------------

        if ( HasWindowConstraints() )
        {
            ImGui::SetNextWindowSizeConstraints( m_windowConstraints[0], m_windowConstraints[1] );
        }

        if ( ImGuiX::BeginViewportPopupModal( m_viewportID, m_title.c_str(), m_showCloseButton ? &m_isOpen : nullptr, m_windowSize, m_windowSizeCond, m_windowFlags ) )
        {
            hasOpenModalDialog = true;

            if ( m_pModalDialog != nullptr )
            {
                m_isOpen = m_pModalDialog->Draw();
            }
            else
            {
                m_isOpen = m_drawFunction();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
            {
                m_isOpen = false;
            }

            if ( !m_isOpen )
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return hasOpenModalDialog;
    }

    void DialogManager::StartModalDialog( ImGuiID viewportID, String const& title, TFunction<bool()> const& drawFunction, ImVec2 const& windowSize, bool isResizable, bool showCloseButton )
    {
        EE_ASSERT( !m_hasActiveDialog );
        EE_ASSERT( m_pModalDialog == nullptr );

        m_drawFunction = drawFunction;
        SetupDialog( viewportID, title.c_str(), windowSize, isResizable, showCloseButton );
    }

    void DialogManager::StartModalDialog( String const& title, TFunction<bool()> const& drawFunction, ImVec2 const& windowSize, bool isResizable, bool showCloseButton )
    {
        EE_ASSERT( !m_hasActiveDialog );
        EE_ASSERT( m_pModalDialog == nullptr );

        m_drawFunction = drawFunction;
        SetupDialog( ImGui::GetWindowViewport()->ID, title.c_str(), windowSize, isResizable, showCloseButton );
    }

    void DialogManager::SetupDialog( ImGuiID viewportID, char const* pTitle, ImVec2 const& windowSize, bool isResizable, bool showCloseButton )
    {
        m_viewportID = viewportID;
        m_title = pTitle;
        m_windowSize = windowSize;
        m_showCloseButton = showCloseButton;

        // If we provide any auto size values, then switch to an auto sized window
        if ( windowSize.x <= 0 && windowSize.y <= 0 )
        {
            m_windowSizeCond = ImGuiCond_FirstUseEver;
            m_windowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
        }
        else // We provided a valid window size
        {
            m_windowSizeCond = isResizable ? ImGuiCond_FirstUseEver : ImGuiCond_Always;
            m_windowFlags = isResizable ? ImGuiWindowFlags_NoSavedSettings : ( ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize );
        }

        m_hasActiveDialog = true;
        m_isOpen = true;
    }

    void DialogManager::SetActiveModalDialogSizeConstraints( ImVec2 const& min, ImVec2 const& max )
    {
        EE_ASSERT( min.x >= 0 && min.x <= max.x );
        EE_ASSERT( min.y >= 0 && min.y <= max.y );
        m_windowConstraints[0] = min;
        m_windowConstraints[1] = max;
    }
}
#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DialogManager final
    {
        friend class EditorTool;
        friend class EditorUI;

        //-------------------------------------------------------------------------

        // Abstraction for a modal dialog
        class ModalDialog
        {
            friend class EditorTool;
            friend class EditorUI;
            friend class DialogManager;

        public:

            bool HasWindowConstraints() const
            {
                return m_windowConstraints[0].x > 0 && m_windowConstraints[0].y > 0 && m_windowConstraints[1].x > 0 && m_windowConstraints[1].y > 0;
            }

        private:

            String                                          m_title;
            TFunction<bool( UpdateContext const& )>         m_drawFunction;
            ImVec2                                          m_windowSize = ImVec2( 0, 0 );
            ImVec2                                          m_windowConstraints[2] = { ImVec2( -1, -1 ), ImVec2( -1, -1 ) };
            ImVec2                                          m_windowPadding;
            ImGuiCond                                       m_windowSizeCond = ImGuiCond_Always;
            ImGuiWindowFlags                                m_windowFlags = ImGuiWindowFlags_NoSavedSettings;
            bool                                            m_isOpen = true;
        };

    public:

        DialogManager() = default;
        DialogManager( DialogManager const& ) = default;
        ~DialogManager() ;

        DialogManager& operator=( DialogManager const& rhs ) = default;

        // Do we have a currently active modal dialog
        EE_FORCE_INLINE bool HasActiveModalDialog() const { return m_pActiveDialog != nullptr; }

        // Draw dialog
        bool DrawDialog( UpdateContext const& context );

        // Create a new modal dialog
        void CreateModalDialog( String const& title, TFunction<bool( UpdateContext const& )> const& drawFunction, ImVec2 const& windowSize = ImVec2( 0, 0 ), bool isResizable = false );

        // Set the current active dialog's size constraints
        void SetActiveModalDialogSizeConstraints( ImVec2 const& min, ImVec2 const& max );

    private:

        ModalDialog* m_pActiveDialog = nullptr;
    };
}
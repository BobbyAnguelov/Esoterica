#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API ModalDialog
    {
    public:

        ModalDialog() : m_viewportID( ImGui::GetWindowViewport()->ID ) {}
        ModalDialog( ImGuiID viewportID ) : m_viewportID( viewportID ) {}
        virtual ~ModalDialog() = default;

        inline ImGuiID GetViewportID() const { return m_viewportID; }
        virtual char const* GetTitle() const = 0;
        virtual ImVec2 GetSize() const { return ImVec2( 0, 0 ); }
        virtual bool IsResizeable() const { return false; }
        virtual bool ShowCloseButton() const { return true; }

        // Return whether the dialog is still open
        virtual bool Draw() = 0;

    private:

        ImGuiID m_viewportID = 0;
    };

    // This is a helper to simplify drawing of a imgui dialogs
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DialogManager final
    {

    public:

        DialogManager() = default;
        DialogManager( DialogManager const& ) = default;
        ~DialogManager() ;

        DialogManager& operator=( DialogManager const& rhs ) = default;

        // Do we have a currently active modal dialog
        EE_FORCE_INLINE bool HasActiveModalDialog() const { return m_hasActiveDialog; }

        // Set the current active dialog's size constraints
        void SetActiveModalDialogSizeConstraints( ImVec2 const& min, ImVec2 const& max );

        // Draw dialog
        bool DrawDialog();

        // Create a new modal dialog - draw function should return whether the dialog is still open
        void StartModalDialog( ImGuiID viewportID, String const& title, TFunction<bool()> const& drawFunction, ImVec2 const& windowSize = ImVec2( 0, 0 ), bool isResizable = false, bool showCloseButton = true );

        // Create a new modal dialog - draw function should return whether the dialog is still open
        void StartModalDialog( String const& title, TFunction<bool()> const& drawFunction, ImVec2 const& windowSize = ImVec2( 0, 0 ), bool isResizable = false, bool showCloseButton = true );

        // Create a new modal dialog - draw function should return whether the dialog is still open
        template<typename T, typename ... ConstructorParams>
        void StartModalDialog( ConstructorParams&&... params )
        {
            EE_ASSERT( !m_hasActiveDialog );
            EE_ASSERT( m_pModalDialog == nullptr );

            static_assert( std::is_base_of<EE::ModalDialog, T>::value, "T is not derived from IModalDialog" );
            m_pModalDialog = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
            SetupDialog( m_pModalDialog->GetViewportID(), m_pModalDialog->GetTitle(), m_pModalDialog->GetSize(), m_pModalDialog->IsResizeable(), m_pModalDialog->ShowCloseButton() );
        }

        void StopDialog();

    private:

        void SetupDialog( ImGuiID viewportID, char const* pTitle, ImVec2 const& windowSize, bool isResizable, bool showCloseButton );

        bool HasWindowConstraints() const
        {
            return m_windowConstraints[0].x > 0 && m_windowConstraints[0].y > 0 && m_windowConstraints[1].x > 0 && m_windowConstraints[1].y > 0;
        }

    private:

        bool                                                                        m_hasActiveDialog = false;

        String                                                                      m_title;
        TFunction<bool()>                                                           m_drawFunction;
        ModalDialog*                                                                m_pModalDialog = nullptr;
        ImGuiID                                                                     m_viewportID = 0;
        ImVec2                                                                      m_windowSize = ImVec2( 0, 0 );
        ImVec2                                                                      m_windowConstraints[2] = { ImVec2( -1, -1 ), ImVec2( -1, -1 ) };
        ImVec2                                                                      m_windowPadding;
        ImGuiCond                                                                   m_windowSizeCond = ImGuiCond_Always;
        ImGuiWindowFlags                                                            m_windowFlags = ImGuiWindowFlags_NoSavedSettings;
        bool                                                                        m_showCloseButton = true;
        bool                                                                        m_isOpen = true;
    };
}
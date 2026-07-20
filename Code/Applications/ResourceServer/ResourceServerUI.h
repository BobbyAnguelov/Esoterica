#pragma once
#include "EngineTools/Core/DialogManager.h"
#include "Engine/Render/Imgui/ImguiImageCache.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiTextBuffer.h"
#include "Base/Math/Rectangle.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE { class DataPathPicker; }
namespace EE::ImGuiX { class ImageCache; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceServer;
    struct Request;

    //-------------------------------------------------------------------------

    class ResourceServerUI final
    {
    public:

        ResourceServerUI( ResourceServer& resourceServer );
        ~ResourceServerUI();

        void Initialize( ImGuiX::ImageCache* pImageCache );
        void Shutdown();
        void Draw();

        void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const;

    private:

        void SetupDockingLayout( ImGuiID dockspaceID );

        void DrawTitleBarMenu();

        void DrawServerInfoWindow();
        void DrawClientInfoWindow();
        void DrawCompilerInfoWindow();
        void DrawCompilationRequestsWindow();
        void DrawPackagingWindow();
        void DrawRecompilationBlockersWindow();

        bool DrawResaveProgressDialog();

        void SetSelectedRequest( Request const* pRequest );
        void UpdateRequestsList( int32_t sortColumnIdx, bool sortAscending );

    private:

        ImGuiX::ApplicationTitleBar                     m_titleBar;
        ResourceServer&                                 m_resourceServer;
        EventBindingID                                  m_requestsUpdatedEventBindingID;
        Request const*                                  m_pSelectedRequest = nullptr;
        Request const*                                  m_pContextMenuRequest = nullptr;
        bool                                            m_requiresLayoutReset = false;

        TVector<Request const*>                         m_requests;
        bool                                            m_requestsUpdated = false;

        mutable ImGuiX::TextBuffer                      m_resourcePathBuffer;
        DataPathPicker*                                 m_pCompileResourcePathPicker = nullptr;
        bool                                            m_forceRecompilation = false;

        ImGuiX::ImageCache*                             m_pImageCache = nullptr;
        ImGuiX::ImageInfo                               m_resourceServerIcon;
        ImGuiX::FilterWidget                            m_requestsFilter;

        mutable ImGuiX::TextBuffer                      m_compilationLogBuffer;
        bool                                            m_compilationLogBufferFilled = false;

        DataPathPicker*                                 m_pRecompilationBlockerPathPicker = nullptr;
        bool                                            m_allowSelectionChange = true;

        bool                                            m_isProfiling = false;
        DialogManager                                   m_dialogManager;
    };
}

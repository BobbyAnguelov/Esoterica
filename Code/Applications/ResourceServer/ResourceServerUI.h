#pragma once
#include "System/Math/Rectangle.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX { class ImageCache; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceServer;
    class CompilationRequest;

    //-------------------------------------------------------------------------

    class ResourceServerUI final
    {

    public:

        ResourceServerUI( ResourceServer& resourceServer, ImGuiX::ImageCache* pImageCache );
        ~ResourceServerUI();

        void Initialize();
        void Shutdown();
        void Draw();

        void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const;

    private:

        void DrawRequests();
        void DrawServerControls();
        void DrawConnectionInfo();
        void DrawPackagingControls();

    private:

        ImGuiX::ApplicationTitleBar                     m_titleBar;
        ResourceServer&                                 m_resourceServer;
        CompilationRequest const*                       m_pSelectedRequest = nullptr;
        CompilationRequest const*                       m_pContextMenuRequest = nullptr;

        char                                            m_resourcePathbuffer[255] = { 0 };
        bool                                            m_forceRecompilation = false;

        ImGuiX::ImageCache*                             m_pImageCache = nullptr;
        ImGuiX::ImageInfo                               m_resourceServerIcon;
    };
}

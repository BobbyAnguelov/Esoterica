#pragma once

#include "ResourceServer.h"
#include "ResourceServerUI.h"
#include "Engine/UpdateContext.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Imgui/ImguiRenderer.h"
#include "Base/Application/Platform/Application_Win32.h"
#include "Base/Render/RenderWindow.h"
#include "Base/Imgui/ImguiSystem.h"
#include "Base/Types/String.h"
#include "Base/Esoterica.h"
#include <shellapi.h>

//-------------------------------------------------------------------------

struct ITaskbarList3;

//-------------------------------------------------------------------------

namespace EE
{
    class ResourceServerApplication : public Win32Application
    {
        class InternalUpdateContext : public UpdateContext
        {
            friend ResourceServerApplication;
        };

    public:

        ResourceServerApplication( HINSTANCE hInstance );

    private:

        virtual bool Initialize( int32_t argc, char** argv ) override;
        virtual bool Shutdown() override;
        virtual bool ApplicationLoop() override;
        virtual bool OnUserExitRequest() override;
        virtual void OnFirstShowMainWindow() override;
        virtual void ResizeMainWindow( Int2 const& newWindowSize ) override;
        virtual void ProcessWindowDestructionMessage() override;
        virtual void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const override { m_resourceServerUI.GetBorderlessTitleBarInfo( outTitlebarRect, isInteractibleWidgetHovered ); }
        virtual LRESULT WindowMessageProcessor( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) override;

        // System Tray
        //-------------------------------------------------------------------------

        void ShowApplicationWindow();
        void HideApplicationWindow();

        bool CreateSystemTrayIcon( int32_t iconID );
        void DestroySystemTrayIcon();
        void RefreshSystemTrayIcon( int32_t iconID );
        bool ShowSystemTrayMenu();

    private:

        // System Tray Icon
        NOTIFYICONDATA                          m_systemTrayIconData;
        bool                                    m_applicationWindowHidden = false;
        int32_t                                 m_currentIconID = 0;

        // Taskbar Icon
        ITaskbarList3*                          m_pTaskbarInterface = nullptr;
        HICON                                   m_busyOverlayIcon = nullptr;
        bool                                    m_busyOverlaySet = false;

        //-------------------------------------------------------------------------

        Seconds                                 m_deltaTime = 0.0f;

        TypeSystem::TypeRegistry                m_typeRegistry;
        ImGuiX::ImguiSystem                     m_imguiSystem;

        // Rendering
        Render::RenderSystem                    m_renderSystem;
        Render::ImguiRenderer                   m_imguiRenderer;
        Render::Window                          m_renderWindow;

        // Resource
        Resource::ResourceServer                m_resourceServer;
        Resource::ResourceServerUI              m_resourceServerUI;
    };
}

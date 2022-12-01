#pragma once

#include "ResourceServer.h"
#include "ResourceServerUI.h"
#include "Engine/Render/Renderers/ImguiRenderer.h"
#include "Engine/UpdateContext.h"
#include "System/Application/Platform/Application_Win32.h"
#include "System/Render/RenderDevice.h"
#include "System/Resource/ResourceSettings.h"
#include "System/Imgui/ImguiSystem.h"
#include "System/Render/RenderViewport.h"
#include "System/Types/String.h"
#include "System/Esoterica.h"
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

        virtual bool Initialize() override;
        virtual bool Shutdown() override;
        virtual bool ApplicationLoop() override;
        virtual bool OnExitRequest() override;

        virtual void ProcessWindowResizeMessage( Int2 const& newWindowSize ) override;
        virtual void ProcessWindowDestructionMessage() override;
        virtual void GetBorderLessWindowDraggableRegions( TInlineVector<Math::ScreenSpaceRectangle, 4>& outDraggableRegions ) const override;

    private:

        NOTIFYICONDATA                          m_systemTrayIconData;

        // Taskbar Icon
        ITaskbarList3*                          m_pTaskbarInterface = nullptr;
        HICON                                   m_busyOverlayIcon = nullptr;
        bool                                    m_busyOverlaySet = false;

        //-------------------------------------------------------------------------

        Seconds                                 m_deltaTime = 0.0f;

        ImGuiX::ImguiSystem                     m_imguiSystem;

        // Rendering
        Render::RenderDevice*                   m_pRenderDevice = nullptr;
        Render::Viewport                        m_viewport;
        Render::ImguiRenderer                   m_imguiRenderer;

        // Resource
        Resource::ResourceServer                m_resourceServer;
        Resource::ResourceServerUI              m_resourceServerUI;
    };
}
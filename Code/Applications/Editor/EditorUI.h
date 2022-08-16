#pragma once

#include "EditorContext.h"
#include "EngineTools/Resource/ResourceBrowser/ResourceBrowser.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Engine/ToolsUI/IToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EditorUI final : public ImGuiX::IToolsUI
    {
    public:

        ~EditorUI();

        void SetStartupMap( ResourceID const& mapID );

        void Initialize( UpdateContext const& context ) override;
        void Shutdown( UpdateContext const& context ) override;

    private:

        void DrawMainMenu( UpdateContext const& context );
        bool DrawWorkspaceWindow( UpdateContext const& context, Workspace* pWorkspace );

        virtual void StartFrame( UpdateContext const& context ) override final;
        virtual void EndFrame( UpdateContext const& context ) override final;
        virtual void Update( UpdateContext const& context ) override final;

        // Hot Reload
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;

    private:

        void DrawUITestWindow();

    private:

        EditorContext                       m_context;
        ImGuiWindowClass                    m_editorWindowClass;

        ResourceBrowser*                    m_pResourceBrowser = nullptr;
        EventBindingID                      m_resourceDatabaseUpdateEventBindingID;
        float                               m_dataBrowserViewWidth = 150;

        ResourceID                          m_startupMap;

        SystemLogView                       m_systemLogView;
        bool                                m_isSystemLogWindowOpen = false;

        bool                                m_isResourceBrowserWindowOpen = true;
        bool                                m_isResourceLogWindowOpen = false;
        bool                                m_isResourceOverviewWindowOpen = false;
        bool                                m_isPhysicsMaterialDatabaseWindowOpen = false;
        bool                                m_isImguiDemoWindowOpen = false;
        bool                                m_isUITestWindowOpen = false;
    };
}
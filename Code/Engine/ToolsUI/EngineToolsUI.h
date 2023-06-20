#pragma once

#include "IDevelopmentToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"
#include "Engine/UpdateContext.h"
#include "System/Types/Arrays.h"


//-------------------------------------------------------------------------

struct ImDrawList;
struct ImGuiWindowClass;

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class EntityWorld;
    class EntityWorldManager;
    class EntityWorldDebugger;
    namespace Render { class Viewport; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EngineToolsUI final : public ImGuiX::IDevelopmentToolsUI
    {
        friend class GamePreviewer;

    public:

        // Locks the game overlay to a given imgui window by ID
        void LockToWindow( String const& windowName ) { m_windowName = windowName; }

    private:

        virtual void Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache ) override final;
        virtual void Shutdown( UpdateContext const& context ) override final;

        virtual void EndFrame( UpdateContext const& context ) override final;

        void DrawMenu( UpdateContext const& context, EntityWorld* pGameWorld );
        void DrawOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport );
        void DrawWindows( UpdateContext const& context, EntityWorld* pGameWorld, ImGuiWindowClass* pWindowClass = nullptr );

        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;

        void HandleUserInput( UpdateContext const& context, EntityWorld* pGameWorld );

        void ToggleWorldPause( EntityWorld* pGameWorld );
        void SetWorldTimeScale( EntityWorld* pGameWorld, float newTimeScale );
        void ResetWorldTimeScale( EntityWorld* pGameWorld );
        void RequestWorldTimeStep( EntityWorld* pGameWorld );

        void DrawPlayerDebugOptionsMenu( UpdateContext const& context, EntityWorld* pGameWorld );

    protected:

        EntityWorldManager*                                 m_pWorldManager = nullptr;
        EntityWorldDebugger*                                m_pWorldDebugger = nullptr;
        String                                              m_windowName;

        Seconds                                             m_averageDeltaTime = 0.0f;
        float                                               m_timeScale = 1.0f;
        bool                                                m_debugOverlayEnabled = false;

        SystemLogView                                       m_systemLogView;
        bool                                                m_isLogWindowOpen = false;
    };
}
#endif
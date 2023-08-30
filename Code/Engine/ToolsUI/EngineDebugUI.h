#pragma once

#include "IDevelopmentToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"
#include "Engine/UpdateContext.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

struct ImDrawList;
struct ImGuiWindowClass;

// Debug UI for the Game World
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class DebugView;
    class EntityWorld;

    namespace Render { class Viewport; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EngineDebugUI final : public ImGuiX::IDevelopmentToolsUI
    {
        // Helper to sort and categorize all the various menus that the debug views can register
        struct Menu
        {
            using MenuPath = TInlineVector<String, 5>;
            static MenuPath CreatePathFromString( String const& pathString );

        public:

            Menu( String const& title ) : m_title( title ) { EE_ASSERT( !m_title.empty() ); }

            inline bool IsEmpty() const { return m_childMenus.empty() && m_debugViews.empty(); }
            
            void Clear();

            void AddChildMenu( DebugView* pDebugView );
            void RemoveChildMenu( DebugView* pDebugView );
            void RemoveEmptyChildMenus();
            void Draw( EntityWorldUpdateContext const& context );

        private:

            Menu& FindOrAddMenu( MenuPath const& path );
            bool TryFindMenu( DebugView* pDebugView );
            bool TryFindAndRemoveMenu( DebugView* pDebugView );

        public:

            String                                              m_title;
            TVector<Menu>                                       m_childMenus;
            TVector<DebugView*>                                 m_debugViews;
        };

    public:

        virtual void Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache ) override final;
        virtual void Shutdown( UpdateContext const& context ) override final;
        virtual void HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources() override;
        virtual void HotReload_ReloadComplete() override;

        void DrawMenu( UpdateContext const& context );
        void DrawOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport );

        // Locks the game overlay to a given imgui window by ID
        void EditorPreviewUpdate( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

    private:

        virtual void EndFrame( UpdateContext const& context ) override final;
        void HandleUserInput( UpdateContext const& context );

        void ToggleWorldPause();
        void SetWorldTimeScale( float newTimeScale );
        void ResetWorldTimeScale();
        void RequestWorldTimeStep();

        void DrawPlayerDebugOptionsMenu( UpdateContext const& context );

        template<typename T>
        T* FindDebugView() const
        {
            for ( auto pDebugView : m_debugViews )
            {
                if ( auto pDesiredDebugView = TryCast<T>( pDebugView ) )
                {
                    return pDesiredDebugView;
                }
            }

            return nullptr;
        }

    protected:

        EntityWorld*                                        m_pGameWorld = nullptr;
        TVector<DebugView*>                                 m_debugViews;
        Menu                                                m_mainMenu = Menu( "Main Menu" );
        ImGuiWindowClass*                                   m_pWindowClass = nullptr;
        bool                                                m_isInEditorPreviewMode = false;

        Seconds                                             m_averageDeltaTime = 0.0f;
        float                                               m_timeScale = 1.0f;
        bool                                                m_debugOverlayEnabled = false;
    };
}
#endif
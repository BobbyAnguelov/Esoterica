#pragma once

#include "ToolsUI.h"
#include "Engine/UpdateContext.h"
#include "Engine/Debug/Widgets/SystemLogWidget.h"
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
    class Viewport;
    class EntityWorldUpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EngineDebugUI : public ImGuiX::ToolsUI
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

        EngineDebugUI() = default;
        EngineDebugUI( ImGuiWindowClass* pWindowClass ) : m_pWindowClass( pWindowClass ), m_isInEditorMode( true ) {}

        virtual void Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache ) override final;
        virtual void Shutdown( UpdateContext const& context ) override final;
        virtual void HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) override;

        void DrawMenu( UpdateContext const& context );
        void DrawOverlayUI( UpdateContext const& context, Viewport const* pViewport );

        virtual void StartFrame( UpdateContext const& context ) override final;
        virtual void EndFrame( UpdateContext const& context ) override final;

    protected:

        void HandleUserInput( UpdateContext const& context );

        void DrawWorldDebugOptionsMenu( UpdateContext const& context );

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
        Menu                                                m_engineMenu = Menu( EE_ICON_ENGINE" Engine" );
        Menu                                                m_gameMenu = Menu( EE_ICON_CONTROLLER" Game" );
        ImGuiWindowClass*                                   m_pWindowClass = nullptr;

        SystemLogWidget                                     m_systemLogWidget;
        bool                                                m_showSystemLog = false;

        Seconds                                             m_averageDeltaTime = 0.0f;
        bool                                                m_debugOverlayEnabled = false;
        bool                                                m_isInEditorMode = false;
    };
}
#endif
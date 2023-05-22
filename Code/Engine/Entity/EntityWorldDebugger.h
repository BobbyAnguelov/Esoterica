#pragma once

#include "Engine/_Module/API.h"
#include "EntityWorldDebugView.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldDebugger final
    {
    private:

        // Helper to sort and categorize all the various menus that the debug views can register
        struct Menu
        {
            using MenuPath = TInlineVector<String, 5>;
            static MenuPath CreatePathFromString( String const& pathString );

        public:

            Menu( String const& title ) : m_title( title ) { EE_ASSERT( !m_title.empty() ); }

            inline bool IsEmpty() const { return m_childMenus.empty() && m_registeredMenus.empty(); }

            void AddCallback( EntityWorldDebugView::DebugMenu const* pCallback );
            void RemoveCallback( EntityWorldDebugView::DebugMenu const* pCallback );
            void RemoveEmptyChildMenus();
            void DrawMenu( EntityWorldUpdateContext const& context );

        private:

            Menu& FindOrAddSubMenu( MenuPath const& path );
            bool TryFindMenuCallback( EntityWorldDebugView::DebugMenu const* pCallback );
            bool TryFindAndRemoveMenuCallback( EntityWorldDebugView::DebugMenu const* pCallback );

        public:

            String                                              m_title;
            TVector<Menu>                                       m_childMenus;
            TVector<EntityWorldDebugView::DebugMenu const*>     m_registeredMenus;
        };

    public:

        EntityWorldDebugger( EntityWorld const* pWorld );

        void DrawMenu( UpdateContext const& context );
        void DrawWindows( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void DrawOverlayElements( UpdateContext const& context );

        void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded );
        void EndHotReload();

    private:

        EntityWorld const*          m_pWorld = nullptr;
        Menu                        m_mainMenu = Menu( "Main Menu" );
    };
}
#endif
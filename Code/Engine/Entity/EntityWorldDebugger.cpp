#include "EntityWorldDebugger.h"
#include "System/Imgui/ImguiX.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    EntityWorldDebugger::Menu::MenuPath EntityWorldDebugger::Menu::CreatePathFromString( String const& pathString )
    {
        EE_ASSERT( !pathString.empty() );
        size_t const categoryNameLength = pathString.length();

        // Validation
        EE_ASSERT( categoryNameLength < 256 );
        if ( categoryNameLength > 0 )
        {
            for ( auto i = 0; i < categoryNameLength - 1; i++ )
            {
                EE_ASSERT( isalnum( pathString[i] ) || pathString[i] == ' ' || pathString[i] == '/' );
            }
        }

        //-------------------------------------------------------------------------

        MenuPath path;
        StringUtils::Split( pathString, path, "/" );
        return path;
    }

    void EntityWorldDebugger::Menu::AddCallback( EntityWorldDebugView::DebugMenu const* pCallback )
    {
        EE_ASSERT( !TryFindMenuCallback( pCallback ) );

        MenuPath const menuPath = CreatePathFromString( pCallback->GetPath() );
        auto& subMenu = FindOrAddSubMenu( menuPath );

        // Add new callback and sort callback list
        //-------------------------------------------------------------------------

        subMenu.m_registeredMenus.emplace_back( pCallback );

        auto callbackSortPredicate = [] ( EntityWorldDebugView::DebugMenu const* const& pA, EntityWorldDebugView::DebugMenu const* const& pB )
        {
            return pA->GetPath() < pB->GetPath();
        };

        eastl::sort( subMenu.m_registeredMenus.begin(), subMenu.m_registeredMenus.end(), callbackSortPredicate );
    }

    void EntityWorldDebugger::Menu::RemoveCallback( EntityWorldDebugView::DebugMenu const* pCallback )
    {
        bool const result = TryFindAndRemoveMenuCallback( pCallback );
        EE_ASSERT( result );
    }

    EntityWorldDebugger::Menu& EntityWorldDebugger::Menu::FindOrAddSubMenu( MenuPath const& path )
    {
        EE_ASSERT( !path.empty() );

        size_t const numPathLevels = path.size();
        int32_t currentPathLevel = 0;

        Menu* pCurrentMenu = this;
        while ( true )
        {
            // Check all child menus for a menu that matches the current path level name
            bool childMenuFound = false;
            for ( auto& childMenu : pCurrentMenu->m_childMenus )
            {
                // If the child menu matches the title, continue the search from the child menu
                if ( childMenu.m_title == path[currentPathLevel] )
                {
                    pCurrentMenu = &childMenu;
                    currentPathLevel++;

                    if ( currentPathLevel == path.size() )
                    {
                        return childMenu;
                    }
                    else
                    {
                        childMenuFound = true;
                        break;
                    }
                }
            }

            //-------------------------------------------------------------------------

            // If we didn't find a matching menu add a new one and redo the search
            if ( !childMenuFound )
            {
                // Add new child menu and sort the child menu list
                pCurrentMenu->m_childMenus.emplace_back( Menu( path[currentPathLevel] ) );

                auto menuSortPredicate = [] ( Menu const& menuA, Menu const& menuB )
                {
                    return menuA.m_title < menuB.m_title;
                };

                eastl::sort( pCurrentMenu->m_childMenus.begin(), pCurrentMenu->m_childMenus.end(), menuSortPredicate );

                // Do not change current menu or current path level, we will redo the child menu 
                // search in the next loop update and find the new menu, this is simpler than trying
                // to sort, and redo the search here
            }
        }

        //-------------------------------------------------------------------------

        EE_UNREACHABLE_CODE();
        return *this;
    }

    bool EntityWorldDebugger::Menu::TryFindMenuCallback( EntityWorldDebugView::DebugMenu const* pCallback )
    {
        auto iter = VectorFind( m_registeredMenus, pCallback );
        if ( iter != m_registeredMenus.end() )
        {
            return true;
        }
        else
        {
            for ( auto& childMenu : m_childMenus )
            {
                if ( childMenu.TryFindMenuCallback( pCallback ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool EntityWorldDebugger::Menu::TryFindAndRemoveMenuCallback( EntityWorldDebugView::DebugMenu const* pCallback )
    {
        auto iter = VectorFind( m_registeredMenus, pCallback );
        if ( iter != m_registeredMenus.end() )
        {
            m_registeredMenus.erase( iter );
            return true;
        }
        else
        {
            for ( auto& childMenu : m_childMenus )
            {
                if ( childMenu.TryFindAndRemoveMenuCallback( pCallback ) )
                {
                    RemoveEmptyChildMenus();
                    return true;
                }
            }
        }

        return false;
    }

    void EntityWorldDebugger::Menu::RemoveEmptyChildMenus()
    {
        for ( auto iter = m_childMenus.begin(); iter != m_childMenus.end(); )
        {
            ( *iter ).RemoveEmptyChildMenus();

            if ( ( *iter ).IsEmpty() )
            {
                iter = m_childMenus.erase( iter );
            }
            else
            {
                ++iter;
            }
        }
    }

    void EntityWorldDebugger::Menu::DrawMenu( EntityWorldUpdateContext const& context )
    {
        for ( auto& childMenu : m_childMenus )
        {
            EE_ASSERT( !childMenu.IsEmpty() );

            if ( ImGui::BeginMenu( childMenu.m_title.c_str() ) )
            {
                childMenu.DrawMenu( context );
                ImGui::EndMenu();
            }
        }

        //-------------------------------------------------------------------------

        for ( auto& pMenu : m_registeredMenus )
        {
            pMenu->Execute( context );
        }
    }

    //-------------------------------------------------------------------------

    EntityWorldDebugger::EntityWorldDebugger( EntityWorld const* pWorld )
        : m_pWorld( pWorld )
    {
        EE_ASSERT( m_pWorld != nullptr );

        // Register all menus
        for ( EntityWorldDebugView* pDebugView : m_pWorld->GetDebugViews() )
        {
            for ( auto& menu : pDebugView->m_menus )
            {
                m_mainMenu.AddCallback( &menu );
            }
        }
    }

    void EntityWorldDebugger::DrawMenu( UpdateContext const& context )
    {
        EntityWorldUpdateContext EntityWorldUpdateContext( context, const_cast<EntityWorld*>( m_pWorld ) );
        if ( m_mainMenu.IsEmpty() )
        {
            ImGui::Text( "No Menu Options" );
        }
        else
        {
            m_mainMenu.DrawMenu( EntityWorldUpdateContext );
        }
    }

    void EntityWorldDebugger::DrawWindows( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );

        EntityWorldUpdateContext EntityWorldUpdateContext( context, const_cast<EntityWorld*>( m_pWorld ) );
        for ( auto pDebugView : m_pWorld->GetDebugViews() )
        {
            pDebugView->DrawWindows( EntityWorldUpdateContext, pWindowClass );
        }
    }

    void EntityWorldDebugger::DrawOverlayElements( UpdateContext const& context )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );

        EntityWorldUpdateContext EntityWorldUpdateContext( context, const_cast<EntityWorld*>( m_pWorld ) );
        for ( auto pDebugView : m_pWorld->GetDebugViews() )
        {
            pDebugView->DrawOverlayElements( EntityWorldUpdateContext );
        }
    }
}
#endif
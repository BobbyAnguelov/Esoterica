#pragma once

#include "Engine/_Module/API.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/UUID.h"
#include "System/Types/Function.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

//-------------------------------------------------------------------------
// Runtime Debug View
//-------------------------------------------------------------------------
// Provides debug info and tools for a given world
// We create an instance of each of these views per world
// They are drawn at the end of each frame

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class SystemRegistry;
    class EntityWorld;
    class EntityWorldUpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldDebugView : public IRegisteredType
    {
        EE_REGISTER_TYPE( EntityWorldDebugView );

        friend class EntityWorldDebugger;

    public:

        //-------------------------------------------------------------------------
        // Debug Menu
        //-------------------------------------------------------------------------
        // The draw function will be called once the debug UI menu is shown and the appropriate category menu has been opened
        // Paths define the name and placement of the menu and are separated using '/' (e.g. "Input/Controller Utils")

        class DebugMenu
        {
        public:

            DebugMenu( String const& menuPath, TFunction<void( EntityWorldUpdateContext const& )>&& drawFunction )
                : m_path( menuPath )
                , m_drawFunction( drawFunction )
            {
                EE_ASSERT( !m_path.empty() && m_drawFunction != nullptr );
            }

            inline String const& GetPath() const { return m_path; }
            inline void Execute( EntityWorldUpdateContext const& ctx ) const { m_drawFunction( ctx ); }

        private:

            String                                              m_path;
            String                                              m_category;
            TFunction<void( EntityWorldUpdateContext const& )>       m_drawFunction;
        };

    public:

        virtual ~EntityWorldDebugView() { EE_ASSERT( m_pWorld == nullptr ); }

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) { m_pWorld = pWorld; }
        virtual void Shutdown() { m_pWorld = nullptr; }

    protected:

        // Called to draw all imgui windows
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) = 0;

        // Called within the context of a large overlay window allowing you to draw helpers and widgets over a viewport
        virtual void DrawOverlayElements( EntityWorldUpdateContext const& context ) {}

    protected:

        EntityWorld const*      m_pWorld = nullptr;
        TVector<DebugMenu>      m_menus;
    };
}
#endif
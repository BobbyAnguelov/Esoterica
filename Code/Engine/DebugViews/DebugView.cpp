#include "DebugView.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void DebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pWorld = pWorld;
        EE_ASSERT( m_pWorld != nullptr && m_pWorld->IsGameWorld() );
    }

    void DebugView::Shutdown()
    {
        m_windows.clear();
        m_pWorld = nullptr;
    }

    DebugView::Window* DebugView::GetDebugWindow( StringID typeID )
    {
        for ( auto& window : m_windows )
        {
            if ( window.m_typeID == typeID )
            {
                return &window;
            }
        }

        return nullptr;
    }

    DebugView::Window* DebugView::GetDebugWindow( uint64_t userData )
    {
        for ( auto& window : m_windows )
        {
            if ( window.m_userData == userData )
            {
                return &window;
            }
        }

        return nullptr;
    }

    DebugView::Window* DebugView::GetDebugWindow( StringID typeID, uint64_t userData )
    {
        for ( auto& window : m_windows )
        {
            if ( window.m_typeID == typeID && window.m_userData == userData )
            {
                return &window;
            }
        }

        return nullptr;
    }
}
#endif
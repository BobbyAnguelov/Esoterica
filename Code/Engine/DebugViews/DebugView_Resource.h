#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    class ResourceSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ResourceDebugView : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( ResourceDebugView );

    public:

        static void DrawLogWindow( ResourceSystem* pResourceSystem, bool* pIsOpen );
        static void DrawOverviewWindow( ResourceSystem* pResourceSystem, bool* pIsOpen );

    public:

        ResourceDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        void DrawResourceMenu( EntityWorldUpdateContext const& context );

    private:

        ResourceSystem*         m_pResourceSystem = nullptr;
        bool                    m_isHistoryWindowOpen = false;
        bool                    m_isOverviewWindowOpen = false;
    };
}
#endif
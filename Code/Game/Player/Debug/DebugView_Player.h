#pragma once

#include "Engine/Debug/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class Entity;
    class GameFlowManager;
    class PlayerSystem;

    //-------------------------------------------------------------------------

    class PlayerDebugView : public DebugView
    {
        EE_REFLECT_TYPE( PlayerDebugView );

    public:

        virtual Category GetCategory() const override { return Category::Game; }
        virtual char const* GetMenuPath() const override { return "Player"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        void UpdatePlayerEntityAndSystemPtrs();

        void DrawActionDebugger( EntityWorldUpdateContext const& context, bool isFocused );
        void DrawPrototypeInfo( EntityWorldUpdateContext const& context, bool isFocused );

    private:

        GameFlowManager*                            m_pGameFlowManager = nullptr;
        PlayerSystem*                               m_pPlayerSystem = nullptr;
        Entity*                                     m_pPlayerEntity = nullptr;
    };
}
#endif
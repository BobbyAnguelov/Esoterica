#pragma once

#include "Engine/Debug/DebugView.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class Entity;
    class GameFlowManager;
    class CharacterComponent;

    //-------------------------------------------------------------------------

    class CharacterDebugView : public DebugView
    {
        EE_REFLECT_TYPE( CharacterDebugView );

    public:

        virtual Category GetCategory() const override { return Category::Game; }
        virtual char const* GetMenuPath() const override { return ""; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void DrawOverlayUI( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        void DrawActorDebugView( EntityWorldUpdateContext const& context, bool isFocused );
        void DrawActorDebugTab( EntityWorldUpdateContext const& context, bool isFocused, CharacterComponent* pCharacter );

    private:

        GameFlowManager*                            m_pGameFlowManager = nullptr;
        TVector<ComponentID>                        m_debuggedCharacterIDs;
        TVector<CharacterComponent*>                m_debuggedCharacters;
        TVector<bool>                               m_openTabs;
    };
}
#endif
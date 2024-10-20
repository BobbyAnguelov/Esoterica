#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"

//-------------------------------------------------------------------------

namespace EE::Input { class GameInputMap; }

//-------------------------------------------------------------------------
// Player Component
//-------------------------------------------------------------------------
// This component identifies all player entities and provides common data needed for player controller systems

namespace EE::Player
{
    class EE_ENGINE_API PlayerComponent : public EntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( PlayerComponent );

    public:

        inline PlayerComponent() = default;
        inline PlayerComponent( StringID name ) : EntityComponent( name ) {}

        inline bool IsPlayerEnabled() const { return m_isEnabled; }
        inline void SetPlayerEnabled( bool isEnabled ) { m_isEnabled = isEnabled; }

        inline bool HasInputMap() const { return m_pInputMap != nullptr; }
        Input::GameInputMap* GetInputMap() { return m_pInputMap; }
        Input::GameInputMap const* GetInputMap() const { return m_pInputMap; }

    protected:

        Input::GameInputMap*            m_pInputMap = nullptr;

    private:

        bool                            m_isEnabled = true;
    };
}
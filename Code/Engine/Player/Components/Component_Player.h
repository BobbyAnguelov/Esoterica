#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityComponent.h"
#include "Engine/Input/VirtualInputRegistry.h"

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

        inline Input::VirtualInputRegistry* GetInputRegistry() { return &m_inputRegistry; }
        inline Input::VirtualInputRegistry const* GetInputRegistry() const { return &m_inputRegistry; }

    private:

        Input::VirtualInputRegistry     m_inputRegistry;
        bool                            m_isEnabled = true;
    };
}
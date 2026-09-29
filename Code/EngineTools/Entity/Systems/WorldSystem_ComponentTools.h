#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_ComponentTools.h"
#include "Engine/Entity/EntityWorldSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API ComponentToolsSystem final : public EntityWorldSystem
    {
        EE_ENTITY_WORLD_SYSTEM( ComponentToolsSystem );

    public:

        inline ComponentToolsManager* GetToolsManager() { return &m_manager; }

    private:

        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override;

    private:

        ComponentToolsManager   m_manager;
    };
}
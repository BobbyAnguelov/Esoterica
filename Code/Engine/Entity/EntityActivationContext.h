#pragma once
#include "System/Threading/Threading.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class EntityComponent;
    class TaskSystem;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct ActivationContext
    {
        ActivationContext() = default;

        ActivationContext( TaskSystem* pTaskSystem )
            : m_pTaskSystem( pTaskSystem )
        {}

        inline bool IsValid() const { return m_pTaskSystem != nullptr; }

    public:

        TaskSystem*                                                 m_pTaskSystem = nullptr;

        // World system registration
        Threading::LockFreeQueue<TPair<Entity*, EntityComponent*>>  m_componentsToRegister;
        Threading::LockFreeQueue<TPair<Entity*, EntityComponent*>>  m_componentsToUnregister;

        // Entity update registration
        Threading::LockFreeQueue<Entity*>                           m_registerForEntityUpdate;
        Threading::LockFreeQueue<Entity*>                           m_unregisterForEntityUpdate;
    };
}
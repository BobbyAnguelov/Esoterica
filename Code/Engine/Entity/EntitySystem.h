#pragma once

#include "Engine/_Module/API.h"
#include "Engine/UpdateStage.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE
{
    class SystemRegistry;
    class EntityWorldUpdateContext;
    class EntityComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntitySystem : public IReflectedType
    {
        EE_REFLECT_TYPE( EntitySystem );

        friend class Entity;

    public:

        EntitySystem() = default;
        EntitySystem( EntitySystem const& ) = default;
        virtual ~EntitySystem() {}

        EntitySystem& operator=( EntitySystem const& rhs ) = default;

        // Get the name for this system
        virtual char const* GetName() const = 0;

        // Called just before we register all the components with this system
        virtual void Initialize() {}

        // Called after all initial components have been registered with the system (on entity creation)
        virtual void PostComponentRegister() {}

        // Called before we start unregistering components from this system (on entity destruction)
        virtual void PreComponentUnregister() {}

        // Called after all components have been unregistered from the system
        virtual void Shutdown() {}

    protected:

        // Get the required update stages and priorities for this component
        virtual UpdatePriorityList const& GetRequiredUpdatePriorities() = 0;

        // Component registration
        virtual void RegisterComponent( EntityComponent* pComponent ) = 0;
        virtual void UnregisterComponent( EntityComponent* pComponent ) = 0;

        // System Update
        virtual void Update( EntityWorldUpdateContext const& ctx ) = 0;
    };
}

//-------------------------------------------------------------------------

#define EE_ENTITY_SYSTEM( TypeName, ... )\
        EE_REFLECT_TYPE( TypeName );\
        virtual UpdatePriorityList const& GetRequiredUpdatePriorities() override { static UpdatePriorityList const priorityList = UpdatePriorityList( __VA_ARGS__ ); return priorityList; };\
        virtual char const* GetName() const override { return #TypeName; }
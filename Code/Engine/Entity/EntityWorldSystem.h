#pragma once

#include "Engine/_Module/API.h"
#include "EntityWorldType.h"
#include "Engine/UpdateStage.h"
#include "Engine/Entity/EntityIDs.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Arrays.h"
#include "Base/Encoding/Hash.h"

//-------------------------------------------------------------------------
// World Entity System
//-------------------------------------------------------------------------
// This is a global system that exists once per world and tracks/updates all components of certain types in the world!

namespace EE
{
    class SystemRegistry;
    class EntityWorldUpdateContext;
    class Entity;
    class EntityComponent;
    class Viewport;
    namespace EntityModel { class EntityMap; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldSystem : public IReflectedType
    {
        EE_REFLECT_TYPE( EntityWorldSystem );

        friend class EntityWorld;
        friend EntityModel::EntityMap;

    public:

        // Is this world system in a game world
        bool IsInAGameWorld() const { return m_parentWorldType == EntityWorldType::Game; }

        // Is this world system in a tools-only world
        bool IsInAToolsWorld() const { return m_parentWorldType == EntityWorldType::Tools; }

    protected:

        // Get the required update stages and priorities for this component
        virtual UpdatePriorityList const& GetRequiredUpdatePriorities() = 0;

        // Called when the system is registered with the world - using explicit "EntitySystem" name to allow for a standalone initialize function
        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) {};

        // Called when the system is removed from the world - using explicit "EntitySystem" name to allow for a standalone shutdown function
        virtual void ShutdownSystem() {};

        // System Update - using explicit "EntitySystem" name to allow for a standalone update functions
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) {};

        // Called whenever a new component is activated (i.e. added to the world)
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) = 0;

        // Called immediately before an component is deactivated
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) = 0;

        #if EE_DEVELOPMENT_TOOLS
        // Debug draw update - called after all update stages complete for a world system (and will be called when a world is paused)
        virtual void DebugDraw( EntityWorldUpdateContext const& ctx ) {}
        #endif

    private:

        EntityWorldType m_parentWorldType = EntityWorldType::Game;
    };

    // Used for storing components and entities in ID Vectors
    //-------------------------------------------------------------------------

    template<typename T>
    struct TEntityComponentPair
    {
        TEntityComponentPair() = default;
        TEntityComponentPair( Entity* pEntity, T* pComponent ) : m_pEntity( pEntity ), m_pComponent( pComponent ) { EE_ASSERT( m_pEntity != nullptr && m_pComponent != nullptr ); }
        inline ComponentID GetID() const { return m_pComponent->GetID(); }

    public:

        Entity*     m_pEntity = nullptr;
        T*          m_pComponent = nullptr;
    };
}

//-------------------------------------------------------------------------

#define EE_ENTITY_WORLD_SYSTEM( Type, ... )\
    EE_REFLECT_TYPE( Type );\
    static UpdatePriorityList const PriorityList;\
    virtual UpdatePriorityList const& GetRequiredUpdatePriorities() override { static UpdatePriorityList const priorityList = UpdatePriorityList( __VA_ARGS__ ); return priorityList; };\

#pragma once

#include "Engine/_Module/API.h"
#include "EntityWorldType.h"
#include "Engine/UpdateStage.h"
#include "Engine/Entity/EntityIDs.h"
#include "System/TypeSystem/ReflectedType.h"
#include "System/Types/Arrays.h"
#include "System/Encoding/Hash.h"

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
    namespace EntityModel { class EntityMap; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IEntityWorldSystem : public IReflectedType
    {
        EE_REFLECT_TYPE( IEntityWorldSystem );

        friend class EntityWorld;
        friend EntityModel::EntityMap;

    public:

        virtual uint32_t GetSystemID() const = 0;

        // Is this world system in a game world
        EE_FORCE_INLINE bool IsInAGameWorld() const { return m_worldType == EntityWorldType::Game; }

        // Is this world system in a tools-only world
        EE_FORCE_INLINE bool IsInAToolsWorld() const { return m_worldType == EntityWorldType::Tools; }

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
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) = 0;

        // Called immediately before an component is deactivated
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) = 0;

    private:

        EntityWorldType     m_worldType = EntityWorldType::Game;
    };
}

//-------------------------------------------------------------------------

#define EE_ENTITY_WORLD_SYSTEM( Type, ... )\
    EE_REFLECT_TYPE( Type );\
    constexpr static uint32_t const s_entitySystemID = Hash::FNV1a::GetHash32( #Type );\
    virtual uint32_t GetSystemID() const override final { return Type::s_entitySystemID; }\
    static UpdatePriorityList const PriorityList;\
    virtual UpdatePriorityList const& GetRequiredUpdatePriorities() override { static UpdatePriorityList const priorityList = UpdatePriorityList( __VA_ARGS__ ); return priorityList; };\

#pragma once
#include "Engine/_Module/API.h"
#include "System/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class TaskSystem;
    namespace TypeSystem { class TypeRegistry; }
    namespace EntityModel { class EntityMap; struct SerializedEntityDescriptor; class SerializedEntityCollection; }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EE_ENGINE_API Serializer
    {
        static Entity* CreateEntity( TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityDescriptor const& entityDesc );
        static TVector<Entity*> CreateEntities( TaskSystem* pTaskSystem, TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityCollection const& entityCollection );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        static bool SerializeEntity( TypeSystem::TypeRegistry const& typeRegistry, Entity const* pEntity, EntityModel::SerializedEntityDescriptor& outDesc );
        static bool SerializeEntityMap( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const* pMap, SerializedEntityCollection& outCollection );
        #endif
    };
}
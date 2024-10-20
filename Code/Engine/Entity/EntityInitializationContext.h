#pragma once
#include "Base/Threading/Threading.h"
#include "Base/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class EntityMap;
    class EntityComponent;
    class TaskSystem;
    class EntityWorldSystem;
    namespace Resource { class ResourceSystem; }
    namespace TypeSystem { class TypeRegistry; }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EntityComponentPair
    {
        EntityComponentPair() = default;
        EntityComponentPair( Entity* pEntity, EntityComponent* pComponent ) : m_pEntity( pEntity ), m_pComponent( pComponent ) {}

        Entity*             m_pEntity = nullptr;
        EntityComponent*    m_pComponent = nullptr;
    };

    using EntityComponentTypeMap = THashMap<TypeSystem::TypeID, TVector<EntityComponent const*>>;

    //-------------------------------------------------------------------------

    struct InitializationContext
    {
        friend EntityMap;

    public:

        InitializationContext( TVector<EntityWorldSystem*> const& worldSystems, TVector<Entity*>& entityUpdateList )
            : m_worldSystems( worldSystems )
            , m_entityUpdateList( entityUpdateList )
        {}

        #if EE_DEVELOPMENT_TOOLS
        void SetComponentTypeMapPtr( EntityComponentTypeMap* pMap ) { m_pComponentTypeMap = pMap; }
        #endif

        inline bool IsValid() const
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( m_pComponentTypeMap == nullptr ) return false;
            #endif

            return m_pTaskSystem != nullptr && m_pTypeRegistry != nullptr;
        }

    public:

        TaskSystem* const                                           m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry const*                             m_pTypeRegistry = nullptr;

        // World system registration
        Threading::LockFreeQueue<EntityComponentPair>               m_componentsToRegister;
        Threading::LockFreeQueue<EntityComponentPair>               m_componentsToUnregister;

        // Entity update registration
        Threading::LockFreeQueue<Entity*>                           m_registerForEntityUpdate;
        Threading::LockFreeQueue<Entity*>                           m_unregisterForEntityUpdate;

    private:

        TVector<EntityWorldSystem*> const&                          m_worldSystems;
        TVector<Entity*>&                                           m_entityUpdateList;

        #if EE_DEVELOPMENT_TOOLS
        EntityComponentTypeMap*                                     m_pComponentTypeMap = nullptr;
        #endif
    };
}
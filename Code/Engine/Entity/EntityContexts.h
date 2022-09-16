#pragma once
#include "System/Threading/Threading.h"
#include "System/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class EntityMap;
    class EntityComponent;
    class TaskSystem;
    class IEntityWorldSystem;
    namespace Resource { class ResourceSystem; }
    namespace TypeSystem { class TypeRegistry; }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct LoadingContext
    {
        LoadingContext() = default;

        LoadingContext( TaskSystem* pTaskSystem, TypeSystem::TypeRegistry const* pTypeRegistry, Resource::ResourceSystem* pResourceSystem )
            : m_pTaskSystem( pTaskSystem )
            , m_pTypeRegistry( pTypeRegistry )
            , m_pResourceSystem( pResourceSystem )
        {
            EE_ASSERT( m_pTypeRegistry != nullptr && m_pResourceSystem != nullptr );
        }

        inline bool IsValid() const
        {
            return m_pTypeRegistry != nullptr && m_pResourceSystem != nullptr;
        }

    public:

        TaskSystem*                                                     m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry const*                                 m_pTypeRegistry = nullptr;
        Resource::ResourceSystem*                                       m_pResourceSystem = nullptr;
    };

    //-------------------------------------------------------------------------

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

        InitializationContext( TVector<IEntityWorldSystem*> const& worldSystems, TVector<Entity*>& entityUpdateList )
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

        TVector<IEntityWorldSystem*> const&                         m_worldSystems;
        TVector<Entity*>&                                           m_entityUpdateList;

        #if EE_DEVELOPMENT_TOOLS
        EntityComponentTypeMap*                                     m_pComponentTypeMap = nullptr;
        #endif
    };
}
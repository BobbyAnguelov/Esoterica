#pragma once

#include "EngineTools/_Module/API.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ResourceID;
    class Entity;
    class EntityWorld;
    class EntityWorldManager;
    namespace Resource { class ResourceSystem; class ResourceDatabase; }
    namespace TypeSystem { class TypeRegistry; }
    namespace FileSystem { class Path; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ToolsContext
    {
    public:

        inline bool IsValid() const { return m_pTypeRegistry != nullptr && m_pResourceDatabase != nullptr && m_pResourceSystem != nullptr; }
        FileSystem::Path const& GetRawResourceDirectory() const;
        FileSystem::Path const& GetCompiledResourceDirectory() const;

        // Resources
        //-------------------------------------------------------------------------

        virtual void TryOpenResource( ResourceID const& resourceID ) const = 0;

        // Debugging
        //-------------------------------------------------------------------------

        // Try to find a created entity
        Entity* TryFindEntityInAllWorlds( EntityID ID ) const;

        // Get all currently created worlds
        TVector<EntityWorld const*> GetAllWorlds() const;

    protected:

        virtual EntityWorldManager* GetWorldManager() const = 0;

    public:

        TypeSystem::TypeRegistry const*                     m_pTypeRegistry = nullptr;
        Resource::ResourceDatabase const*                   m_pResourceDatabase = nullptr;
        Resource::ResourceSystem*                           m_pResourceSystem = nullptr;
    };
}
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
    class SystemRegistry;
    namespace Resource { class ResourceSystem; class ResourceDatabase; }
    namespace TypeSystem { class TypeRegistry; }
    namespace FileSystem { class Path; }
    namespace ImGuiX { class ImageCache; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ToolsContext
    {
    public:

        virtual ~ToolsContext() = default;
        inline bool IsValid() const { return m_pTypeRegistry != nullptr && m_pResourceDatabase != nullptr && m_pSystemRegistry != nullptr; }
        FileSystem::Path const& GetRawResourceDirectory() const;
        FileSystem::Path const& GetCompiledResourceDirectory() const;

        // Resources
        //-------------------------------------------------------------------------

        virtual bool TryOpenResource( ResourceID const& resourceID ) const = 0;

        virtual bool TryOpenRawResource( FileSystem::Path const& resourcePath ) const = 0;

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
        SystemRegistry const*                               m_pSystemRegistry = nullptr;
        Resource::ResourceDatabase const*                   m_pResourceDatabase = nullptr;
        ImGuiX::ImageCache*                                 m_pImageCache = nullptr;
    };
}
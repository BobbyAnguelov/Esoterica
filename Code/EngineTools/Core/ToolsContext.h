#pragma once

//-------------------------------------------------------------------------

namespace EE
{
    class ResourceID;
    class EntityWorld;
    namespace Resource { class ResourceSystem; class ResourceDatabase; }
    namespace TypeSystem { class TypeRegistry; }
    namespace FileSystem { class Path; }

    //-------------------------------------------------------------------------

    class ToolsContext
    {
    public:

        inline bool IsValid() const { return m_pTypeRegistry != nullptr && m_pResourceDatabase != nullptr && m_pResourceSystem != nullptr; }
        FileSystem::Path const& GetRawResourceDirectory() const;
        FileSystem::Path const& GetCompiledResourceDirectory() const;

        // Resources
        virtual void TryOpenResource( ResourceID const& resourceID ) const = 0;

        // Debugging
        virtual bool IsGameRunning() const { return false; }
        virtual EntityWorld const* GetGameWorld() const { return nullptr; }

    public:

        TypeSystem::TypeRegistry const*                     m_pTypeRegistry = nullptr;
        Resource::ResourceDatabase const*                   m_pResourceDatabase = nullptr;
        Resource::ResourceSystem*                           m_pResourceSystem = nullptr;
    };
}
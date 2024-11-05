#pragma once

#include "EngineTools/_Module/API.h"
#include "Engine/Entity/EntityIDs.h"
#include "Base/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ResourceID;
    class Entity;
    class EntityWorld;
    class EntityWorldManager;
    class SystemRegistry;
    class DataPath;
    class FileRegistry;
    namespace Resource { class ResourceSystem; }
    namespace TypeSystem { class TypeRegistry; }
    namespace FileSystem { class Path; }
    namespace ImGuiX { class ImageCache; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ToolsContext
    {
    public:

        virtual ~ToolsContext() = default;
        inline bool IsValid() const { return m_pTypeRegistry != nullptr && m_pFileRegistry != nullptr && m_pSystemRegistry != nullptr; }
        FileSystem::Path const& GetSourceDataDirectory() const;
        FileSystem::Path const& GetCompiledResourceDirectory() const;

        // Resources
        //-------------------------------------------------------------------------

        virtual bool TryOpenDataFile( DataPath const& path ) const = 0;

        inline bool TryOpenResource( ResourceID const& resourceID ) const
        {
            return TryOpenDataFile( resourceID.GetResourcePath() );
        }

        virtual bool TryFindInResourceBrowser( DataPath const& path ) const = 0;

        inline bool TryFindInResourceBrowser( ResourceID const& resourceID ) const
        {
            ResourceID const finalResourceID = ( resourceID.IsSubResourceID() ) ? resourceID.GetParentResourceID() : resourceID;
            return TryFindInResourceBrowser( finalResourceID.GetResourcePath() );
        }

        virtual void TryCreateNewResourceDescriptor( TypeSystem::TypeID descriptorTypeID, FileSystem::Path const& startingDir = FileSystem::Path() ) const = 0;

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
        FileRegistry const*                                 m_pFileRegistry = nullptr;
        ImGuiX::ImageCache*                                 m_pImageCache = nullptr;
    };
}
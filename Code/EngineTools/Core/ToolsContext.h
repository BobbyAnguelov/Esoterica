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
    class DataFileRegistry;
    class DialogManager;
    namespace Resource { class ResourceSystem; }
    namespace TypeSystem { class TypeRegistry; }
    namespace FileSystem { class Path; }
    namespace ImGuiX { class ImageCache; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ToolsContext
    {
    public:

        struct EntityLookupResult
        {
            inline bool IsValid() const { return m_pEntity != nullptr && m_pWorld != nullptr; }

        public:

            Entity*         m_pEntity = nullptr;
            EntityWorld*    m_pWorld = nullptr;
        };

    public:

        virtual ~ToolsContext() = default;
        inline bool IsValid() const { return m_pTypeRegistry != nullptr && m_pDataFileRegistry != nullptr && m_pSystemRegistry != nullptr; }
        FileSystem::Path const& GetSourceDataDirectory() const;
        FileSystem::Path const& GetCompiledResourceDirectory() const;

        // Resources
        //-------------------------------------------------------------------------

        virtual bool TryOpenFile( DataPath const& path ) const = 0;

        inline bool TryOpenResource( ResourceID const& resourceID ) const
        {
            return TryOpenFile( resourceID.GetDataPath() );
        }

        virtual bool TryFindInResourceBrowser( DataPath const& path ) const = 0;

        inline bool TryFindInResourceBrowser( ResourceID const& resourceID ) const
        {
            ResourceID const finalResourceID = ( resourceID.IsSubResourceID() ) ? resourceID.GetParentResourceID() : resourceID;
            return TryFindInResourceBrowser( finalResourceID.GetDataPath() );
        }

        virtual void TryCreateNewResourceDescriptorOrDataFile( TypeSystem::TypeID typeID, FileSystem::Path const& startingDir = FileSystem::Path() ) const = 0;

        virtual void OpenResourceImporter() const = 0;

        virtual void ShowResourceDependencies( ResourceID const& resourceID ) const = 0;

        // Tools
        //-------------------------------------------------------------------------

        virtual InlineString GetEditorToolName( uint64_t toolID ) const = 0;

        // Debugging
        //-------------------------------------------------------------------------

        // Try to find a created entity
        EntityLookupResult TryFindEntityInAllWorlds( EntityID ID ) const;

        // Get all currently created worlds
        TVector<EntityWorld const*> GetAllWorlds() const;

    protected:

        virtual EntityWorldManager* GetWorldManager() const = 0;

    public:

        TypeSystem::TypeRegistry const*                     m_pTypeRegistry = nullptr;
        SystemRegistry const*                               m_pSystemRegistry = nullptr;
        DataFileRegistry const*                             m_pDataFileRegistry = nullptr;
        DialogManager*                                      m_pDialogManager = nullptr;
        ImGuiX::ImageCache*                                 m_pImageCache = nullptr;
    };
}
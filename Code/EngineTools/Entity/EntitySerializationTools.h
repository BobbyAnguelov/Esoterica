#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class ResourceTypeID;
    namespace FileSystem { class Path; }
    namespace TypeSystem { class TypeRegistry; }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityCollection;
    struct EntityDescriptor;
    class EntityMapDescriptor;
    class EntityMap;

    //-------------------------------------------------------------------------

    constexpr static char const* const g_entitiesNodeName = "Entities";
    constexpr static char const* const g_entityNodeName = "Entity";

    constexpr static char const* const g_entityNameAttrName = "Name";
    constexpr static char const* const g_spatialParentAttrName = "SpatialParent";
    constexpr static char const* const g_attachmentSocketAttrName = "AttachmentSocketID";

    constexpr static char const* const g_entityComponentsParentNodeName = "Components";
    constexpr static char const* const g_componentNamePropertyName = "m_name";

    constexpr static char const* const g_entitySystemsParentNodeName = "Systems";
    constexpr static char const* const g_entitySystemNodeName = "System";

    constexpr static char const* const g_referencedResourcesParentNodeName = "ReferencedResources";
    constexpr static char const* const g_referencedResourceNodeName = "Resource";
    constexpr static char const* const g_referencedResourceValueAttrName = "ID";

    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API bool CreateEntityDescriptor( TypeSystem::TypeRegistry const& typeRegistry, Entity const* pEntity, EntityDescriptor& outDesc );
    EE_ENGINETOOLS_API bool CreateEntityMapDescriptor( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const* pMap, EntityMapDescriptor& outDesc );

    EE_ENGINETOOLS_API bool ReadEntityCollectionFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, EntityCollection& outCollection );
    EE_ENGINETOOLS_API bool ReadMapDescriptorFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, EntityMapDescriptor& outMap );

    EE_ENGINETOOLS_API bool WriteEntityCollectionToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityCollection const& collection, FileSystem::Path const& outFilePath );
    EE_ENGINETOOLS_API bool WriteMapDescriptorToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityMapDescriptor const& mapDesc, FileSystem::Path const& outFilePath );
    EE_ENGINETOOLS_API bool WriteMapToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const& map, FileSystem::Path const& outFilePath );
}
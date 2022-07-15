#pragma once
#include "Engine/_Module/API.h"
#include "System/Esoterica.h"
#include "System/Serialization/JSONSerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    namespace FileSystem { class Path; }
    namespace TypeSystem { class TypeRegistry; }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EntityDescriptor;
    class EntityMap;
    class EntityCollectionDescriptor;
}

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::EntityModel::Serializer
{
    EE_ENGINE_API bool ReadEntityDescriptor( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& entityValue, EntityDescriptor& outEntityDesc );
    EE_ENGINE_API bool ReadEntityCollectionFromJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& entitiesArrayValue, EntityCollectionDescriptor& outCollectionDesc );
    EE_ENGINE_API bool ReadEntityCollectionFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, EntityCollectionDescriptor& outCollectionDesc );

    //-------------------------------------------------------------------------

    EE_ENGINE_API bool WriteEntityToJson( TypeSystem::TypeRegistry const& typeRegistry, EntityDescriptor const& entityDesc, Serialization::JsonWriter& writer );
    EE_ENGINE_API bool WriteEntityToJson( TypeSystem::TypeRegistry const& typeRegistry, Entity const* pEntity, Serialization::JsonWriter& writer );
    EE_ENGINE_API bool WriteEntityCollectionToJson( TypeSystem::TypeRegistry const& typeRegistry, EntityCollectionDescriptor const& collection, Serialization::JsonWriter& writer );
    EE_ENGINE_API bool WriteMapToJson( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const& map, Serialization::JsonWriter& writer );
    EE_ENGINE_API bool WriteEntityCollectionToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityCollectionDescriptor const& collection, FileSystem::Path const& outFilePath );
    EE_ENGINE_API bool WriteMapToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const& map, FileSystem::Path const& outFilePath );
}
#endif
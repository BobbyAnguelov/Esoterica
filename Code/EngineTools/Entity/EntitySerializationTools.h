#pragma once
#include "EngineTools/_Module/API.h"
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
    class SerializedEntityCollection;
    class SerializedEntityMap;
    class EntityMap;

    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API bool ReadSerializedEntityCollectionFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, SerializedEntityCollection& outCollection );
    EE_ENGINETOOLS_API bool ReadSerializedEntityMapFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, SerializedEntityMap& outMap );
    EE_ENGINETOOLS_API bool WriteSerializedEntityCollectionToFile( TypeSystem::TypeRegistry const& typeRegistry, SerializedEntityCollection const& collection, FileSystem::Path const& outFilePath );
    EE_ENGINETOOLS_API bool WriteMapToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const& map, FileSystem::Path const& outFilePath );
}
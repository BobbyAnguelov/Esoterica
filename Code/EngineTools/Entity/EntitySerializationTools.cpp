#include "EntitySerializationTools.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityLog.h"
#include "Base/Serialization/TypeSerialization.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/FileSystem/FileSystem.h"

#include <eastl/sort.h>

using namespace EE::Serialization;

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    static bool Error( char const* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        SystemLog::AddEntryVarArgs( Severity::Error, "Entity", "Serialization", __FILE__, __LINE__, pFormat, args);
        va_end( args );
        return false;
    }

    static bool Warning( char const* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        SystemLog::AddEntryVarArgs( Severity::Warning, "Entity", "Serializer", __FILE__, __LINE__, pFormat, args );
        va_end( args );
        return false;
    }

    //-------------------------------------------------------------------------
    // Descriptors
    //-------------------------------------------------------------------------

    bool CreateEntityDescriptor( TypeSystem::TypeRegistry const& typeRegistry, Entity const* pEntity, EntityModel::EntityDescriptor& outDesc )
    {
        EE_ASSERT( !outDesc.IsValid() );
        outDesc.m_name = pEntity->GetNameID();

        #if EE_DEVELOPMENT_TOOLS
        outDesc.m_transientEntityID = pEntity->GetID();
        #endif

        // Get spatial parent
        if ( pEntity->HasSpatialParent() )
        {
            outDesc.m_spatialParentName = pEntity->GetSpatialParent()->GetNameID();
            outDesc.m_attachmentSocketID = pEntity->GetAttachmentSocketID();
        }

        // Components
        //-------------------------------------------------------------------------

        auto ComponentComparator = [] ( EntityComponent const* pA, EntityComponent const* pB )
        {
            auto const pSpatialA = TryCast<SpatialEntityComponent>( pA );
            auto const pSpatialB = TryCast<SpatialEntityComponent>( pB );

            // If both are spatial then provide some arbitrary sort
            if ( pSpatialA != nullptr && pSpatialB != nullptr )
            {
                if ( pSpatialA->IsRootComponent() )
                {
                    return true;
                }
                else if ( pSpatialB->IsRootComponent() )
                {
                    return false;
                }

                return (uintptr_t) pA < (uintptr_t) pB;
            }
            // If neither are spatial then provide some arbitrary sort
            else if ( pSpatialA == nullptr && pSpatialB == nullptr )
            {
                return (uintptr_t) pA < (uintptr_t) pB;
            }
            else // Only one is a spatial component
            {
                return pSpatialA != nullptr;
            }
        };

        TInlineVector<EntityComponent*, 10> sortedComponents( pEntity->GetComponents().begin(), pEntity->GetComponents().end() );
        eastl::sort( sortedComponents.begin(), sortedComponents.end(), ComponentComparator );

        //-------------------------------------------------------------------------

        TVector<StringID> entityComponentList;
        for ( auto pComponent : sortedComponents )
        {
            EntityModel::ComponentDescriptor componentDesc;
            componentDesc.m_name = pComponent->GetNameID();

            #if EE_DEVELOPMENT_TOOLS
            componentDesc.m_transientComponentID = pComponent->GetID();
            #endif

            // Check for unique names
            if ( VectorContains( entityComponentList, componentDesc.m_name ) )
            {
                // Duplicate name detected!!
                EE_LOG_ENTITY_ERROR( pEntity, "Entity", "Failed to create entity descriptor, duplicate component name detected: %s on entity %s", pComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
                return false;
            }
            else
            {
                entityComponentList.emplace_back( componentDesc.m_name );
            }

            // Spatial info
            auto pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );
            if ( pSpatialEntityComponent != nullptr )
            {
                if ( !pSpatialEntityComponent->IsRootComponent() )
                {
                    EntityComponent const* pSpatialParentComponent = pEntity->FindComponent( pSpatialEntityComponent->GetSpatialParentID() );
                    componentDesc.m_spatialParentName = pSpatialParentComponent->GetNameID();
                    componentDesc.m_attachmentSocketID = pSpatialEntityComponent->GetAttachmentSocketID();
                }

                componentDesc.m_isSpatialComponent = true;
            }

            // Type descriptor - Properties
            TypeSystem::TypeDescriptor::DescribeType( typeRegistry, pComponent, componentDesc );

            // Add component
            outDesc.m_components.emplace_back( componentDesc );
            if ( componentDesc.m_isSpatialComponent )
            {
                outDesc.m_numSpatialComponents++;
            }
        }

        // Systems
        //-------------------------------------------------------------------------

        for ( EntitySystem const* pSystem : pEntity->GetSystems() )
        {
            EntityModel::SystemDescriptor systemDesc;
            systemDesc.m_typeID = pSystem->GetTypeID();
            outDesc.m_systems.emplace_back( systemDesc );
        }

        // Referenced Resources
        //-------------------------------------------------------------------------

        outDesc.m_referencedResources.clear();

        for ( auto pComponent : sortedComponents )
        {
            TypeSystem::TypeDescriptor::GetAllReferencedResources( typeRegistry, pComponent, outDesc.m_referencedResources );
        }

        return true;
    }

    bool CreateEntityMapDescriptor( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const* pMap, EntityMapDescriptor& outDesc )
    {
        //EE_ASSERT( pMap->IsLoaded() );
        EE_ASSERT( !pMap->HasPendingAddOrRemoveRequests() );

        THashMap<StringID, StringID> entityNameMap;
        entityNameMap.reserve( pMap->GetNumEntities() );

        TVector<EntityDescriptor> entityDescs;
        entityDescs.reserve( pMap->GetNumEntities() );

        for ( auto pEntity : pMap->GetEntities() )
        {
            // Check for unique names - This should never happen but we're paranoid so let's keep the extra validation
            if ( entityNameMap.find( pEntity->GetNameID() ) != entityNameMap.end() )
            {
                EE_LOG_ENTITY_ERROR( pEntity, "Entity", "Failed to create entity collection descriptor, duplicate entity name found: %s", pEntity->GetNameID().c_str() );
                return false;
            }
            else
            {
                entityNameMap.insert( TPair<StringID, StringID>( pEntity->GetNameID(), pEntity->GetNameID() ) );
            }

            //-------------------------------------------------------------------------

            EntityDescriptor entityDesc;
            if ( !CreateEntityDescriptor( typeRegistry, pEntity, entityDesc ) )
            {
                return false;
            }

            entityDescs.emplace_back( entityDesc );
        }

        //-------------------------------------------------------------------------

        outDesc.SetCollectionData( eastl::move( entityDescs ) );
        return true;
    }

    //-------------------------------------------------------------------------
    // Reading
    //-------------------------------------------------------------------------

    struct ParsingContext
    {
        ParsingContext( TypeSystem::TypeRegistry const& typeRegistry ) : m_typeRegistry( typeRegistry ) {}

        inline void ClearComponentNames()
        {
            m_componentNames.clear();
        }

        inline bool DoesComponentExist( StringID componentName ) const
        {
            return m_componentNames.find( componentName ) != m_componentNames.end();
        }

        inline bool DoesEntityExist( StringID entityName ) const
        {
            return m_entityNames.find( entityName ) != m_entityNames.end();
        }

    public:

        TypeSystem::TypeRegistry const&             m_typeRegistry;

        // Parsing context ID - Entity/Component/etc...
        StringID                                    m_parsingContextName;

        // Maps to allow for fast lookups of entity/component names for validation
        THashMap<StringID, bool>                    m_entityNames;
        THashMap<StringID, bool>                    m_componentNames;
    };

    //-------------------------------------------------------------------------

    static InlineString GetComponentName( xml_node componentNode )
    {
        TInlineVector<xml_node, 10> propertyNodes;
        GetAllChildNodes( componentNode, g_propertyNodeName, propertyNodes );
        int32_t const numProperties = (int32_t) propertyNodes.size();

        // Read all properties
        for ( pugi::xml_node propertyNode : propertyNodes )
        {
            xml_attribute nameAttr = propertyNode.attribute( g_propertyPathAttrName );
            xml_attribute valueAttr = propertyNode.attribute( g_propertyValueAttrName );
            if ( !nameAttr.empty() && !valueAttr.empty() && strcmp(nameAttr.value(), g_componentNamePropertyName) == 0 )
            {
                return valueAttr.value();
            }
        }

        return InlineString();
    }

    static bool ReadComponent( ParsingContext& ctx, xml_node componentNode, ComponentDescriptor& outComponentDesc )
    {
        // Read type descriptor
        //-------------------------------------------------------------------------

        if ( !Serialization::ReadTypeDescriptorFromXML( ctx.m_typeRegistry, componentNode, outComponentDesc ) )
        {
            InlineString const componentName = GetComponentName( componentNode );
            return Error( "Failed to deserialize type descriptor for component '%s' on entity % s!", componentName.c_str(), ctx.m_parsingContextName.c_str());
        }

        TypeSystem::TypeInfo const* pTypeInfo = ctx.m_typeRegistry.GetTypeInfo( outComponentDesc.m_typeID );
        if ( pTypeInfo == nullptr )
        {
            return Error( "Invalid entity component type ID detected for entity (%s): %s", ctx.m_parsingContextName.c_str(), outComponentDesc.m_typeID.c_str() );
        }

        // Try get name
        //-------------------------------------------------------------------------

        for ( auto const& propertyDesc : outComponentDesc.m_properties )
        {
            if ( propertyDesc.m_path.ToString() == g_componentNamePropertyName )
            {
                outComponentDesc.m_name = StringID( propertyDesc.m_stringValue );
            }
        }

        if ( !outComponentDesc.m_name.IsValid() )
        {
            outComponentDesc.m_name = Cast<EntityComponent>( pTypeInfo->GetDefaultInstance() )->GetNameID();
        }

        // Spatial component info
        //-------------------------------------------------------------------------

        outComponentDesc.m_isSpatialComponent = pTypeInfo->IsDerivedFrom<SpatialEntityComponent>();

        if ( outComponentDesc.m_isSpatialComponent )
        {
            xml_attribute spatialParentAttr = componentNode.attribute( g_spatialParentAttrName );
            if ( !spatialParentAttr.empty() )
            {
                outComponentDesc.m_spatialParentName = StringID( spatialParentAttr.as_string() );
            }

            xml_attribute attachmentSocketAttr = componentNode.attribute( g_attachmentSocketAttrName );
            if ( !attachmentSocketAttr.empty() )
            {
                outComponentDesc.m_attachmentSocketID = StringID( attachmentSocketAttr.as_string() );
            }
        }

        //-------------------------------------------------------------------------

        if ( ctx.DoesComponentExist( outComponentDesc.m_name ) )
        {
            return Error( "Duplicate component detected: '%s' on entity %s!", outComponentDesc.m_name.c_str(), ctx.m_parsingContextName.c_str() );
        }
        else
        {
            ctx.m_componentNames.insert( TPair<StringID, bool>( outComponentDesc.m_name, true ) );
            return true;
        }
    }

    //-------------------------------------------------------------------------

    static bool ReadSystemData( ParsingContext& ctx, xml_node systemNode, SystemDescriptor& outSystemDesc )
    {
        xml_attribute typeAttr = systemNode.attribute( g_typeIDAttrName );
        if ( typeAttr.empty() )
        {
            return Error( "Invalid entity system format (systems must have a TypeID string value set) on entity %s", ctx.m_parsingContextName.c_str() );
        }

        outSystemDesc.m_typeID = StringID( typeAttr.as_string() );
        return true;
    }

    //-------------------------------------------------------------------------

    static bool ReadEntityData( ParsingContext& ctx, xml_node entityNode, EntityDescriptor& outEntityDesc )
    {
        // Read name
        //-------------------------------------------------------------------------

        xml_attribute nameAttr = entityNode.attribute( g_entityNameAttrName );
        if ( nameAttr.empty() )
        {
            return Error( "Invalid entity format detected for entity (%s): entities must have name set", ctx.m_parsingContextName.c_str() );
        }

        outEntityDesc.m_name = StringID( nameAttr.as_string() );

        // Read spatial info
        //-------------------------------------------------------------------------

        xml_attribute spatialParentAttr = entityNode.attribute( g_spatialParentAttrName );
        if ( !spatialParentAttr.empty() )
        {
            outEntityDesc.m_spatialParentName = StringID( spatialParentAttr.as_string() );
        }

        xml_attribute attachmentSocketAttr = entityNode.attribute( g_attachmentSocketAttrName );
        if ( !attachmentSocketAttr.empty() )
        {
            outEntityDesc.m_attachmentSocketID = StringID( attachmentSocketAttr.as_string() );
        }

        // Set parsing ctx ID
        //-------------------------------------------------------------------------

        ctx.m_parsingContextName = outEntityDesc.m_name;

        //-------------------------------------------------------------------------
        // Read components
        //-------------------------------------------------------------------------

        ctx.ClearComponentNames();

        xml_node componentsParentNode = entityNode.child( g_entityComponentsParentNodeName );
        if ( !componentsParentNode.empty() )
        {
            // Read component data
            //-------------------------------------------------------------------------

            TInlineVector<xml_node, 10> componentNodes;
            Serialization::GetAllChildNodes( componentsParentNode, g_typeNodeName, componentNodes );
            int32_t const numComponents = (int32_t) componentNodes.size();
            EE_ASSERT( outEntityDesc.m_components.empty() && outEntityDesc.m_numSpatialComponents == 0 );
            outEntityDesc.m_components.resize( numComponents );

            bool wasRootComponentFound = false;

            for ( int32_t i = 0; i < numComponents; i++ )
            {
                if ( !ReadComponent( ctx, componentNodes[i], outEntityDesc.m_components[i] ) )
                {
                    return Error( "Failed to read component definition %u for entity (%s)", i, outEntityDesc.m_name.c_str() );
                }

                if ( outEntityDesc.m_components[i].IsSpatialComponent() )
                {
                    if ( outEntityDesc.m_components[i].IsRootComponent() )
                    {
                        if ( wasRootComponentFound )
                        {
                            return Error( "Multiple root components found on entity (%s)", outEntityDesc.m_name.c_str() );
                        }
                        else
                        {
                            wasRootComponentFound = true;
                        }
                    }

                    outEntityDesc.m_numSpatialComponents++;
                }
            }

            // Validate spatial components
            //-------------------------------------------------------------------------

            for ( auto const& componentDesc : outEntityDesc.m_components )
            {
                if ( componentDesc.IsSpatialComponent() && componentDesc.HasSpatialParent() )
                {
                    if ( !ctx.DoesComponentExist( componentDesc.m_spatialParentName ) )
                    {
                        return Error( "Couldn't find spatial parent (%s) for component (%s) on entity (%s)", componentDesc.m_spatialParentName.c_str(), componentDesc.m_name.c_str(), outEntityDesc.m_name.c_str() );
                    }
                }
            }

            // Validate singleton components
            //-------------------------------------------------------------------------
            // As soon as a given component is a singleton all components derived from it are singleton components

            for ( int32_t i = 0; i < numComponents; i++ )
            {
                auto pComponentTypeInfo = ctx.m_typeRegistry.GetTypeInfo( outEntityDesc.m_components[i].m_typeID );
                if ( pComponentTypeInfo->IsAbstractType() )
                {
                    return Error( "Abstract component type detected (%s) found on entity (%s)", pComponentTypeInfo->GetTypeName(), outEntityDesc.m_name.c_str() );
                }

                auto pDefaultComponentInstance = Cast<EntityComponent>( pComponentTypeInfo->GetDefaultInstance() );
                if ( !pDefaultComponentInstance->IsSingletonComponent() )
                {
                    continue;
                }

                for ( int32_t j = 0; j < numComponents; j++ )
                {
                    if ( i == j )
                    {
                        continue;
                    }

                    if ( ctx.m_typeRegistry.IsTypeDerivedFrom( outEntityDesc.m_components[j].m_typeID, outEntityDesc.m_components[i].m_typeID ) )
                    {
                        return Error( "Multiple singleton components of type (%s) found on the same entity (%s)", pComponentTypeInfo->GetTypeName(), outEntityDesc.m_name.c_str() );
                    }
                }
            }

            // Sort Components
            //-------------------------------------------------------------------------

            auto comparator = [] ( ComponentDescriptor const& componentDescA, ComponentDescriptor const& componentDescB )
            {
                // Spatial components have precedence
                if ( componentDescA.IsSpatialComponent() && !componentDescB.IsSpatialComponent() )
                {
                    return true;
                }

                if ( !componentDescA.IsSpatialComponent() && componentDescB.IsSpatialComponent() )
                {
                    return false;
                }

                // Handle spatial component compare - root component takes precedence
                if ( componentDescA.IsSpatialComponent() && componentDescB.IsSpatialComponent() )
                {
                    if ( componentDescA.IsRootComponent() )
                    {
                        return true;
                    }
                    else if ( componentDescB.IsRootComponent() )
                    {
                        return false;
                    }
                }

                // Arbitrary sort based on name ID
                return strcmp( componentDescA.m_name.c_str(), componentDescB.m_name.c_str() ) < 0;
            };

            eastl::sort( outEntityDesc.m_components.begin(), outEntityDesc.m_components.end(), comparator );
        }

        //-------------------------------------------------------------------------
        // Read systems
        //-------------------------------------------------------------------------

        xml_node systemsParentNode = entityNode.child( g_entitySystemsParentNodeName );
        if ( !systemsParentNode.empty() )
        {
            EE_ASSERT( outEntityDesc.m_systems.empty() );

            TInlineVector<xml_node, 10> systemNodes;
            Serialization::GetAllChildNodes( systemsParentNode, g_entitySystemNodeName, systemNodes );
            int32_t const numSystems = (int32_t) systemNodes.size();
            outEntityDesc.m_systems.resize( numSystems );

            for ( int32_t i = 0; i < numSystems; i++ )
            {
                if ( !ReadSystemData( ctx, systemNodes[i], outEntityDesc.m_systems[i] ) )
                {
                    return Error( "Failed to read system definition %u on entity (%s)", i, outEntityDesc.m_name.c_str() );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Read ReferencedResources
        //-------------------------------------------------------------------------

        xml_node referencedResourcesParentNode = entityNode.child( g_referencedResourcesParentNodeName );
        if ( !referencedResourcesParentNode.empty() )
        {
            TInlineVector<xml_node, 10> resourceNodes;
            GetAllChildNodes( referencedResourcesParentNode, g_referencedResourceNodeName, resourceNodes );
            int32_t const numResources = (int32_t) resourceNodes.size();
            outEntityDesc.m_referencedResources.resize( numResources );

            for ( int32_t i = 0; i < numResources; i++ )
            {
                ResourceID const resourceID( resourceNodes[i].attribute( g_referencedResourceValueAttrName ).as_string() );
                if ( resourceID.IsValid() )
                {
                    outEntityDesc.m_referencedResources[i] = resourceID;
                }
            }
        }

        //-------------------------------------------------------------------------

        ctx.m_parsingContextName.Clear();

        //-------------------------------------------------------------------------

        if ( ctx.DoesEntityExist( outEntityDesc.m_name ) )
        {
            return Error( "Duplicate entity name ID detected: %s", outEntityDesc.m_name.c_str() );
        }
        else
        {
            ctx.m_entityNames.insert( TPair<StringID, bool>( outEntityDesc.m_name, true ) );
            return true;
        }
    }

    static bool ReadEntityCollection( ParsingContext& ctx, xml_node entitiesParentNode, EntityCollection& outCollection )
    {
        // Get all entity nodes
        TInlineVector<xml_node, 10> entityNodes;
        GetAllChildNodes( entitiesParentNode, g_entityNodeName, entityNodes );
        int32_t const numEntities = (int32_t) entityNodes.size();

        TVector<EntityDescriptor> entityDescs;
        entityDescs.reserve( numEntities );

        for ( int32_t i = 0; i < numEntities; i++ )
        {
            EntityDescriptor entityDesc;
            if ( !ReadEntityData( ctx, entityNodes[i], entityDesc ) )
            {
                return false;
            }

            entityDescs.emplace_back( entityDesc );
        }

        //-------------------------------------------------------------------------

        outCollection.SetCollectionData( eastl::move( entityDescs ) );
        return true;
    }

    static bool ReadEntityDescriptor( TypeSystem::TypeRegistry const& typeRegistry, xml_node entityNode, EntityDescriptor& outEntityDesc )
    {
        ParsingContext ctx( typeRegistry );
        return ReadEntityData( ctx, entityNode, outEntityDesc );
    }

    static bool ReadEntityCollectionFromXml( TypeSystem::TypeRegistry const& typeRegistry, xml_document const& doc, EntityCollection& outCollection )
    {
        xml_node entitiesNode = doc.child( g_entitiesNodeName );
        if ( entitiesNode.empty() )
        {
            return Error( "Invalid format for entity collection file, missing root entities array" );
        }

        ParsingContext ctx( typeRegistry );
        return ReadEntityCollection( ctx, entitiesNode, outCollection );
    }

    //-------------------------------------------------------------------------

    bool ReadEntityCollectionFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, EntityCollection& outCollection )
    {
        EE_ASSERT( filePath.IsValid() );

        //-------------------------------------------------------------------------

        outCollection.Clear();

        //-------------------------------------------------------------------------

        xml_document doc;
        if ( !Serialization::ReadXmlFromFile( filePath, doc ) )
        {
            return Error( "Cant read source file %s", filePath.GetFullPath().c_str() );
        }

        // Read Entities
        //-------------------------------------------------------------------------

        return ReadEntityCollectionFromXml( typeRegistry, doc, outCollection );
    }

    bool ReadMapDescriptorFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, EntityMapDescriptor& outMap )
    {
        return ReadEntityCollectionFromFile( typeRegistry, filePath, outMap );
    }

    //-------------------------------------------------------------------------
    // Writing
    //-------------------------------------------------------------------------

    static bool WriteEntityComponent( xml_node componentsNode, TypeSystem::TypeRegistry const& typeRegistry, ComponentDescriptor const& componentDesc )
    {
        xml_node componentDescNode = Serialization::WriteTypeDescriptorToXML( typeRegistry, componentsNode, componentDesc );

        if ( componentDesc.m_spatialParentName.IsValid() )
        {
            componentDescNode.append_attribute( g_spatialParentAttrName ).set_value( componentDesc.m_spatialParentName.c_str() );
        }

        if ( componentDesc.m_attachmentSocketID.IsValid() )
        {
            componentDescNode.append_attribute( g_attachmentSocketAttrName ).set_value( componentDesc.m_attachmentSocketID.c_str() );
        }

        return true;
    }

    static bool WriteEntitySystem( xml_node systemsNode, TypeSystem::TypeRegistry const& typeRegistry, SystemDescriptor const& systemDesc )
    {
        if ( !systemDesc.m_typeID.IsValid() )
        {
            return Error( "Failed to write entity system desc since the system type ID was invalid." );
        }

        xml_node systemNode = systemsNode.append_child( g_entitySystemNodeName );
        systemNode.append_attribute( g_typeIDAttrName ).set_value( systemDesc.m_typeID.c_str() );
        return true;
    }

    static bool WriteEntity( TypeSystem::TypeRegistry const& typeRegistry, EntityDescriptor const& entityDesc, xml_node entitiesNode )
    {
        xml_node entityNode = entitiesNode.append_child( g_entityNodeName );
        entityNode.append_attribute( g_entityNameAttrName ).set_value( entityDesc.m_name.c_str() );

        if ( entityDesc.m_spatialParentName.IsValid() )
        {
            entityNode.append_attribute( g_spatialParentAttrName ).set_value( entityDesc.m_spatialParentName.c_str() );
        }

        if ( entityDesc.m_attachmentSocketID.IsValid() )
        {
            entityNode.append_attribute( g_attachmentSocketAttrName ).set_value( entityDesc.m_attachmentSocketID.c_str() );
        }

        //-------------------------------------------------------------------------

        if ( !entityDesc.m_components.empty() )
        {
            xml_node componentsNode = entityNode.append_child( g_entityComponentsParentNodeName );
            for ( auto const& component : entityDesc.m_components )
            {
                if ( !WriteEntityComponent( componentsNode, typeRegistry, component ) )
                {
                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( !entityDesc.m_systems.empty() )
        {
            xml_node systemsNode = entityNode.append_child( g_entitySystemsParentNodeName );
            for ( auto const& system : entityDesc.m_systems )
            {
                if ( !WriteEntitySystem( systemsNode, typeRegistry, system ) )
                {
                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( !entityDesc.m_referencedResources.empty() )
        {
            xml_node referencedResourcesNode = entityNode.append_child( g_referencedResourcesParentNodeName );
            for ( auto const& resourceID : entityDesc.m_referencedResources )
            {
                xml_node resourceNode = referencedResourcesNode.append_child( g_referencedResourceNodeName );
                resourceNode.append_attribute( g_referencedResourceValueAttrName ).set_value( resourceID.c_str() );
            }
        }

        return true;
    }

    static bool WriteEntity( TypeSystem::TypeRegistry const& typeRegistry, Entity const* pEntity, xml_node entitiesNode )
    {
        EntityDescriptor entityDesc;
        if ( !CreateEntityDescriptor( typeRegistry, pEntity, entityDesc ) )
        {
            return false;
        }

        return WriteEntity( typeRegistry, entityDesc, entitiesNode );
    }

    static bool WriteEntityCollection( TypeSystem::TypeRegistry const& typeRegistry, EntityCollection const& collection, xml_document& doc )
    {
        xml_node entitiesNode = doc.append_child( g_entitiesNodeName );

        // Sort descriptors by name to ensure consistency in output file
        //-------------------------------------------------------------------------

        TVector<EntityDescriptor> const& entityDescs = collection.GetEntityDescriptors();
        int32_t const numEntityDescs = (int32_t) entityDescs.size();
        TVector<int32_t> alphabeticallySortedDescriptors;
        alphabeticallySortedDescriptors.reserve( numEntityDescs );

        for ( int32_t i = 0; i < numEntityDescs; i++ )
        {
            alphabeticallySortedDescriptors.emplace_back( i );
        }

        auto SortPredicate = [&] ( int32_t a, int32_t b )
        {
            return _stricmp( entityDescs[a].m_name.c_str(), entityDescs[b].m_name.c_str() ) < 0;
        };

        eastl::sort( alphabeticallySortedDescriptors.begin(), alphabeticallySortedDescriptors.end(), SortPredicate );

        // Write collection to document
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < numEntityDescs; i++ )
        {
            if ( !WriteEntity( typeRegistry, entityDescs[alphabeticallySortedDescriptors[i]], entitiesNode) )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool WriteEntityCollectionToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityCollection const& collection, FileSystem::Path const& outFilePath )
    {
        EE_ASSERT( outFilePath.IsValid() );

        xml_document doc;
        WriteEntityCollection( typeRegistry, collection, doc );
        return Serialization::WriteXmlToFile( doc, outFilePath );
    }

    bool WriteMapDescriptorToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityMapDescriptor const& mapDesc, FileSystem::Path const& outFilePath )
    {
        xml_document doc;
        WriteEntityCollection( typeRegistry, mapDesc, doc );
        return Serialization::WriteXmlToFile( doc, outFilePath );
    }

    bool WriteMapToFile( TypeSystem::TypeRegistry const& typeRegistry, EntityMap const& map, FileSystem::Path const& outFilePath )
    {
        EntityMapDescriptor mapDesc;
        if ( !CreateEntityMapDescriptor( typeRegistry, &map, mapDesc ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        xml_document doc;
        WriteEntityCollection( typeRegistry, mapDesc, doc );
        return Serialization::WriteXmlToFile( doc, outFilePath );
    }
}
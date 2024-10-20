#pragma once

#include "Base/_Module/API.h"
#include "Base/TypeSystem/TypeDescriptors.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Serialization/XmlSerialization.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem 
{
    class TypeRegistry;
}

//-------------------------------------------------------------------------
// Type Serialization
//-------------------------------------------------------------------------
// This file contain basic facilities to convert between XML and the various type representations we have

#if EE_DEVELOPMENT_TOOLS
namespace EE::Serialization
{
    constexpr static char const* const g_typeNodeName = "Type";
    constexpr static char const* const g_typeIDAttrName = "TypeID";
    constexpr static char const* const g_propertyNodeName = "Property";
    constexpr static char const* const g_propertyIDAttrName = "ID";
    constexpr static char const* const g_propertyPathAttrName = "Path";
    constexpr static char const* const g_propertyArrayIdxAttrName = "Index";
    constexpr static char const* const g_propertyValueAttrName = "Value";

    // Type Descriptors
    //-------------------------------------------------------------------------

    EE_BASE_API bool ReadTypeDescriptorFromXML( TypeSystem::TypeRegistry const& typeRegistry, xml_node const& descriptorNode, TypeSystem::TypeDescriptor& outDesc );

    EE_BASE_API xml_node WriteTypeDescriptorToXML( TypeSystem::TypeRegistry const& typeRegistry, xml_node parentNode, TypeSystem::TypeDescriptor const& type );

    // Type Instances
    //-------------------------------------------------------------------------

    // Read the data for a native type from XML - expect a fully created type to be supplied and will override the values
    EE_BASE_API bool ReadType( TypeSystem::TypeRegistry const& typeRegistry, xml_node const& node, IReflectedType* pTypeInstance );

    // Read the data for a native type from XML - expect a fully created type to be supplied and will override the values
    inline bool ReadType( TypeSystem::TypeRegistry const& typeRegistry, xml_document const& doc, IReflectedType* pTypeInstance ) { return ReadType( typeRegistry, doc.first_child(), pTypeInstance ); }

    // Read the data for a native type from XML - expect a fully created type to be supplied and will override the values
    EE_BASE_API bool ReadTypeFromString( TypeSystem::TypeRegistry const& typeRegistry, String const& xmlString, IReflectedType* pTypeInstance );

    // Serialize a supplied native type to XML - creates a new XML object for this type
    EE_BASE_API void WriteType( TypeSystem::TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, xml_node& parentNode );

    // Write the property data for a supplied native type to XML
    EE_BASE_API void WriteTypeToString( TypeSystem::TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, String& outString );

    // Create a new instance of a type from a supplied XML version
    EE_BASE_API IReflectedType* TryCreateAndReadType( TypeSystem::TypeRegistry const& typeRegistry, xml_node const& node );

    // Create a new instance of a type from a supplied XML version
    inline IReflectedType* TryCreateAndReadType( TypeSystem::TypeRegistry const& typeRegistry, xml_document const& doc ) { return TryCreateAndReadType( typeRegistry, doc.first_child() ); }

    // Create a new instance of a type from a supplied XML version
    template<typename T>
    T* TryCreateAndReadType( TypeSystem::TypeRegistry const& typeRegistry, xml_node const& node )
    {
        IReflectedType* pCreatedType = TryCreateAndReadType( typeRegistry, node );
        if ( pCreatedType != nullptr )
        {
            if ( IsOfType<T>( pCreatedType ) )
            {
                return reinterpret_cast<T*>( pCreatedType );
            }
            else
            {
                EE::Delete( pCreatedType );
            }
        }
        return nullptr;
    }

    // Create a new instance of a type from a supplied XML version
    template<typename T>
    T* TryCreateAndReadType( TypeSystem::TypeRegistry const& typeRegistry, xml_document const& doc )
    {
        return TryCreateAndReadType<T>( typeRegistry, doc.first_child() );
    }

    // Utility Functions
    //-------------------------------------------------------------------------

    inline void GetAllChildNodes( xml_node const& parentNode, char const* pNodeName, TInlineVector<xml_node, 10>& outNodes )
    {
        for ( pugi::xml_node typeNode : parentNode.children( pNodeName ) )
        {
            outNodes.emplace_back( typeNode );
        }
    }
}
#endif
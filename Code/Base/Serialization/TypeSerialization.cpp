#include "TypeSerialization.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/TypeInfo.h"
#include "Base/TypeSystem/TypeInstance.h"
#include "Base/TypeSystem/CoreTypeConversions.h"

//-------------------------------------------------------------------------

using namespace EE::TypeSystem;

//-------------------------------------------------------------------------
// Descriptors
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Serialization
{
    bool ReadTypeDescriptorFromXML( TypeRegistry const& typeRegistry, xml_node const& descriptorNode, TypeDescriptor& outDesc )
    {
        auto Error = [] ( char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            SystemLog::AddEntryVarArgs( Severity::Error, "TypeSystem", "TypeDescriptor", __FILE__, __LINE__, pFormat, args );
            va_end( args );
        };

        auto Warning = [] ( char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            SystemLog::AddEntryVarArgs( Severity::Warning, "TypeSystem", "TypeDescriptor", __FILE__, __LINE__, pFormat, args );
            va_end( args );
        };

        auto ReadAndConvertPropertyValue = [&] ( TypeSystem::TypeRegistry const& typeRegistry, TypeSystem::TypeInfo const* pTypeInfo, pugi::xml_node propertyNode, TInlineVector<PropertyDescriptor, 6>& propertyDescs )
        {
            xml_attribute pathAttr = propertyNode.attribute( g_propertyPathAttrName );
            xml_attribute valueAttr = propertyNode.attribute( g_propertyValueAttrName );
            if ( pathAttr.empty() || valueAttr.empty() )
            {
                Error( "Malformed type descriptor property encountered!" );
                return false;
            }

            //-------------------------------------------------------------------------

            PropertyDescriptor propertyDesc;
            propertyDesc.m_path = TypeSystem::PropertyPath( pathAttr.as_string() );
            propertyDesc.m_stringValue = valueAttr.as_string();

            //-------------------------------------------------------------------------

            auto const pPropertyInfo = typeRegistry.ResolvePropertyPath( pTypeInfo, propertyDesc.m_path );
            if ( pPropertyInfo == nullptr )
            {
                Warning( "Failed to resolve property path: %s, for type (%s)", propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.c_str() );
            }
            else
            {
                // Handle array size descs
                if ( pPropertyInfo->IsDynamicArrayProperty() && !propertyDesc.m_path.IsPathToArrayElement() )
                {
                    if ( TypeSystem::Conversion::ConvertStringToBinary( typeRegistry, GetCoreTypeID( CoreTypeID::Int32 ), TypeID(), propertyDesc.m_stringValue, propertyDesc.m_byteValue ) )
                    {
                        propertyDescs.emplace_back( propertyDesc );
                    }
                    else
                    {
                        return false;
                    }
                }
                else if ( pPropertyInfo->IsTypeInstanceProperty() )
                {
                    if ( TypeSystem::Conversion::ConvertStringToBinary( typeRegistry, GetCoreTypeID( CoreTypeID::TypeID ), TypeID(), propertyDesc.m_stringValue, propertyDesc.m_byteValue ) )
                    {
                        propertyDescs.emplace_back( propertyDesc );
                    }
                    else
                    {
                        return false;
                    }
                }
                else // Get property value
                {
                    if ( TypeSystem::IsCoreType( pPropertyInfo->m_typeID ) || pPropertyInfo->IsEnumProperty() || pPropertyInfo->IsBitFlagsProperty() )
                    {
                        // If the value was successfully converted add it to the list of property descs
                        if ( TypeSystem::Conversion::ConvertStringToBinary( typeRegistry, *pPropertyInfo, propertyDesc.m_stringValue, propertyDesc.m_byteValue ) )
                        {
                            propertyDescs.emplace_back( propertyDesc );
                        }
                        else
                        {
                            Warning( "Failed to convert string value (%s) to binary for property: %s for type (%s)", propertyDesc.m_stringValue.c_str(), propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.c_str() );
                        }
                    }
                }
            }

            return true;
        };

        // Get Type ID
        //-------------------------------------------------------------------------

        xml_attribute typeAttr = descriptorNode.attribute( g_typeIDAttrName );
        if ( typeAttr.empty() )
        {
            Error( "Failed to find 'TypeID' member when deserializing TypeData" );
            return false;
        }

        TypeID const typeID( typeAttr.as_string() );
        auto const pTypeInfo = typeRegistry.GetTypeInfo( typeID );
        if ( pTypeInfo == nullptr )
        {
            Error( "Invalid type: %s encountered when trying to deserializing", typeID.c_str() );
            return false;
        }
        outDesc.m_typeID = typeID;

        // Properties
        //-------------------------------------------------------------------------

        // Get all property nodes
        TInlineVector<xml_node, 10> propertyNodes;
        GetAllChildNodes( descriptorNode, g_propertyNodeName, propertyNodes );
        int32_t const numProperties = (int32_t) propertyNodes.size();

        // Reserve memory for all property and create an empty desc
        outDesc.m_properties.reserve( numProperties );

        // Read all properties
        for ( pugi::xml_node propertyNode : propertyNodes )
        {
            // Try to read the property value, some properties are allowed to gracefully fail while other are a fatal error
            if ( !ReadAndConvertPropertyValue( typeRegistry, pTypeInfo, propertyNode, outDesc.m_properties ) )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    xml_node WriteTypeDescriptorToXML( TypeRegistry const& typeRegistry, xml_node parentNode, TypeDescriptor const& typeDesc )
    {
        EE_ASSERT( typeDesc.IsValid() );

        xml_node typeNode = parentNode.append_child( g_typeNodeName );
        typeNode.append_attribute( g_typeIDAttrName ).set_value( typeDesc.m_typeID.c_str() );

        for ( auto const& propertyDesc : typeDesc.m_properties )
        {
            xml_node propertyNode = typeNode.append_child( g_propertyNodeName );
            propertyNode.append_attribute( g_propertyPathAttrName ).set_value( propertyDesc.m_path.ToString().c_str() );
            propertyNode.append_attribute( g_propertyValueAttrName ).set_value( propertyDesc.m_stringValue.c_str() );
        }

        return typeNode;
    }
}

//-------------------------------------------------------------------------
// Native
//-------------------------------------------------------------------------

namespace EE::Serialization
{
    struct NativeTypeReaderXml
    {
        static bool ReadType( TypeRegistry const& typeRegistry, xml_node const& typeNode, TypeID typeID, IReflectedType* pTypeInstance )
        {
            EE_ASSERT( pTypeInstance != nullptr );
            EE_ASSERT( !IsCoreType( typeID ) );

            auto const pTypeInfo = typeRegistry.GetTypeInfo( typeID );
            if ( pTypeInfo == nullptr )
            {
                EE_LOG_ERROR( "TypeSystem", "Serialization", "Unknown type encountered: '%s'", typeID.c_str() );
                return false;
            }

            if ( strcmp( typeNode.name(), g_typeNodeName ) != 0 )
            {
                EE_LOG_ERROR( "TypeSystem", "Serialization", "Unknown node encountered: '%s'", typeNode.name() );
                return false;
            }

            xml_attribute typeAttr = typeNode.attribute( g_typeIDAttrName );
            if ( typeAttr.empty() )
            {
                EE_LOG_ERROR( "TypeSystem", "Serialization", "Missing typeID from xml representation: '%s'", typeID.c_str() );
                return false;
            }

            TypeID const actualTypeID( typeAttr.as_string() );

            if ( !typeRegistry.IsRegisteredType( actualTypeID ) )
            {
                EE_LOG_ERROR( "TypeSystem", "Serialization", "Unknown type encountered '%s'", actualTypeID.c_str() );
                return false;
            }

            // If you hit this the type in the xml file and the type you are trying to deserialize do not match
            if ( typeID != actualTypeID && !typeRegistry.IsTypeDerivedFrom( actualTypeID, typeID ) )
            {
                EE_LOG_ERROR( "TypeSystem", "Serialization", "Type mismatch, expected '%s', encountered '%s'", typeID.c_str(), actualTypeID.c_str() );
                return false;
            }

            //-------------------------------------------------------------------------

            for ( auto const& propInfo : pTypeInfo->m_properties )
            {
                // Try to find the property node
                xml_node propertyNode = typeNode.find_child_by_attribute( g_propertyIDAttrName, propInfo.m_ID.c_str() );
                if ( propertyNode.empty() )
                {
                    continue;
                }

                auto pPropertyDataAddress = propInfo.GetPropertyAddress( pTypeInstance );

                if ( propInfo.IsArrayProperty() )
                {
                    // Get all array element nodes
                    //-------------------------------------------------------------------------

                    TInlineVector<xml_node, 10> arrayElementNodes;

                    if ( ( IsCoreType( propInfo.m_typeID ) || propInfo.IsEnumProperty() ) && !propInfo.IsTypeInstanceProperty() )
                    {
                        GetAllChildNodes( propertyNode, g_propertyNodeName, arrayElementNodes );
                    }
                    else
                    {
                        GetAllChildNodes( propertyNode, g_typeNodeName, arrayElementNodes );
                    }

                    int32_t const numElements = (int32_t) arrayElementNodes.size();

                    // Static array
                    if ( propInfo.IsStaticArrayProperty() )
                    {
                        if ( propInfo.m_arraySize < numElements )
                        {
                            EE_LOG_ERROR( "TypeSystem", "Serialization", "Static array size mismatch for %s, expected maximum %d elements, encountered %d elements", propInfo.m_size, propInfo.m_size, numElements );
                            return false;
                        }

                        uint8_t* pArrayElementAddress = reinterpret_cast<uint8_t*>( pPropertyDataAddress );
                        for ( int32_t i = 0; i < numElements; i++ )
                        {
                            if ( !ReadProperty( typeRegistry, arrayElementNodes[i], propInfo, pArrayElementAddress, i ) )
                            {
                                return false;
                            }
                            pArrayElementAddress += propInfo.m_arrayElementSize;
                        }
                    }
                    else // Dynamic array
                    {
                        // If we have less elements in the json array than in the current type, clear the array as we will resize the array appropriately as part of reading the values
                        size_t const currentArraySize = pTypeInfo->GetArraySize( pTypeInstance, propInfo.m_ID.ToUint() );
                        if ( numElements < currentArraySize )
                        {
                            pTypeInfo->ClearArray( pTypeInstance, propInfo.m_ID.ToUint() );
                        }

                        // Do the traversal backwards to only allocate once
                        for ( int32_t i = ( numElements - 1 ); i >= 0; i-- )
                        {
                            auto pArrayElementAddress = pTypeInfo->GetArrayElementDataPtr( pTypeInstance, propInfo.m_ID.ToUint(), i );
                            if ( !ReadProperty( typeRegistry, arrayElementNodes[i], propInfo, pArrayElementAddress, i ) )
                            {
                                return false;
                            }
                        }
                    }
                }
                else
                {
                    if ( !ReadProperty( typeRegistry, propertyNode, propInfo, pPropertyDataAddress ) )
                    {
                         return false;
                    }
                }
            }

            //-------------------------------------------------------------------------

            pTypeInstance->PostDeserialize();

            return true;
        }

        static bool ReadProperty( TypeRegistry const& typeRegistry, xml_node const& propertyNode, PropertyInfo const& propertyInfo, void* pPropertyInstance, int32_t arrayElementIdx = InvalidIndex )
        {
            EE_ASSERT( pPropertyInstance != nullptr );

            if ( propertyInfo.IsArrayProperty() )
            {
                EE_ASSERT( !propertyNode.attribute( g_propertyArrayIdxAttrName ).empty() );
                EE_ASSERT( propertyNode.attribute( g_propertyArrayIdxAttrName ).as_int() == arrayElementIdx );
            }
            else
            {
                EE_ASSERT( strcmp( propertyNode.attribute( g_propertyIDAttrName ).as_string(), propertyInfo.m_ID.c_str() ) == 0 );
            }

            //-------------------------------------------------------------------------

            if ( propertyInfo.IsTypeInstanceProperty() )
            {
                EE_ASSERT( strcmp( propertyNode.name(), g_typeNodeName ) == 0 );

                // Has an instance set
                xml_attribute typeAttr = propertyNode.attribute( g_typeIDAttrName );
                if( !typeAttr.empty() )
                {
                    TypeID const instanceTypeID( typeAttr.as_string() );
                    TypeInfo const* pInstanceTypeInfo = typeRegistry.GetTypeInfo( instanceTypeID );
                    if ( pInstanceTypeInfo == nullptr )
                    {
                        EE_LOG_ERROR( "TypeSystem", "Serialization", "Unknown type encountered: %s", instanceTypeID.c_str() );
                        return false;
                    }

                    auto pInstanceContainer = (TypeInstance*) pPropertyInstance;
                    if ( !pInstanceTypeInfo->IsDerivedFrom( pInstanceContainer->GetAllowedBaseTypeInfo()->m_ID ) )
                    {
                        EE_LOG_ERROR( "TypeSystem", "Serialization", "Invalid type detected for instance property: %s", instanceTypeID.c_str() );
                        return false;
                    }

                    pInstanceContainer->CreateInstance( pInstanceTypeInfo );
                    return ReadType( typeRegistry, propertyNode, instanceTypeID, pInstanceContainer->Get() );
                }
                else
                {
                    auto pInstanceContainer = (TypeInstance*) pPropertyInstance;
                    pInstanceContainer->DestroyInstance();
                    return true;
                }
            }
            // Read the property node
            else if ( IsCoreType( propertyInfo.m_typeID ) || propertyInfo.IsEnumProperty() )
            {
                EE_ASSERT( strcmp( propertyNode.name(), g_propertyNodeName ) == 0 );

                xml_attribute valueAttr = propertyNode.attribute( g_propertyValueAttrName );
                EE_ASSERT( !valueAttr.empty() );
                return Conversion::ConvertStringToNativeType( typeRegistry, propertyInfo, valueAttr.as_string(), pPropertyInstance );
            }
            else // Complex Type
            {
                EE_ASSERT( strcmp( propertyNode.name(), g_typeNodeName ) == 0 );
                return ReadType( typeRegistry, propertyNode, propertyInfo.m_typeID, (IReflectedType*) pPropertyInstance );
            }

            //-------------------------------------------------------------------------

            EE_UNREACHABLE_CODE();
            return false;
        }
    };

    //-------------------------------------------------------------------------

    struct NativeTypeWriterXml
    {
        static xml_node WriteType( TypeRegistry const& typeRegistry, xml_node parentNode, String& scratchBuffer, TypeID typeID, IReflectedType const* pTypeInstance )
        {
            EE_ASSERT( !IsCoreType( typeID ) );
            auto const pTypeInfo = typeRegistry.GetTypeInfo( typeID );
            EE_ASSERT( pTypeInfo != nullptr );

            // Type and ID
            //-------------------------------------------------------------------------

            xml_node typeNode = parentNode.append_child( g_typeNodeName );
            typeNode.append_attribute( g_typeIDAttrName ).set_value( typeID.c_str() );

            // Properties
            //-------------------------------------------------------------------------

            for ( PropertyInfo const& propInfo : pTypeInfo->m_properties )
            {
                // Skip default value properties
                if ( pTypeInfo->IsPropertyValueSetToDefault( pTypeInstance, propInfo.m_ID ) )
                {
                    continue;
                }

                // Write Value
                auto pPropertyDataAddress = propInfo.GetPropertyAddress( pTypeInstance );
                if ( propInfo.IsArrayProperty() )
                {
                    size_t const elementByteSize = propInfo.m_arrayElementSize;
                    EE_ASSERT( elementByteSize > 0 );

                    size_t numArrayElements = 0;
                    uint8_t const* pElementAddress = nullptr;

                    // Static array
                    if ( propInfo.IsStaticArrayProperty() )
                    {
                        numArrayElements = propInfo.m_size / elementByteSize;
                        pElementAddress = propInfo.GetPropertyAddress<uint8_t>( pTypeInstance );
                    }
                    else // Dynamic array
                    {
                        Blob const& dynamicArray = *propInfo.GetPropertyAddress< Blob >( pTypeInstance );
                        size_t const arrayByteSize = dynamicArray.size();
                        numArrayElements = arrayByteSize / elementByteSize;
                        pElementAddress = dynamicArray.data();
                    }

                    // Create enclosing property node for array
                    xml_node arrayNode = typeNode.append_child( g_propertyNodeName );
                    arrayNode.append_attribute( g_propertyIDAttrName ).set_value( propInfo.m_ID.c_str() );

                    // Write array elements
                    for ( auto i = 0u; i < numArrayElements; i++ )
                    {
                        WriteProperty( typeRegistry, arrayNode, scratchBuffer, propInfo, pElementAddress, i );
                        pElementAddress += elementByteSize;
                    }
                }
                else
                {
                    WriteProperty( typeRegistry, typeNode, scratchBuffer, propInfo, pPropertyDataAddress );
                }
            }

            //-------------------------------------------------------------------------

            return typeNode;
        }

        static void WriteProperty( TypeRegistry const& typeRegistry, xml_node parentTypeNode, String& scratchBuffer, PropertyInfo const& propertyInfo, void const* pPropertyInstance, int32_t arrayElementIdx = InvalidIndex )
        {
            xml_node propertyNode;

            if ( propertyInfo.IsTypeInstanceProperty() )
            {
                // Skip property node and directly output the type
                auto pInstanceContainer = (TypeInstance const*) pPropertyInstance;
                if ( pInstanceContainer->IsSet() )
                {
                    propertyNode = WriteType( typeRegistry, parentTypeNode, scratchBuffer, pInstanceContainer->GetInstanceTypeInfo()->m_ID, pInstanceContainer->Get() );
                }
                else // Write empty type node
                {
                    propertyNode = parentTypeNode.append_child( g_typeNodeName );
                }
            }
            // Create a property node and write out string value
            else if ( IsCoreType( propertyInfo.m_typeID ) || propertyInfo.IsEnumProperty() )
            {
                propertyNode = parentTypeNode.append_child( g_propertyNodeName );
                Conversion::ConvertNativeTypeToString( typeRegistry, propertyInfo, pPropertyInstance, scratchBuffer );
                propertyNode.append_attribute( g_propertyValueAttrName ).set_value( scratchBuffer.c_str() );
            }
            else // Skip property node and directly output the type
            {
                propertyNode = WriteType( typeRegistry, parentTypeNode, scratchBuffer, propertyInfo.m_typeID, (IReflectedType*) pPropertyInstance );
            }

            // Write ID or index into the created property node
            //-------------------------------------------------------------------------

            if ( arrayElementIdx == InvalidIndex )
            {
                if ( propertyNode.first_attribute().empty() )
                {
                    propertyNode.append_attribute( g_propertyIDAttrName ).set_value( propertyInfo.m_ID.c_str() );
                }
                else
                {
                    propertyNode.insert_attribute_before( g_propertyIDAttrName, propertyNode.first_attribute() ).set_value( propertyInfo.m_ID.c_str() );
                }
            }
            else
            {
                if ( propertyNode.first_attribute().empty() )
                {
                    propertyNode.append_attribute( g_propertyArrayIdxAttrName ).set_value( arrayElementIdx );
                }
                else
                {
                    propertyNode.insert_attribute_before( g_propertyArrayIdxAttrName, propertyNode.first_attribute() ).set_value( arrayElementIdx );
                }
            }
        }
    };
}

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    bool ReadType( TypeRegistry const& typeRegistry, xml_node const& node, IReflectedType* pTypeInstance )
    {
        xml_attribute typeAttr = node.attribute( g_typeIDAttrName );
        if ( typeAttr.empty() )
        {
            EE_LOG_ERROR( "TypeSystem", "Serialization", "Missing type ID for type element" );
            return false;
        }

        return NativeTypeReaderXml::ReadType( typeRegistry, node, pTypeInstance->GetTypeID(), pTypeInstance );
    }

    bool ReadTypeFromString( TypeRegistry const& typeRegistry, String const& xmlString, IReflectedType* pTypeInstance )
    {
        EE_ASSERT( !xmlString.empty() && pTypeInstance != nullptr );

        xml_document doc;
        if ( !ReadXmlFromString( xmlString, doc ) )
        {
            return false;
        }

        // Reset the type
        TypeSystem::TypeInfo const* pTypeInfo = pTypeInstance->GetTypeInfo();
        pTypeInfo->ResetType( pTypeInstance );

        // Deserialize
        return ReadType( typeRegistry, doc.first_child(), pTypeInstance );
    }

    IReflectedType* TryCreateAndReadType( TypeRegistry const& typeRegistry, xml_node const& node )
    {
        xml_attribute typeAttr = node.attribute( g_typeIDAttrName );
        if ( typeAttr.empty() )
        {
            EE_LOG_ERROR( "TypeSystem", "Serialization", "Missing type ID for type element" );
            return nullptr;
        }

        TypeID const typeID( typeAttr.as_string() );
        auto const pTypeInfo = typeRegistry.GetTypeInfo( typeID );
        if ( pTypeInfo == nullptr )
        {
            EE_LOG_ERROR( "TypeSystem", "Serialization", "Unknown type encountered: %s", typeID.c_str() );
            return nullptr;
        }

        IReflectedType* pTypeInstance = pTypeInfo->CreateType();

        if ( !ReadType( typeRegistry, node, pTypeInstance ) )
        {
            EE::Delete( pTypeInstance );
            return nullptr;
        }

        return pTypeInstance;
    }

    //-------------------------------------------------------------------------

    void WriteType( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, xml_node& parentNode )
    {
        String scratchBuffer;
        scratchBuffer.reserve( 255 );
        NativeTypeWriterXml::WriteType( typeRegistry, parentNode, scratchBuffer, pTypeInstance->GetTypeID(), pTypeInstance );
    }

    void WriteTypeToString( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, String& outString )
    {
        xml_document doc;
        WriteType( typeRegistry, pTypeInstance, doc );
        Serialization::WriteXmlToString( doc, outString );
    }
}
#endif
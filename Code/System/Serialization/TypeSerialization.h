#pragma once

#include "System/_Module/API.h"
#include "System/TypeSystem/TypeDescriptors.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Serialization/JsonSerialization.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem 
{
    class TypeRegistry;
    class TypeInstanceModel;
}

//-------------------------------------------------------------------------
// Type Serialization
//-------------------------------------------------------------------------
// This file contain basic facilities to convert between JSON and the various type representations we have

#if EE_DEVELOPMENT_TOOLS
namespace EE::Serialization
{
    constexpr static char const* const s_typeIDKey = "TypeID";

    // Type Descriptors
    //-------------------------------------------------------------------------

    EE_SYSTEM_API bool ReadTypeDescriptorFromJSON( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue, TypeSystem::TypeDescriptor& outDesc );
    EE_SYSTEM_API void WriteTypeDescriptorToJSON( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer, TypeSystem::TypeDescriptor const& type );

    // Type Instances
    //-------------------------------------------------------------------------

    // Read the data for a native type from JSON - expect a fully created type to be supplied and will override the values
    EE_SYSTEM_API bool ReadNativeType( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue, IRegisteredType* pTypeInstance );

    // Read the data for a native type from JSON - expect a fully created type to be supplied and will override the values
    EE_SYSTEM_API bool ReadNativeTypeFromString( TypeSystem::TypeRegistry const& typeRegistry, String const& jsonString, IRegisteredType* pTypeInstance );

    // Serialize a supplied native type to JSON - creates a new JSON object for this type
    EE_SYSTEM_API void WriteNativeType( TypeSystem::TypeRegistry const& typeRegistry, IRegisteredType const* pTypeInstance, Serialization::JsonWriter& writer );

    // Writes out the type ID and property data for a supplied native type to an existing JSON object - Note: This function does not create a new json object!
    EE_SYSTEM_API void WriteNativeTypeContents( TypeSystem::TypeRegistry const& typeRegistry, IRegisteredType const* pTypeInstance, Serialization::JsonWriter& writer );

    // Write the property data for a supplied native type to JSON
    EE_SYSTEM_API void WriteNativeTypeToString( TypeSystem::TypeRegistry const& typeRegistry, IRegisteredType const* pTypeInstance, String& outString );

    // Create a new instance of a type from a supplied JSON version
    EE_SYSTEM_API IRegisteredType* CreateAndReadNativeType( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue );

    // Create a new instance of a type from a supplied JSON version
    template<typename T>
    T* CreateAndReadNativeType( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue )
    {
        auto pCreatedType = CreateAndReadNativeType( typeRegistry, typeObjectValue );
        return Cast<T>( pCreatedType );
    }

    //-------------------------------------------------------------------------
    // Native Type Serialization : Reading
    //-------------------------------------------------------------------------
    // Supports multiple compound types in a single archive
    // An archive is either a single serialized type or an array of serialized types
    // Each type is serialized as a JSON object with a 'TypeID' property containing the type ID of the serialized type

    class EE_SYSTEM_API TypeArchiveReader : public JsonArchiveReader
    {
    public:

        TypeArchiveReader( TypeSystem::TypeRegistry const& typeRegistry );

        // Get number of types serialized in the read json file
        inline int32_t GetNumSerializedTypes() const { return m_numSerializedTypes; }

        // Descriptor
        //-------------------------------------------------------------------------

        inline bool ReadType( TypeSystem::TypeDescriptor& typeDesc )
        {
            return ReadTypeDescriptorFromJSON( m_typeRegistry, GetObjectValueToBeDeserialized(), typeDesc );
        }

        inline TypeArchiveReader const& operator>>( TypeSystem::TypeDescriptor& typeDesc )
        {
            bool const result = ReadType( typeDesc );
            EE_ASSERT( result );
            return *this;
        }

        // Native
        //-------------------------------------------------------------------------
        // Do not try to serialize core-types using this reader

        inline bool ReadType( IRegisteredType* pType )
        {
            return ReadNativeType( m_typeRegistry, GetObjectValueToBeDeserialized(), pType );
        }

        inline TypeArchiveReader const& operator>>( IRegisteredType* pType )
        {
            bool const result = ReadType( pType );
            EE_ASSERT( result );
            return *this;
        }

        inline TypeArchiveReader const& operator>>( IRegisteredType& type )
        {
            bool const result = ReadType( &type );
            EE_ASSERT( result );
            return *this;
        }

    private:

        virtual void Reset() override final;
        virtual void OnFileReadSuccess() override final;

        Serialization::JsonValue const& GetObjectValueToBeDeserialized();

    private:

        TypeSystem::TypeRegistry const&                             m_typeRegistry;
        int32_t                                                     m_numSerializedTypes = 0;
        int32_t                                                     m_deserializedTypeIdx = 0;
    };

    //-------------------------------------------------------------------------
    // // Native Type Serialization : Writing
    //-------------------------------------------------------------------------
    // Supports multiple compound types in a single archive
    // An archive is either a single serialized type or an array of serialized types
    // Each type is serialized as a JSON object with a 'TypeID' property containing the type ID of the serialized type

    class EE_SYSTEM_API TypeArchiveWriter final : public JsonArchiveWriter
    {
    public:

        TypeArchiveWriter( TypeSystem::TypeRegistry const& typeRegistry );

        // Reset all serialized data without writing to disk
        void Reset() override;

        // Native
        //-------------------------------------------------------------------------
        // Do not try to serialize core-types using this writer

        template<typename T>
        inline TypeArchiveWriter& operator<<( T const* pType )
        {
            PreSerializeType();
            WriteNativeType( m_typeRegistry, pType, m_writer );
            m_numTypesSerialized++;
            return *this;
        }

        // Descriptor
        //-------------------------------------------------------------------------

        inline TypeArchiveWriter& operator<< ( TypeSystem::TypeDescriptor const& typeDesc )
        {
            PreSerializeType();
            WriteTypeDescriptorToJSON( m_typeRegistry, m_writer, typeDesc );
            m_numTypesSerialized++;
            return *this;
        }

    private:

        using JsonArchiveWriter::GetWriter;

        void PreSerializeType();
        virtual void FinalizeSerializedData() override final;

    private:

        TypeSystem::TypeRegistry const&                             m_typeRegistry;
        int32_t                                                     m_numTypesSerialized = 0;
    };
}
#endif
#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Serialization/TypeSerialization.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Resource/ResourceID.h"
#include "System/Types/Function.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct EE_ENGINETOOLS_API ResourceDescriptor : public IRegisteredType
    {
        EE_REGISTER_TYPE( ResourceDescriptor );

    public:

        // Try to read a descriptor from a file without knowing the type
        static inline ResourceDescriptor* TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& descriptorPath )
        {
            Serialization::TypeArchiveReader typeReader( typeRegistry );

            if ( !typeReader.ReadFromFile( descriptorPath ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Descriptor", "Failed to read resource descriptor file: %s", descriptorPath.c_str() );
                return nullptr;
            }

            //-------------------------------------------------------------------------

            auto pDescriptor = typeReader.TryReadType();
            if ( pDescriptor == nullptr )
            {
                EE_LOG_ERROR( "Resource", "Resource Descriptor", "Failed to read resource descriptor file: %s", descriptorPath.c_str() );
                return nullptr;
            }

            //-------------------------------------------------------------------------

            if ( !pDescriptor->GetTypeInfo()->IsDerivedFrom( ResourceDescriptor::GetStaticTypeID() ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Descriptor", "Read type from file: %s is not a resource descriptor", descriptorPath.c_str() );
                EE::Delete( pDescriptor );
                return nullptr;
            }

            return Cast<ResourceDescriptor>( pDescriptor );
        }

        // Try to read a specific descriptor from a file
        template<typename T>
        static bool TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& descriptorPath, T& outData )
        {
            static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );

            Serialization::TypeArchiveReader typeReader( typeRegistry );
            if ( !typeReader.ReadFromFile( descriptorPath ) )
            {
                EE_LOG_ERROR( "Resource", "Resource Descriptor", "Failed to read resource descriptor file: %s", descriptorPath.c_str() );
                return false;
            }

            if ( !typeReader.ReadType( &outData ) )
            {
                return false;
            }

            return true;
        }

        // Write a descriptor to a file
        template<typename T>
        static bool TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& descriptorPath, T const* pDescriptorData )
        {
            static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );

            EE_ASSERT( descriptorPath.IsFilePath() );

            Serialization::TypeArchiveWriter typeWriter( typeRegistry );
            typeWriter << pDescriptorData;
            return typeWriter.WriteToFile( descriptorPath );
        }

    public:

        virtual ~ResourceDescriptor() = default;

        // Get all the resources that are required for the compilation of the resource
        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) = 0;

        // Is this a valid descriptor - This only signifies whether all the required data is set and not whether the resource or any other authored data within the descriptor is valid
        virtual bool IsValid() const = 0;

        // Can this descriptor be created by a user in the editor?
        virtual bool IsUserCreateableDescriptor() const { return false; }

        // What is the compiled resource type for this descriptor - Only needed for user createable descriptors
        virtual ResourceTypeID GetCompiledResourceTypeID() const { return ResourceTypeID(); }
    };
}
#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct EE_ENGINETOOLS_API ResourceDescriptor : public IDataFile
    {
        EE_REFLECT_TYPE( ResourceDescriptor );

    public:

        static inline ResourceDescriptor* TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath )
        {
            IDataFile* pDataFile = IDataFile::TryReadFromFile( typeRegistry, dataFilePath );
            if ( pDataFile == nullptr )
            {
                return nullptr;
            }

            return Cast<ResourceDescriptor>( pDataFile );
        }

        template<typename T>
        static bool TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, T& outData )
        {
            static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );
            return IDataFile::TryReadFromFile<T>( typeRegistry, dataFilePath, outData );
        }

        template<typename T>
        static bool TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, T const* pDataFile )
        {
            static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );
            return IDataFile::TryWriteToFile(typeRegistry, dataFilePath, pDataFile );
        }

    public:

        ResourceDescriptor() = default;
        ResourceDescriptor( ResourceDescriptor const& ) = default;
        virtual ~ResourceDescriptor() = default;

        ResourceDescriptor& operator=( ResourceDescriptor const& rhs ) = default;

        virtual void Clear() = 0;

        // Get all the resources that are required for the compilation of the resource
        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) = 0;

        // Is this a valid descriptor - This only signifies whether all the required data is set and not whether the resource or any other authored data within the descriptor is valid
        virtual bool IsValid() const = 0;

        // Can this descriptor be created by a user in the editor?
        virtual bool IsUserCreateableDescriptor() const { return false; }

        // What is the compiled resource type for this descriptor - Only needed for user createable descriptors
        virtual ResourceTypeID GetCompiledResourceTypeID() const = 0;
    };
}
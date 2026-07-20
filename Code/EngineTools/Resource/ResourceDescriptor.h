#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/FileSystem/IDataFile.h"
#include "Base/Types/Function.h"
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct CompileDependency
    {
        CompileDependency() = default;
        explicit CompileDependency( DataPath const& path, bool isResource ) : m_path( path ), m_isResource( isResource ) {}

        inline bool IsValid() const { return m_path.IsValid(); }
        inline FileSystem::Path GetFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const { return m_path.GetFileSystemPath( dataDirectoryPath ); }

        operator DataPath const&() const { return m_path; }

        inline bool operator==( CompileDependency const& rhs ) const { return m_path == rhs.m_path; }
        inline bool operator!=( CompileDependency const& rhs ) const { return m_path != rhs.m_path; }
        inline bool operator==( DataPath const& rhs ) const { return m_path == rhs; }
        inline bool operator!=( DataPath const& rhs ) const { return m_path != rhs; }

    public:

        DataPath    m_path;
        bool        m_isResource = false;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API ResourceDescriptor : public IDataFile
    {
        EE_REFLECT_TYPE( ResourceDescriptor );

    public:

        static inline ResourceDescriptor* TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, uint64_t* pOutFileHash = nullptr )
        {
            IDataFile::ReadResult const result = IDataFile::TryReadFromFile( typeRegistry, log, filePath, pOutFileHash );
            if ( result.m_pDataFile == nullptr )
            {
                return nullptr;
            }

            return Cast<ResourceDescriptor>( result.m_pDataFile );
        }

        template<typename T>
        static bool TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, T& outData, uint64_t* pOutFileHash = nullptr )
        {
            static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );
            IDataFile::ReadResult const result = IDataFile::TryReadFromFile<T>( typeRegistry, log, filePath, outData, pOutFileHash );
            return result.m_succeeded;
        }

        template<typename T>
        static bool TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, T const* pDataFile )
        {
            static_assert( std::is_base_of<ResourceDescriptor, T>::value, "T must be a child of ResourceDescriptor" );
            return IDataFile::TryWriteToFile(typeRegistry, log, filePath, pDataFile );
        }

    public:

        ResourceDescriptor() = default;
        ResourceDescriptor( ResourceDescriptor const& ) = default;
        virtual ~ResourceDescriptor() = default;

        ResourceDescriptor& operator=( ResourceDescriptor const& rhs ) = default;

        virtual void Clear() = 0;

        // Is this a valid descriptor - This only signifies whether all the required data is set and not whether the resource or any other authored data within the descriptor is valid
        virtual bool IsValid() const = 0;

        // Can this descriptor be created by a user in the editor?
        virtual bool IsUserCreateableDescriptor() const { return false; }

        // When creating a new descriptor, do we need to go through an initial step or can we just create it?
        virtual bool RequiresInitialSetup() const { return true; }

        // What is the compiled resource type for this descriptor
        virtual ResourceTypeID GetCompiledResourceTypeID() const = 0;

        // Get any sub-resources that are contained within this resource
        virtual void GetAllSubResources( TVector<String>& outSubResources ) const {}

        // Get all the resources that are required for the compilation of the resource. The sub-resource ID (data://parent.ext:child.ext) will just be the child i.e. "child.ext"
        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<CompileDependency>& outDependencies ) const {}

        // Get all the resources that are required for the runtime load of the resource. The sub-resource ID (data://parent.ext:child.ext) will just be the child i.e. "child.ext"
        virtual void GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const {}
    };
}
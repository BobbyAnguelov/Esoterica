#pragma once

#include "ResourceDescriptor.h"
#include "Base/Resource/ResourceHeader.h"
#include "Base/TypeSystem/ReflectedType.h"

#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    enum class CompilationResult : int32_t
    {
        Failure = -1,
        SuccessUpToDate = 0,
        Success = 1,
        SuccessWithWarnings = 2,
    };

    // Log Delimiter
    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API CompilationLog
    {
        constexpr static char const* const s_delimiter = "Esoterica Resource Compiler\n";
    };

    // Context for a single compilation operation
    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API CompileContext
    {
        CompileContext( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& compiledResourceDirectoryPath, ResourceID const& resourceToCompile, bool isCompilingForPackagedBuild );

        bool IsValid() const;

        inline bool IsCompilingForDevelopmentBuild() const { return !m_isCompilingForPackagedBuild; }
        inline bool IsCompilingForPackagedBuild() const { return m_isCompilingForPackagedBuild; }

    public:

        Platform::Target const                          m_platform = Platform::Target::PC;
        FileSystem::Path const                          m_rawResourceDirectoryPath;
        FileSystem::Path const                          m_compiledResourceDirectoryPath;
        bool                                            m_isCompilingForPackagedBuild = false;

        ResourceID const                                m_resourceID;
        FileSystem::Path const                          m_inputFilePath;
        FileSystem::Path const                          m_outputFilePath;

        uint64_t                                        m_sourceResourceHash = 0; // The combined hash of the source resource and its dependencies
    };

    // Resource Compiler
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Compiler : public IReflectedType
    {
        EE_REFLECT_TYPE( Compiler );

    public:

        Compiler( String const& name, int32_t version ) : m_version( version ), m_name( name ) {}
        virtual ~Compiler() {}
        virtual CompilationResult Compile( CompileContext const& ctx ) const = 0;

        String const& GetName() const { return m_name; }
        inline int32_t GetVersion() const { return Serialization::GetBinarySerializationVersion() + m_version; }

        void Initialize( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath );
        void Shutdown();

        // The list of resource type we can compile
        virtual TVector<ResourceTypeID> const& GetOutputTypes() const { return m_outputTypes; }

        // Does this compiler actually require the input file or is it optional.
        virtual bool IsInputFileRequired() const { return true; }

        // Get all referenced resources needed at runtime
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const { return true; }

    protected:

        Compiler& operator=( Compiler const& ) = delete;

        CompilationResult Error( char const* pFormat, ... ) const;
        CompilationResult Warning( char const* pFormat, ... ) const;
        CompilationResult Message( char const* pFormat, ... ) const;

        CompilationResult CompilationSucceeded( CompileContext const& ctx ) const;
        CompilationResult CompilationSucceededWithWarnings( CompileContext const& ctx ) const;
        CompilationResult CompilationFailed( CompileContext const& ctx ) const;

        // Utilities
        //-------------------------------------------------------------------------

        // Converts a resource path to a file path
        inline bool ConvertResourcePathToFilePath( ResourcePath const& resourcePath, FileSystem::Path& filePath ) const
        {
            if ( resourcePath.IsValid() )
            {
                filePath = ResourcePath::ToFileSystemPath( m_rawResourceDirectoryPath, resourcePath );
                return true;
            }
            else
            {
                Error( "ResourceCompiler", "Failed to convert resource path to file system path: '%s'", resourcePath.c_str() );
                return false;
            }
        }

        // Converts a file path to a resource path
        inline bool ConvertFilePathToResourcePath( FileSystem::Path const& filePath, ResourcePath& resourcePath ) const
        {
            if ( resourcePath.IsValid() )
            {
                resourcePath = ResourcePath::FromFileSystemPath( m_rawResourceDirectoryPath, filePath );
                return true;
            }
            else
            {
                Error( "ResourceCompiler", "Failed to convert file system path to resource path: '%s'", filePath.c_str() );
                return false;
            }
        }

        // Try to load a resource descriptor from a resource path
        // Optional: returns the json document read for the descriptor if you need to read additional data from it
        template<typename T>
        bool TryLoadResourceDescriptor( FileSystem::Path const& descriptorFilePath, T& outDescriptor, rapidjson::Document* pOutOptionalDescriptorDocument = nullptr ) const
        {
            if ( !descriptorFilePath.IsValid() )
            {
                Error( "Invalid descriptor file path provided!" );
                return false;
            }

            if ( !FileSystem::Exists( descriptorFilePath ) )
            {
                Error( "Descriptor file doesnt exist: %s", descriptorFilePath.c_str() );
                return false;
            }

            Serialization::TypeArchiveReader typeReader( *m_pTypeRegistry );
            if ( !typeReader.ReadFromFile( descriptorFilePath.c_str() ) )
            {
                Error( "Failed to read resource descriptor file: %s", descriptorFilePath.c_str() );
                return false;
            }

            if ( !typeReader.ReadType( &outDescriptor ) )
            {
                Error( "Failed to read resource descriptor from file: %s", descriptorFilePath.c_str() );
                return false;
            }

            // Make a copy of the json document to read further data from
            if ( pOutOptionalDescriptorDocument != nullptr )
            {
                pOutOptionalDescriptorDocument->CopyFrom( typeReader.GetDocument(), pOutOptionalDescriptorDocument->GetAllocator(), true );
            }

            return true;
        }
        
        // Try to load a resource descriptor from a resource path
        // Optional: returns the json document read for the descriptor if you need to read additional data from it
        template<typename T>
        bool TryLoadResourceDescriptor( ResourcePath const& descriptorResourcePath, T& outDescriptor, rapidjson::Document* pOutOptionalDescriptorDocument = nullptr ) const
        {
            if ( !descriptorResourcePath.IsValid() )
            {
                Error( "Invalid descriptor resource path provided!" );
                return false;
            }

            FileSystem::Path descriptorFilePath;
            if ( !ConvertResourcePathToFilePath( descriptorResourcePath, descriptorFilePath ) )
            {
                Error( "Invalid descriptor resource path: %s", descriptorResourcePath.c_str() );
                return false;
            }

            return TryLoadResourceDescriptor( descriptorFilePath, outDescriptor, pOutOptionalDescriptorDocument );
        }

        // Combines two results together and keeps the most severe one
        EE_FORCE_INLINE CompilationResult CombineResultCode( CompilationResult a, CompilationResult b ) const
        {
            if ( a == Resource::CompilationResult::Failure || b == Resource::CompilationResult::Failure )
            {
                return Resource::CompilationResult::Failure;
            }

            if ( a == Resource::CompilationResult::SuccessWithWarnings || b == Resource::CompilationResult::SuccessWithWarnings )
            {
                return Resource::CompilationResult::SuccessWithWarnings;
            }

            if ( a == Resource::CompilationResult::SuccessUpToDate || b == Resource::CompilationResult::SuccessUpToDate )
            {
                return Resource::CompilationResult::SuccessUpToDate;
            }

            return Resource::CompilationResult::Success;
        }

    protected:

        TypeSystem::TypeRegistry const*                 m_pTypeRegistry = nullptr;
        FileSystem::Path                                m_rawResourceDirectoryPath;
        int32_t const                                   m_version;
        String const                                    m_name;
        TVector<ResourceTypeID>                         m_outputTypes;
    };
}
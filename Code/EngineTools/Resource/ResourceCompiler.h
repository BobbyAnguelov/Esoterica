#pragma once

#include "ResourceDescriptor.h"
#include "Base/Resource/ResourceHeader.h"
#include "Base/Resource/IResource.h"
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
        constexpr static char const* const s_delimiter = "Esoterica Resource Compiler\n-------------------------------------------------------------------------\n\n";
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
        FileSystem::Path const                          m_sourceDataDirectoryPath;
        FileSystem::Path const                          m_compiledResourceDirectoryPath;
        bool                                            m_isCompilingForPackagedBuild = false;

        ResourceID const                                m_resourceID;
        FileSystem::Path const                          m_inputFilePath;
        FileSystem::Path const                          m_outputFilePath;

        uint64_t                                        m_sourceResourceHash = 0; // The combined hash of the source resource and its dependencies
        uint64_t                                        m_advancedUpToDateHash = 0; // The optional advanced hash of the source source
    };

    // Resource Compiler
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Compiler : public IReflectedType
    {
        EE_REFLECT_TYPE( Compiler );

    public:

        struct OutputType
        {
            ResourceTypeID  m_typeID;
            int32_t         m_version;
            bool            m_requiresAdditionalDataFile;
        };

    public:

        Compiler( String const& name ) : m_name( name ) {}
        virtual ~Compiler() {}

        // Get the friendly name for this compiler
        String const& GetName() const { return m_name; }

        // Get the version for the resource
        int32_t GetVersion( ResourceTypeID resourceTypeID ) const;

        void Initialize( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath );
        void Shutdown();

        // The list of resource type we can compile
        virtual TVector<OutputType> const& GetOutputTypes() const { return m_outputTypes; }

        // Will this compiler generate an additional data file for this resource type?
        bool WillGenerateAdditionalDataFile( ResourceTypeID resourceTypeID ) const;

        // Does this compiler actually require the input file or is it optional.
        virtual bool IsInputFileRequired() const { return true; }

        // Get all referenced resources needed at runtime
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const { return true; }

        // Does this resource require an additional advanced up-to-date check
        virtual bool RequiresAdvancedUpToDateCheck( ResourceTypeID resourceTypeID ) const { return false; }

        // Calculate the advanced up to date check hash
        virtual uint64_t CalculateAdvancedUpToDateHash( ResourceID resourceID ) const { return 0; }

        // Compile a resource
        virtual CompilationResult Compile( CompileContext const& ctx ) const = 0;

    protected:

        Compiler& operator=( Compiler const& ) = delete;

        CompilationResult Error( char const* pFormat, ... ) const;
        CompilationResult Warning( char const* pFormat, ... ) const;
        CompilationResult Message( char const* pFormat, ... ) const;

        CompilationResult CompilationSucceeded( CompileContext const& ctx ) const;
        CompilationResult CompilationSucceededWithWarnings( CompileContext const& ctx ) const;
        CompilationResult CompilationFailed( CompileContext const& ctx ) const;

        // Initialization
        //-------------------------------------------------------------------------

        template<typename T>
        void AddOutputType()
        {
            static_assert( std::is_base_of<EE::Resource::IResource, T>::value, "T is not derived from IResource" );
            m_outputTypes.emplace_back( T::GetStaticResourceTypeID(), T::s_version, T::s_requiresAdditionalDataFile );
        }

        // Utilities
        //-------------------------------------------------------------------------

        // Converts a resource path to a file path
        inline bool GetFilePathForResourceID( ResourceID const& resourceID, FileSystem::Path& filePath ) const
        {
            if ( resourceID.IsValid() )
            {
                filePath = resourceID.GetFileSystemPath( m_sourceDataDirectoryPath );
                return true;
            }
            else
            {
                Error( "ResourceCompiler", "Failed to convert resource path to file system path: '%s'", resourceID.c_str() );
                return false;
            }
        }


        // Converts a resource path to a file path
        inline bool ConvertDataPathToFilePath( DataPath const& resourcePath, FileSystem::Path& filePath ) const
        {
            if ( resourcePath.IsValid() )
            {
                filePath = resourcePath.GetFileSystemPath( m_sourceDataDirectoryPath );
                return true;
            }
            else
            {
                Error( "ResourceCompiler", "Failed to convert resource path to file system path: '%s'", resourcePath.c_str() );
                return false;
            }
        }

        // Converts a file path to a resource path
        inline bool ConvertFilePathToDataPath( FileSystem::Path const& filePath, DataPath& resourcePath ) const
        {
            if ( resourcePath.IsValid() )
            {
                resourcePath = DataPath::FromFileSystemPath( m_sourceDataDirectoryPath, filePath );
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
        bool TryLoadResourceDescriptor( FileSystem::Path const& descriptorFilePath, T& outDescriptor ) const
        {
            if ( !descriptorFilePath.IsValid() )
            {
                Error( "Invalid descriptor file path provided!" );
                return false;
            }

            if ( !descriptorFilePath.Exists() )
            {
                Error( "Descriptor file doesnt exist: %s", descriptorFilePath.c_str() );
                return false;
            }

            if ( !ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, descriptorFilePath, outDescriptor ) )
            {
                EE_LOG_ERROR( "ResourceCompiler", "Load Descriptor", "Failed to read data file: %s", descriptorFilePath.c_str() );
                return false;
            }

            return true;
        }

        // Try to load a resource descriptor from a resource path
        // Optional: returns the json document read for the descriptor if you need to read additional data from it
        template<typename T>
        bool TryLoadResourceDescriptor( DataPath const& descriptorResourcePath, T& outDescriptor ) const
        {
            if ( !descriptorResourcePath.IsValid() )
            {
                Error( "Invalid descriptor resource path provided!" );
                return false;
            }

            FileSystem::Path descriptorFilePath;
            if ( !ConvertDataPathToFilePath( descriptorResourcePath, descriptorFilePath ) )
            {
                Error( "Invalid descriptor resource path: %s", descriptorResourcePath.c_str() );
                return false;
            }

            return TryLoadResourceDescriptor( descriptorFilePath, outDescriptor );
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
        FileSystem::Path                                m_sourceDataDirectoryPath;
        String const                                    m_name;

    private:

        TVector<OutputType>                             m_outputTypes;
    };
}
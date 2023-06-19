#pragma once

#include "ResourceDescriptor.h"
#include "System/Resource/ResourceHeader.h"
#include "System/TypeSystem/ReflectedType.h"
#include "System/Log.h"
#include "System/Types/Function.h"

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

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API CompileContext
    {
        CompileContext( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& compiledResourceDirectoryPath, ResourceID const& resourceToCompile, bool isCompilingForShippingBuild );

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
                Error( "ResourceCompiler", "Invalid data path encountered: '%s'", resourcePath.c_str() );
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

    protected:

        TypeSystem::TypeRegistry const*                 m_pTypeRegistry = nullptr;
        FileSystem::Path                                m_rawResourceDirectoryPath;
        int32_t const                                   m_version;
        String const                                    m_name;
        TVector<ResourceTypeID>                         m_outputTypes;
    };
}
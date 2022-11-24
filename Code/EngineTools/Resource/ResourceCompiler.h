#pragma once

#include "ResourceDescriptor.h"
#include "System/Resource/ResourceHeader.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Log.h"
#include "System/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    enum class CompilationResult
    {
        Failure = -1,
        Success = 0,
        SuccessWithWarnings = 1,
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
        FileSystem::Path const                          m_compiledResourceDirectoryPath;
        bool                                            m_isCompilingForPackagedBuild = false;

        ResourceID const                                m_resourceID;
        FileSystem::Path const                          m_inputFilePath;
        FileSystem::Path const                          m_outputFilePath;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Compiler : public IRegisteredType
    {
        EE_REGISTER_TYPE( Compiler );

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

        // Get all referenced resources for a specific resource
        virtual bool GetReferencedResources( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const { return true; }

    protected:

        Compiler& operator=( Compiler const& ) = delete;

        CompilationResult Error( char const* pFormat, ... ) const;
        CompilationResult Warning( char const* pFormat, ... ) const;
        CompilationResult Message( char const* pFormat, ... ) const;

        CompilationResult CompilationSucceeded( CompileContext const& ctx ) const;
        CompilationResult CompilationSucceededWithWarnings( CompileContext const& ctx ) const;
        CompilationResult CompilationFailed( CompileContext const& ctx ) const;

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

    protected:

        TypeSystem::TypeRegistry const*                 m_pTypeRegistry = nullptr;
        FileSystem::Path                                m_rawResourceDirectoryPath;
        int32_t const                                   m_version;
        String const                                    m_name;
        TVector<ResourceTypeID>                         m_outputTypes;
    };
}
#include "ResourceCompiler.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    CompileContext::CompileContext( FileSystem::Path const& sourceDataDirectoryPath, FileSystem::Path const& compiledResourceDirectoryPath, ResourceID const& resourceToCompile, bool isCompilingForShippingBuild )
        : m_sourceDataDirectoryPath( sourceDataDirectoryPath )
        , m_compiledResourceDirectoryPath( compiledResourceDirectoryPath )
        , m_isCompilingForPackagedBuild( isCompilingForShippingBuild )
        , m_resourceID( resourceToCompile )
    {
        EE_ASSERT( sourceDataDirectoryPath.IsDirectoryPath() && FileSystem::Exists( sourceDataDirectoryPath ) && resourceToCompile.IsValid() );

        // Resolve paths
        const_cast<FileSystem::Path&>( m_inputFilePath ) = resourceToCompile.GetParentResourceFileSystemPath( m_sourceDataDirectoryPath );
        const_cast<FileSystem::Path&>( m_outputFilePath ) = resourceToCompile.GetFileSystemPath( m_compiledResourceDirectoryPath );
    }

    bool CompileContext::IsValid() const
    {
        if ( !m_inputFilePath.IsValid() || !m_outputFilePath.IsValid() || !m_resourceID.IsValid() )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    int32_t Compiler::GetVersion( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& outputType : m_outputTypes )
        {
            if ( outputType.m_typeID == resourceTypeID )
            {
                return Serialization::GetBinarySerializationVersion() + outputType.m_version;
                break;
            }
        }

        EE_UNREACHABLE_CODE();
        return -1;
    }

    bool Compiler::WillGenerateAdditionalDataFile( ResourceTypeID resourceTypeID ) const
    {
        for ( auto const& outputType : m_outputTypes )
        {
            if ( outputType.m_typeID == resourceTypeID )
            {
                return outputType.m_requiresAdditionalDataFile;
            }
        }

        EE_UNREACHABLE_CODE();
        return false;
    }

    void Compiler::Initialize( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath )
    {
        m_pTypeRegistry = &typeRegistry;
        m_sourceDataDirectoryPath = rawResourceDirectoryPath;
    }

    void Compiler::Shutdown()
    {
        m_pTypeRegistry = nullptr;
        m_sourceDataDirectoryPath.Clear();
    }

    CompilationResult Compiler::Error( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        SystemLog::AddEntryVarArgs( Severity::Error, "Resource", m_name.c_str(), __FILE__, __LINE__, pFormat, args);
        va_end( args );
        return CompilationResult::Failure;
    }

    CompilationResult Compiler::Warning( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        SystemLog::AddEntryVarArgs( Severity::Warning, "Resource", m_name.c_str(), __FILE__, __LINE__, pFormat, args );
        va_end( args );
        return CompilationResult::SuccessWithWarnings;
    }

    CompilationResult Compiler::Message( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        SystemLog::AddEntryVarArgs( Severity::Info, "Resource", m_name.c_str(), __FILE__, __LINE__, pFormat, args );
        va_end( args );
        return CompilationResult::Success;
    }

    CompilationResult Compiler::CompilationSucceeded( CompileContext const& ctx ) const
    {
        Message( "Compiled successfully: %s", (char const*) ctx.m_inputFilePath );
        Message( "Output File: %s", (char const*) ctx.m_outputFilePath );
        return CompilationResult::Success;
    }

    CompilationResult Compiler::CompilationSucceededWithWarnings( CompileContext const& ctx ) const
    {
        Warning( "Compiled with warnings: %s", (char const*) ctx.m_inputFilePath );
        Message( "Output File: %s", (char const*) ctx.m_outputFilePath );
        return CompilationResult::SuccessWithWarnings;
    }

    CompilationResult Compiler::CompilationFailed( CompileContext const& ctx ) const
    {
        return Error( "Failed to compile resource: %s", (char const*) ctx.m_outputFilePath );
    }
}
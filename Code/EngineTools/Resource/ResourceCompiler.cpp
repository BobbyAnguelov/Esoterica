#include "ResourceCompiler.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    CompileContext::CompileContext( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& compiledResourceDirectoryPath, ResourceID const& resourceToCompile, bool isCompilingForShippingBuild )
        : m_rawResourceDirectoryPath( rawResourceDirectoryPath )
        , m_compiledResourceDirectoryPath( compiledResourceDirectoryPath )
        , m_isCompilingForPackagedBuild( isCompilingForShippingBuild )
        , m_resourceID( resourceToCompile )
    {
        EE_ASSERT( rawResourceDirectoryPath.IsDirectoryPath() && FileSystem::Exists( rawResourceDirectoryPath ) && resourceToCompile.IsValid() );

        // Resolve paths
        ResourcePath const& resourceToCompilePath = resourceToCompile.GetResourcePath();
        const_cast<FileSystem::Path&>( m_inputFilePath ) = ResourcePath::ToFileSystemPath( rawResourceDirectoryPath, resourceToCompilePath );
        const_cast<FileSystem::Path&>( m_outputFilePath ) = ResourcePath::ToFileSystemPath( m_compiledResourceDirectoryPath, resourceToCompilePath );
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

    void Compiler::Initialize( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath )
    {
        m_pTypeRegistry = &typeRegistry;
        m_rawResourceDirectoryPath = rawResourceDirectoryPath;
    }

    void Compiler::Shutdown()
    {
        m_pTypeRegistry = nullptr;
        m_rawResourceDirectoryPath.Clear();
    }

    CompilationResult Compiler::Error( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        Log::AddEntryVarArgs( Log::Severity::Error, "Resource", m_name.c_str(), __FILE__, __LINE__, pFormat, args);
        va_end( args );
        return CompilationResult::Failure;
    }

    CompilationResult Compiler::Warning( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        Log::AddEntryVarArgs( Log::Severity::Warning, "Resource", m_name.c_str(), __FILE__, __LINE__, pFormat, args );
        va_end( args );
        return CompilationResult::SuccessWithWarnings;
    }

    CompilationResult Compiler::Message( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        Log::AddEntryVarArgs( Log::Severity::Info, "Resource", m_name.c_str(), __FILE__, __LINE__, pFormat, args );
        va_end( args );
        return CompilationResult::Success;
    }

    CompilationResult Compiler::CompilationSucceeded( CompileContext const& ctx ) const
    {
        return Message( "Compiled '%s' to '%s' successfully", (char const*) ctx.m_inputFilePath, (char const*) ctx.m_outputFilePath );
    }

    CompilationResult Compiler::CompilationSucceededWithWarnings( CompileContext const& ctx ) const
    {
        return Warning( "Compiled '%s' to '%s' successfully (with warnings)", (char const*) ctx.m_inputFilePath, (char const*) ctx.m_outputFilePath );
    }

    CompilationResult Compiler::CompilationFailed( CompileContext const& ctx ) const
    {
        return Error( "Failed to compile resource: '%s'", (char const*) ctx.m_outputFilePath );
    }
}
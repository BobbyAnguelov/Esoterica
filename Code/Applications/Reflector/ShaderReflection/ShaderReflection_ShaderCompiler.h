#pragma once
#include "Base/Threading/TaskSystem.h"
#include "Base/Threading/Threading.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/String.h"

#include "d3d12shader.h"
#include "dxcapi.h"
#include <wrl/client.h>

//-------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ReflectedProject;
    class ReflectedShader;

    //-------------------------------------------------------------------------

    class ShaderCompiler
    {
        struct CompilationItem
        {
            ReflectedShader*    m_pShader;
            FileSystem::Path    m_outputPath;
        };

    public:

        ShaderCompiler( FileSystem::Path const& solutionDirectoryPath, TVector<ReflectedProject>& projects, int32_t numCoresToUse );
        ~ShaderCompiler();

        inline bool HasErrors() const { return !m_errorMessage.empty(); }
        inline char const* GetErrorMessage() const { return m_errorMessage.c_str(); }

        inline bool HasWarnings() const { return !m_warnings.empty(); }
        inline TVector<String> const& GetWarnings() const { return m_warnings; }

        bool CompileShaders();

    private:

        void CompilationTask( TaskSetPartition range, uint32_t threadNum );

        bool CompileMaterialShader( ReflectedShader& shader, ComPtr<IDxcCompiler3> &compiler, ComPtr<IDxcIncludeHandler> &includeHandler ) const;
        bool CompileSurfaceShader( ReflectedShader& shader, ComPtr<IDxcCompiler3> &compiler, ComPtr<IDxcIncludeHandler> &includeHandler ) const;
        bool CompileComputeShader( ReflectedShader& shader, ComPtr<IDxcCompiler3> &compiler, ComPtr<IDxcIncludeHandler> &includeHandler ) const;

        bool CompileShaderStage( ReflectedShader& shader, String const& shaderStageID, ComPtr<IDxcCompiler3> &compiler, ComPtr<IDxcIncludeHandler> &includeHandler, TInlineVector<wchar_t const*, 20>& args ) const;

        bool LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;

    private:

        TVector<ReflectedProject>&          m_projects;
        TaskSystem                          m_taskSystem;
        AsyncTask                           m_compilationTask;

        WString                             m_codeDirectoryPath;
        TVector<CompilationItem>            m_shadersToCompile;

        mutable Threading::Mutex            m_mutex;
        mutable String                      m_errorMessage;
        mutable TVector<String>             m_warnings;
    };
}
#pragma once
#include "Applications/Reflector/ReflectedProject.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    struct GeneratedFile;
    class ReflectedShader;

    //-------------------------------------------------------------------------

    class ShaderInputReflector
    {

    public:

        ShaderInputReflector( TVector<ReflectedProject> const& projects );

        inline bool HasErrors() const { return !m_errorMessage.empty(); }
        inline char const* GetErrorMessage() const { return m_errorMessage.c_str(); }

        bool Reflect();

    private:

        void GenerateReflectedInputsForShader( FileSystem::Path const& outputDirectoryPath, ReflectedShader const& shader );
        bool WriteGeneratedFiles();

    private:

        TVector<ReflectedProject> const&    m_projects;
        TVector<GeneratedFile>              m_generatedFiles;

        mutable String                      m_errorMessage;
    };
}
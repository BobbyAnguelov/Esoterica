#pragma once

#include "Applications/Reflector/ReflectedCodeGenerator.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ShaderCodeGenerator : public CodeGenerator
    {

    public:

        using CodeGenerator::CodeGenerator;
        virtual bool GenerateCodeForSolution() override;

    private:

        bool GenerateReflectedParametersFile( ReflectedProject const& project, ReflectedShader const& shader );
        bool GenerateShaderFile( ReflectedProject const& project, ReflectedShader const& shader );
        bool GenerateShaderRegistrationFiles();

        bool GenerateMaterialShaderRegistration( std::stringstream& stream, ReflectedShader const& shader );
        bool GenerateSurfaceShaderRegistration( std::stringstream& stream, ReflectedShader const& shader );
        bool GenerateComputeShaderRegistration( std::stringstream& stream, ReflectedShader const& shader );

        void GenerateXML();
    };
}
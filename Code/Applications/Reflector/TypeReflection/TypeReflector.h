#pragma once

#include "ReflectedSolution.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class TypeInfoReflector
    {

    public:

        TypeInfoReflector( FileSystem::Path const& slnPath );

        bool ParseSolution();
        bool Clean();
        bool Build();

    private:

        bool WriteGeneratedFiles( class CodeGenerator const& generator );

    private:

        ReflectedSolution                   m_solution;
    };
}
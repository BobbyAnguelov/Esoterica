#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class SkeletonCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( SkeletonCompiler );

    public:

        SkeletonCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;
    };
}
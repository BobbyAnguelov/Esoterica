#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IKRigCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( IKRigCompiler );

    public:

        IKRigCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;
    };
}
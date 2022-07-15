#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BoneMaskCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( BoneMaskCompiler );
        static const int32_t s_version = 1;

    public:

        BoneMaskCompiler();

    private:

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;
    };
}
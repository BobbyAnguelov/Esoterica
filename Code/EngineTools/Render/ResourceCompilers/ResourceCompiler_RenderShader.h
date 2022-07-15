#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"
#include "System/Render/RenderShader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class ShaderCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( ShaderCompiler );
        static const int32_t s_version = 1;

    public:

        ShaderCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
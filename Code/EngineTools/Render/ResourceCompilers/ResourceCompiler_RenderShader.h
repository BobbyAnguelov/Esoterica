#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"
#include "Base/Render/RenderShader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class ShaderCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( ShaderCompiler );
        static const int32_t s_version = 1;

    public:

        ShaderCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class DecalMaterialCompiler final : public Resource::Compiler
    {
        EE_REFLECT_TYPE( DecalMaterialCompiler );

    public:

        DecalMaterialCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override final;
    };
}

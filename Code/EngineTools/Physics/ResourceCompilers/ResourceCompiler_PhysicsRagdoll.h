#pragma  once

#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class RagdollCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( RagdollCompiler );

    public:

        RagdollCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
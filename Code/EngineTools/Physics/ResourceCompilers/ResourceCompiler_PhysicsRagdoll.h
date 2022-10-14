#pragma  once

#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class RagdollCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( RagdollCompiler );
        static const int32_t s_version = 5;

    public:

        RagdollCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
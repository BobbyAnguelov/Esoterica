#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsMaterialDatabaseCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( PhysicsMaterialDatabaseCompiler );
        static const int32_t s_version = 0;

    public:

        PhysicsMaterialDatabaseCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
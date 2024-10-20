#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsMaterialDatabaseCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( PhysicsMaterialDatabaseCompiler );

    public:

        PhysicsMaterialDatabaseCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
    };
}
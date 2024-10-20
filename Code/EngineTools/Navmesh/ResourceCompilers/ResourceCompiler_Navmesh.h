#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( NavmeshCompiler );

    public:

        NavmeshCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual bool IsInputFileRequired() const override { return false; }
        virtual bool RequiresAdvancedUpToDateCheck( ResourceTypeID resourceTypeID ) const override { return true; }
        virtual uint64_t CalculateAdvancedUpToDateHash( ResourceID resourceID ) const override;

    private:

        Resource::CompilationResult GenerateNavmesh( Resource::CompileContext const& ctx, bool updatePregeneratedNavmesh ) const;
    };
}
#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityMapCompiler final : public Resource::Compiler
    {
        EE_REFLECT_TYPE( EntityMapCompiler );

    public:

        EntityMapCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;
    };
}

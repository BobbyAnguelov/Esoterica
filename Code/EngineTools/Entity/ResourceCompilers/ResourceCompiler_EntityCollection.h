#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityCollectionCompiler final : public Resource::Compiler
    {
        EE_REGISTER_TYPE( EntityCollectionCompiler );
        static const int32_t s_version = 7;

    public:

        EntityCollectionCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;
    };
}

#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"
#include "System/Render/RenderTexture.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class MaterialCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( MaterialCompiler );
        static const int32_t s_version = 1;

    public:

        MaterialCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;
    };
}
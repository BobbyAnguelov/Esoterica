#pragma once

#include "EngineTools/Resource/ResourceCompiler.h"
#include "Base/Render/RenderTexture.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class MaterialCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( MaterialCompiler );

    public:

        MaterialCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;
    };
}
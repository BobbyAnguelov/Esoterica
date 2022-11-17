#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ToolsGraphDefinition;
    class GraphDataSet;

    //-------------------------------------------------------------------------

    class AnimationGraphCompiler final : public Resource::Compiler
    {
        EE_REGISTER_TYPE( AnimationGraphCompiler );
        constexpr static const int32_t s_version = 16 + GraphDefinitionCompiler::s_version;

    public:

        AnimationGraphCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;
        virtual bool GetReferencedResources( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;

    private:

        virtual bool IsInputFileRequired() const override { return false; }

        bool TryToGenerateAnimGraphVariationFile( Resource::CompileContext const& ctx ) const;
        Resource::CompilationResult CompileGraphDefinition( Resource::CompileContext const& ctx ) const;
        Resource::CompilationResult CompileGraphVariation( Resource::CompileContext const& ctx ) const;
        bool LoadAndCompileGraph( FileSystem::Path const& graphFilePath, ToolsGraphDefinition& editorGraph, GraphDefinitionCompiler& definitionCompiler ) const;
        bool GenerateDataSet( Resource::CompileContext const& ctx, ToolsGraphDefinition const& editorGraph, TVector<UUID> const& registeredDataSlots, GraphDataSet& dataSet ) const;
    };
}
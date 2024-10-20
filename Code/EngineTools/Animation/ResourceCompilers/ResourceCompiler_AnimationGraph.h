#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct GraphResourceDescriptor;
    class GraphDataSet;

    //-------------------------------------------------------------------------

    class AnimationGraphCompiler final : public Resource::Compiler
    {
        EE_REFLECT_TYPE( AnimationGraphCompiler );

    public:

        AnimationGraphCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;
        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;

    private:

        virtual bool IsInputFileRequired() const override { return false; }

        Resource::CompilationResult CompileGraphDefinition( Resource::CompileContext const& ctx ) const;
        Resource::CompilationResult CompileGraphVariation( Resource::CompileContext const& ctx ) const;
        bool LoadAndCompileGraph( FileSystem::Path const& graphFilePath, GraphResourceDescriptor& graphDescriptor, GraphDefinitionCompiler& definitionCompiler ) const;
        bool GenerateDataSet( Resource::CompileContext const& ctx, ToolsGraphDefinition const& editorGraph, TVector<UUID> const& registeredDataSlots, GraphDataSet& dataSet ) const;
    };
}
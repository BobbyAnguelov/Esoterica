#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EditorGraphDefinition;
    class GraphDefinitionCompiler;

    //-------------------------------------------------------------------------

    class AnimationGraphCompiler final : public Resource::Compiler
    {
        EE_REGISTER_TYPE( AnimationGraphCompiler );
        static const int32_t s_version = 7;

    public:

        AnimationGraphCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;
        virtual bool GetReferencedResources( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;

    private:

        Resource::CompilationResult CompileGraphDefinition( Resource::CompileContext const& ctx ) const;
        Resource::CompilationResult CompileGraphVariation( Resource::CompileContext const& ctx ) const;
        bool LoadAndCompileGraph( FileSystem::Path const& graphFilePath, EditorGraphDefinition& editorGraph, GraphDefinitionCompiler& definitionCompiler ) const;
        bool GenerateVirtualDataSetResource( Resource::CompileContext const& ctx, EditorGraphDefinition const& editorGraph, TVector<UUID> const& registeredDataSlots, StringID const& variationID, ResourcePath const& dataSetPath ) const;
    };
}
#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ToolsGraphDefinition;
    class GraphCompilationContext;

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API GraphResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( GraphResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return GraphDefinition::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourcePath>& outDependencies ) override {}
        virtual void Clear() override {}

        // Flat list of all variations present
        EE_REFLECT( "IsToolsReadOnly" : true );
        TVector<StringID>           m_variations;
    };
}
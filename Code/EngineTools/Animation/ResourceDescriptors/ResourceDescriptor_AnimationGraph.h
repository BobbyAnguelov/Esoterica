#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EditorGraphDefinition;
    class GraphCompilationContext;

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API GraphResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( GraphResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return GraphDefinition::GetStaticResourceTypeID(); }
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API GraphVariationResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( GraphVariationResourceDescriptor );

    public:

        EE_EXPOSE ResourcePath                 m_graphPath;
        EE_EXPOSE StringID                     m_variationID; // Optional: if not set, will use the default variation
    };
}
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
        EE_REGISTER_TYPE( GraphResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return GraphDefinition::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override {}
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API GraphVariationResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( GraphVariationResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return false; }
        
        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_graphPath.IsValid() )
            {
                outDependencies.emplace_back( m_graphPath );
            }
        }

    public:

        EE_REGISTER ResourcePath                 m_graphPath;
        EE_REGISTER StringID                     m_variationID; // Optional: if not set, will use the default variation
    };
}
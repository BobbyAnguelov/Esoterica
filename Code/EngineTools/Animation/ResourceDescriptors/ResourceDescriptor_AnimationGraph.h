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
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API GraphVariationResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( GraphVariationResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return false; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return GraphVariation::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourcePath>& outDependencies ) override
        {
            if ( m_graphPath.IsValid() )
            {
                outDependencies.emplace_back( m_graphPath );
            }
        }

        virtual void Clear() override
        {
            m_graphPath.Clear();
            m_variationID.Clear();
        }

    public:

        EE_REFLECT( "IsToolsReadOnly" : true )
        ResourcePath                 m_graphPath;

        EE_REFLECT( "IsToolsReadOnly" : true )
        StringID                     m_variationID; // Optional: if not set, will use the default variation
    };
}
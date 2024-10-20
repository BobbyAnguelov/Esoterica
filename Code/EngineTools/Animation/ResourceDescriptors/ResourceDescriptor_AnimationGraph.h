#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API GraphResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( GraphResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return GraphDefinition::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return GraphDefinition::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return GraphDefinition::s_friendlyName; }
        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override {}
        virtual void Clear() override {}

    public:

        EE_REFLECT( Hidden );
        ToolsGraphDefinition m_graphDefinition;
    };
}
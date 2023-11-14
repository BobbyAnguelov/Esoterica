#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EE_ENGINETOOLS_API EntityMapDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( EntityMapDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return false; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return SerializedEntityMap::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourcePath>& outDependencies ) override {}
        virtual void Clear() override {}
    };
}
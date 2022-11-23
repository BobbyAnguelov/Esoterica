#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EE_ENGINETOOLS_API EntityCollectionDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( EntityCollectionDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return false; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return SerializedEntityCollection::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override {}
    };
}
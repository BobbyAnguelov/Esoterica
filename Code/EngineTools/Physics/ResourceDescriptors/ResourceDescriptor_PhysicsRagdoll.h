#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API RagdollResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( RagdollResourceDescriptor );

    public:

        virtual bool IsValid() const override { return m_skeleton.IsSet(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return RagdollDefinition::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override {}

    public:

        EE_REFLECT()    TResourcePtr<Animation::Skeleton>             m_skeleton;
        EE_REFLECT()    RagdollDefinition                             m_definition;
    };
}
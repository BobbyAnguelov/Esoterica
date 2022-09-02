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
        EE_REGISTER_TYPE( RagdollResourceDescriptor );
    
    public:

        virtual bool IsValid() const override { return m_skeleton.IsSet(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return RagdollDefinition::GetStaticResourceTypeID(); }

    public:

        EE_EXPOSE   TResourcePtr<Animation::Skeleton>             m_skeleton;
        EE_REGISTER RagdollDefinition                             m_definition;
    };
}
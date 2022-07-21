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
    
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return RagdollDefinition::GetStaticResourceTypeID(); }

    public:

        EE_EXPOSE RagdollDefinition    m_definition;
    };
}
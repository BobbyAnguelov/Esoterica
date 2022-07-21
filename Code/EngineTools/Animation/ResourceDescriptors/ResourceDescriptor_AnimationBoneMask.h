#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API BoneMaskResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( BoneMaskResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return BoneMaskDefinition::GetStaticResourceTypeID(); }

    public:

        EE_EXPOSE TResourcePtr<Skeleton>                   m_pSkeleton = nullptr;
        EE_EXPOSE TVector<float>                           m_boneWeights;
    };
}
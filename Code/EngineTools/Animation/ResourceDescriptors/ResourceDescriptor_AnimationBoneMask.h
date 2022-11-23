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

        virtual bool IsValid() const override { return m_skeleton.IsSet(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return BoneMaskDefinition::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if( m_skeleton.IsSet() )
            {
                outDependencies.emplace_back( m_skeleton.GetResourceID() );
            }
        }

    public:

        EE_EXPOSE TResourcePtr<Skeleton>                    m_skeleton = nullptr;
        EE_REGISTER TVector<float>                          m_boneWeights;
    };
}
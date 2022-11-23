#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API AnimationClipResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( AnimationClipResourceDescriptor );

        virtual bool IsValid() const override { return m_skeleton.IsSet() && m_animationPath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return AnimationClip::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_skeleton.IsSet() )
            {
                outDependencies.emplace_back( m_skeleton.GetResourceID() );
            }

            if ( m_animationPath.IsValid() )
            {
                outDependencies.emplace_back( m_animationPath );
            }
        }

    public:

        EE_EXPOSE ResourcePath                m_animationPath;
        EE_EXPOSE TResourcePtr<Skeleton>      m_skeleton = nullptr;
        EE_EXPOSE String                      m_animationName; // Optional: if not set, will use the first animation in the file
        EE_EXPOSE bool                        m_regenerateRootMotion = false; // Force regeneration of root motion track from the specified bone
        EE_EXPOSE bool                        m_rootMotionGenerationRestrictToHorizontalPlane = false; // Ensure that the root motion has no vertical motion
        EE_EXPOSE StringID                    m_rootMotionGenerationBoneID;
        EE_EXPOSE EulerAngles                 m_rootMotionGenerationPreRotation;
        EE_EXPOSE bool                        m_generateTestAdditive = false; // This is to generate an additive pose (based on the reference pose) so that we can test the rest of the code (remove once we have a proper additive import pipeline)
        EE_EXPOSE IntRange                    m_limitFrameRange;
    };
}
#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Animation/Events/AnimationEventTimeline.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API AnimationClipResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( AnimationClipResourceDescriptor );

        enum class AdditiveType
        {
            EE_REFLECT_ENUM

            None,
            RelativeToSkeleton,
            RelativeToFrame,
            RelativeToAnimationClip
        };

        struct SecondaryAnimationDescriptor : public IReflectedType
        {
            EE_REFLECT_TYPE( SecondaryAnimationDescriptor );

            EE_REFLECT();
            DataPath                m_animationPath;

            EE_REFLECT();
            TResourcePtr<Skeleton>      m_skeleton = nullptr;
        };

    public:

        virtual bool IsValid() const override { return m_skeleton.IsSet() && m_animationPath.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return AnimationClip::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return AnimationClip::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return AnimationClip::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            if ( m_skeleton.IsSet() )
            {
                outDependencies.emplace_back( m_skeleton.GetResourcePath() );
            }

            if ( m_animationPath.IsValid() )
            {
                outDependencies.emplace_back( m_animationPath );
            }
        }

        virtual void Clear() override
        {
            m_animationPath.Clear();
            m_skeleton = nullptr;
            m_animationName.clear();
            m_limitFrameRange.Clear();
            m_regenerateRootMotion = false;
            m_rootMotionGenerationRestrictToHorizontalPlane = false;
            m_rootMotionGenerationBoneID.Clear();
            m_rootMotionGenerationPreRotation = EulerAngles();
            m_additiveType = AdditiveType::None;
            m_additiveBaseAnimation = nullptr;
            m_additiveBaseFrameIndex = 0;
            m_secondaryAnimations.clear();
        }

    public:

        EE_REFLECT();
        DataPath                                m_animationPath;

        EE_REFLECT();
        TResourcePtr<Skeleton>                  m_skeleton = nullptr;

        // Optional: if not set, will use the first animation in the file
        EE_REFLECT();
        String                                  m_animationName;

        EE_REFLECT();
        IntRange                                m_limitFrameRange;

        // Force regeneration of root motion track from the specified bone
        EE_REFLECT( Category = "Root Motion" );
        bool                                    m_regenerateRootMotion = false;

        // Ensure that the root motion has no vertical motion
        EE_REFLECT( Category = "Root Motion" );
        bool                                    m_rootMotionGenerationRestrictToHorizontalPlane = false;

        EE_REFLECT( Category = "Root Motion" );
        StringID                                m_rootMotionGenerationBoneID;

        EE_REFLECT( Category = "Root Motion" );
        EulerAngles                             m_rootMotionGenerationPreRotation;

        // Additive
        //-------------------------------------------------------------------------

        // This is to generate an additive pose (based on the reference pose) so that we can test the rest of the code (remove once we have a proper additive import pipeline)
        EE_REFLECT( Category = "Additive" );
        AdditiveType                            m_additiveType = AdditiveType::None;

        // The animation to use as the the base for the additive animation
        EE_REFLECT( Category = "Additive" );
        TResourcePtr<AnimationClip>             m_additiveBaseAnimation = nullptr;

        // The frame to use as the base for the additive animation
        EE_REFLECT( Category = "Additive" );
        uint32_t                                m_additiveBaseFrameIndex = 0;

        // Child Animation
        //-------------------------------------------------------------------------
        
        EE_REFLECT( Category = "Secondary Animations" );
        TVector<SecondaryAnimationDescriptor>   m_secondaryAnimations;

        // Events
        //-------------------------------------------------------------------------

        EE_REFLECT( Hidden );
        EventTimeline                           m_events;
    };
}
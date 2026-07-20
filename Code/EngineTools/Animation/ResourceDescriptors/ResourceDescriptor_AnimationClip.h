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

        enum class DurationOverride
        {
            EE_REFLECT_ENUM

            None,
            Multiplier,
            FixedValue,
        };

    public:

        virtual bool IsValid() const override { return m_skeleton.IsSet() && m_animationPath.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return AnimationClip::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return AnimationClip::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return AnimationClip::s_friendlyName; }

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override;
        virtual void GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const override;

        virtual void Clear() override;

    public:

        EE_REFLECT();
        DataPath                                m_animationPath;

        EE_REFLECT();
        TResourcePtr<Skeleton>                  m_skeleton = nullptr;

        EE_REFLECT();
        TVector<TResourcePtr<Skeleton>>         m_secondarySkeletons;

        // Optional: if not set, will use the first animation in the file
        EE_REFLECT();
        String                                  m_animationName;

        EE_REFLECT( Category = "Import Modifiers" );
        IntRange                                m_limitFrameRange;

        EE_REFLECT( Category = "Import Modifiers" );
        DurationOverride                        m_durationOverride = DurationOverride::None;

        EE_REFLECT( Category = "Import Modifiers" );
        float                                   m_durationOverrideValue = 1.0f;

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

        EE_REFLECT( Category = "Advanced" );
        TVector<StringID>                       m_bonesToSampleInModelSpace;

        // Additive
        //-------------------------------------------------------------------------

        // This is to generate an additive pose (based on the reference pose) so that we can test the rest of the code (remove once we have a proper additive import pipeline)
        EE_REFLECT( Category = "Additive" );
        AdditiveType                            m_additiveType = AdditiveType::None;

        // The animation to use as the the base for the additive animation
        EE_REFLECT( Category = "Additive" );
        DataPath                                m_additiveBaseAnimationPath;

        // The frame to use as the base for the additive animation
        EE_REFLECT( Category = "Additive" );
        int32_t                                 m_additiveBaseFrameIndex = InvalidIndex;

        // Events
        //-------------------------------------------------------------------------

        EE_REFLECT( Hidden );
        EventTimeline                           m_events;
    };
}
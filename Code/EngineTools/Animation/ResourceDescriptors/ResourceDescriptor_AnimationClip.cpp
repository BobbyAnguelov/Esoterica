#include "ResourceDescriptor_AnimationClip.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AnimationClipResourceDescriptor::Clear()
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

    //-------------------------------------------------------------------------

    class ClipDescriptorEditingRules : public PG::TTypeEditingRules<AnimationClipResourceDescriptor>
    {
        using PG::TTypeEditingRules<AnimationClipResourceDescriptor>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            static StringID const baseAnimPropertyID( "m_additiveBaseAnimation" );
            if ( propertyID == baseAnimPropertyID )
            {
                if ( m_pTypeInstance->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip )
                {
                    return HiddenState::Hidden;
                }
            }

            static StringID const baseFramePropertyID( "m_additiveBaseFrameIndex" );
            if ( propertyID == baseFramePropertyID )
            {
                if ( m_pTypeInstance->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame )
                {
                    return HiddenState::Hidden;
                }
            }

            static StringID const durationOverrideValuePropertyID( "m_durationOverrideValue" );
            if ( propertyID == durationOverrideValuePropertyID )
            {
                if ( m_pTypeInstance->m_durationOverride == AnimationClipResourceDescriptor::DurationOverride::None )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( ClipDescriptorEditingRulesFactory, AnimationClipResourceDescriptor, ClipDescriptorEditingRules );
}

#include "ResourceDescriptor_AnimationClip.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ClipDescriptorEditingRules : public PG::TTypeEditingRules<AnimationClipResourceDescriptor>
    {
        using PG::TTypeEditingRules<AnimationClipResourceDescriptor>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            StringID const baseAnimPropertyID( "m_additiveBaseAnimation" );
            if ( propertyID == baseAnimPropertyID )
            {
                if ( m_pTypeInstance->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip )
                {
                    return HiddenState::Hidden;
                }
            }

            StringID const baseFramePropertyID( "m_additiveBaseFrameIndex" );
            if ( propertyID == baseFramePropertyID )
            {
                if ( m_pTypeInstance->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( ClipDescriptorEditingRulesFactory, AnimationClipResourceDescriptor, ClipDescriptorEditingRules );
}

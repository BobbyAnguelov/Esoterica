#include "ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Core/PropertyGrid/PropertyGridTypeEditingRules.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ClipDescriptorEditingRules : public PG::TTypeEditingRules<AnimationClipResourceDescriptor>
    {
        using PG::TTypeEditingRules<AnimationClipResourceDescriptor>::TTypeEditingRules;

        virtual bool IsReadOnly( StringID const& propertyID ) override
        {
            return false;
        }

        virtual bool IsHidden( StringID const& propertyID ) override
        {
            StringID const baseAnimPropertyID( "m_additiveBaseAnimation" );
            if ( propertyID == baseAnimPropertyID )
            {
                return m_pTypeInstance->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::RelativeToAnimationClip;
            }

            StringID const baseFramePropertyID( "m_additiveBaseFrameIndex" );
            if ( propertyID == baseFramePropertyID )
            {
                return m_pTypeInstance->m_additiveType != AnimationClipResourceDescriptor::AdditiveType::RelativeToFrame;
            }

            return false;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( ClipDescriptorEditingRulesFactory, AnimationClipResourceDescriptor, ClipDescriptorEditingRules );
}
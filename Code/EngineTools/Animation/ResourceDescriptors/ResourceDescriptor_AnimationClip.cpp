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
        m_secondarySkeletons.clear();
        m_animationName.clear();
        m_limitFrameRange.Clear();
        m_regenerateRootMotion = false;
        m_rootMotionGenerationRestrictToHorizontalPlane = false;
        m_rootMotionGenerationBoneID.Clear();
        m_rootMotionGenerationPreRotation = EulerAngles();
        m_additiveType = AdditiveType::None;
        m_additiveBaseFrameIndex = 0;
    }

    void AnimationClipResourceDescriptor::GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const
    {
        if ( m_skeleton.IsSet() )
        {
            outDependencies.emplace_back( m_skeleton.GetDataPath(), true );
        }

        for ( auto const& skeleton : m_secondarySkeletons )
        {
            if ( skeleton.IsSet() )
            {
                outDependencies.emplace_back( skeleton.GetDataPath(), true );
            }
        }

        if ( m_animationPath.IsValid() )
        {
            outDependencies.emplace_back( m_animationPath, false );
        }
    }

    void AnimationClipResourceDescriptor::GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const
    {
        outDependencies.emplace_back( m_skeleton.GetResourceID() );

        // Add secondary animation skeletons as install dependencies
        for ( auto const& secondarySkeleton : m_secondarySkeletons )
        {
            if ( secondarySkeleton.IsSet() )
            {
                VectorEmplaceBackUnique( outDependencies, secondarySkeleton.GetResourceID() );
            }
        }

        // Try read event tracks
        //-------------------------------------------------------------------------

        for ( TTypeInstance<Timeline::Track> const& track : m_events.GetTracks() )
        {
            for ( TTypeInstance<Timeline::TrackItem> const& item : track->GetItems() )
            {
                EventTrackItem const* pEventItem = Cast<EventTrackItem>( item.Get() );
                Event const* pEvent = pEventItem->GetEvent();
                pEvent->GetTypeInfo()->GetReferencedResources( pEvent, outDependencies );
            }
        }
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

    EE_PROPERTY_GRID_EDITING_RULES( AnimationClipResourceDescriptor, ClipDescriptorEditingRules );
}

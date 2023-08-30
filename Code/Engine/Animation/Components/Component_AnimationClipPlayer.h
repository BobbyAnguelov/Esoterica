#pragma once

#include "Engine/Animation/AnimationClip.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API AnimationClipPlayerComponent final : public EntityComponent
    {
        EE_ENTITY_COMPONENT( AnimationClipPlayerComponent );

    public:

        enum class PlayMode
        {
            EE_REFLECT_ENUM

            PlayOnce,
            Loop,
            Posed
        };

    public:

        inline AnimationClipPlayerComponent() = default;
        inline AnimationClipPlayerComponent( StringID name ) : EntityComponent( name ) {}

        void Update( Seconds deltaTime, Transform const& characterTransform );

        //-------------------------------------------------------------------------

        inline bool HasAnimationSet() const { return m_pAnimation != nullptr; }
        Pose const* GetPose() const { return m_pPose; }

        // Does this component require a manual update via a custom entity system?
        inline bool RequiresManualUpdate() const { return m_requiresManualUpdate; }

        // Should we apply the root motion delta automatically to the character once we evaluate the graph 
        // (Note: only works if we dont require a manual update)
        inline bool ShouldApplyRootMotionToEntity() const { return m_applyRootMotionToEntity; }

        // Gets the root motion delta for the last update (Note: this delta is in character space!)
        inline Transform const& GetRootMotionDelta() const { return m_rootMotionDelta; }

        // Skeleton
        //-------------------------------------------------------------------------

        Skeleton const* GetSkeleton() const;
        void SetSkeletonLOD( Skeleton::LOD lod ) { m_skeletonLOD = lod; }

        //-------------------------------------------------------------------------

        // This function will change the animation resource currently played! Note: this can only be called for unloaded components
        void SetAnimation( ResourceID animationResourceID );

        inline Percentage GetAnimTime() const { return m_animTime; }
        inline ResourceID const& GetAnimationID() const { return m_pAnimation->GetResourceID(); }

        // Play Controls
        //-------------------------------------------------------------------------

        inline bool IsPosed() const { return m_playMode == PlayMode::Posed; }

        // Set's the play mode for this component. Can be called at any time
        void SetPlayMode( PlayMode mode );

        // This is the preferred method for setting the animation time for the component. Can be called at any time
        void SetAnimTime( Percentage inTime );

        // This is a helper that automatically converts seconds to percentage and sets the time! Note: this can only be called for initialized components
        void SetAnimTime( Seconds inTime );

    protected:

        virtual void Initialize() override;
        virtual void Shutdown() override;

    private:

        EE_REFLECT();
        TResourcePtr<AnimationClip>             m_pAnimation = nullptr;

        EE_REFLECT();
        PlayMode                                m_playMode = PlayMode::Loop;

        EE_REFLECT();
        bool                                    m_requiresManualUpdate = false; // Does this component require a manual update via a custom entity system?

        EE_REFLECT();
        bool                                    m_applyRootMotionToEntity = false; // Should we apply the root motion delta automatically to the character once we evaluate the graph. (Note: only works if we dont require a manual update)

        //-------------------------------------------------------------------------

        Skeleton::LOD                           m_skeletonLOD = Skeleton::LOD::High;
        Pose*                                   m_pPose = nullptr;
        Percentage                              m_previousAnimTime = Percentage( 0.0f );
        Percentage                              m_animTime = Percentage( 0.0f );
        Transform                               m_rootMotionDelta = Transform::Identity;
    };
}
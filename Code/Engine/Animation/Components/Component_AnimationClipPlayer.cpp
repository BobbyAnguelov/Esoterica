#include "Component_AnimationClipPlayer.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityLog.h"
#include "Base/Profiling.h"


//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AnimationClipPlayerComponent::SetAnimation( ResourceID animationResourceID )
    {
        EE_ASSERT( IsUnloaded() );
        EE_ASSERT( animationResourceID.IsValid() );
        m_pAnimation = animationResourceID;
    }

    void AnimationClipPlayerComponent::SetPlayMode( PlayMode mode )
    {
        m_playMode = mode;
        m_previousAnimTime = Percentage( -1 );
    }

    void AnimationClipPlayerComponent::SetAnimTime( Percentage inTime )
    {
        m_animTime = inTime.GetClamped( m_playMode == PlayMode::Loop );
    }

    void AnimationClipPlayerComponent::SetAnimTime( Seconds inTime )
    {
        if ( !IsInitialized() )
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Anim Clip Player", "Trying to set anim time on an uninitialized component!" );
            return;
        }

        if ( m_pAnimation == nullptr )
        {
            EE_LOG_ENTITY_ERROR( this, "Animation", "Anim Clip Player", "Trying to set anim time on a player with no animation set!" );
            return;
        }

        EE_ASSERT( m_pAnimation.IsLoaded() );
        Percentage const percentage( inTime / m_pAnimation->GetDuration() );
        SetAnimTime( percentage );
    }

    //-------------------------------------------------------------------------

    void AnimationClipPlayerComponent::Initialize()
    {
        EntityComponent::Initialize();

        if ( m_pAnimation != nullptr )
        {
            EE_ASSERT( m_pAnimation.IsLoaded() );
            m_pPose = EE::New<Pose>( m_pAnimation->GetSkeleton() );

            for ( auto pSecondaryAnimation : m_pAnimation->GetSecondaryAnimations() )
            {
                m_secondaryPoses.emplace_back( EE::New<Pose>( pSecondaryAnimation->GetSkeleton() ) );
            }
        }
    }

    void AnimationClipPlayerComponent::Shutdown()
    {
        EE::Delete( m_pPose );

        for ( auto pSecondaryPose : m_secondaryPoses )
        {
            EE::Delete( pSecondaryPose );
        }
        m_secondaryPoses.clear();

        m_previousAnimTime = -1.0f;
        EntityComponent::Shutdown();
    }

    //-------------------------------------------------------------------------

    void AnimationClipPlayerComponent::Update( Seconds deltaTime, Transform const& characterTransform )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        
        if ( m_pAnimation == nullptr )
        {
            return;
        }

        EE_ASSERT( m_pPose != nullptr );

        //-------------------------------------------------------------------------

        bool bShouldUpdate = false;

        switch ( m_playMode )
        {
            case PlayMode::Loop:
            {
                m_previousAnimTime = m_animTime;
                m_animTime += Percentage( deltaTime / m_pAnimation->GetDuration() );
                m_animTime = m_animTime.GetClamped( true );
                bShouldUpdate = true;
            }
            break;

            case PlayMode::PlayOnce:
            {
                if ( m_previousAnimTime < 1.0f )
                {
                    m_previousAnimTime = m_animTime;
                    m_animTime += Percentage( deltaTime / m_pAnimation->GetDuration() );
                    m_animTime = m_animTime.GetClamped( false );
                    bShouldUpdate = true;
                }
            }
            break;

            case PlayMode::Posed:
            {
                if ( m_previousAnimTime != m_animTime )
                {
                    m_previousAnimTime = m_animTime;
                    bShouldUpdate = true;
                }
            }
            break;
        }

        //-------------------------------------------------------------------------

        if ( bShouldUpdate )
        {
            // Sample primary pose
            //-------------------------------------------------------------------------

            m_pAnimation->GetPose( m_animTime, m_pPose, m_skeletonLOD );

            // No point displaying a pile of bones, so display an additive on top of the reference pose
            if ( m_pPose->IsAdditivePose() )
            {
                Blender::ApplyAdditiveToReferencePose( m_skeletonLOD, m_pPose, 1.0f, nullptr, m_pPose );
            }

            m_pPose->CalculateModelSpaceTransforms();

            // Sample secondary animations
            //-------------------------------------------------------------------------

            int32_t const numChildAnimations = m_pAnimation->GetNumSecondaryAnimations();
            for ( int32_t i = 0; i < numChildAnimations; i++ )
            {
                m_pAnimation->GetSecondaryAnimations()[i]->GetPose( m_animTime, m_secondaryPoses[i], m_skeletonLOD );
                m_secondaryPoses[i]->CalculateModelSpaceTransforms();
            }

            // Sample root motion
            //-------------------------------------------------------------------------

            m_rootMotionDelta = m_pAnimation->GetRootMotionDelta( m_previousAnimTime, m_animTime );
        }
        else // Clear the root motion delta
        {
            m_rootMotionDelta = Transform::Identity;
        }
    }
}
#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"
#include "Engine/Animation/Events/AnimationEvent_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    enum class CharacterAnimationState : uint8_t
    {
        Locomotion = 0,
        InAir,
        Ability,
        Interaction,

        GhostMode,
        NumStates
    };

    //-------------------------------------------------------------------------

    class AnimationController final : public Animation::GraphController
    {

    public:

        AnimationController( Animation::GraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent );

        virtual void PostGraphUpdate( Seconds deltaTime ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetName() const { return "Player Graph Controller"; }
        #endif

        // General Player State
        //-------------------------------------------------------------------------

        void SetCharacterState( CharacterAnimationState state );

        // Generic Transition Info
        //-------------------------------------------------------------------------

        inline bool HasTransitionMarker() const { return m_hasTransitionMarker; }
        inline bool IsAnyTransitionAllowed() const { return IsTransitionFullyAllowed() || IsTransitionConditionallyAllowed(); }
        inline bool IsTransitionFullyAllowed() const { return m_transitionMarker == Animation::TransitionMarker::AllowTransition; }
        inline bool IsTransitionConditionallyAllowed() const { return m_transitionMarker == Animation::TransitionMarker::ConditionallyAllowTransition; }

    private:

        ControlParameter<StringID>      m_characterStateParam = "CharacterState";
        Animation::TransitionMarker     m_transitionMarker = Animation::TransitionMarker::BlockTransition;
        bool                            m_hasTransitionMarker = false;
    };
}
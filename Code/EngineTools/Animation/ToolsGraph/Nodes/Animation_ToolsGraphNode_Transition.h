#pragma once
#include "Animation_ToolsGraphNode.h"
#include "EngineTools/NodeGraph/NodeGraph_StateMachineGraph.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "Base/Math/Easing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GlobalTransitionsEditorGraph;
    class StateMachineGraph;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    // The result node for a transition
    class TransitionToolsNode : public ResultToolsNode
    {
        friend class StateMachineToolsNode;
        friend class TransitionEditingRules;
        EE_REFLECT_TYPE( TransitionToolsNode );

        enum class TimeMatchMode
        {
            EE_REFLECT_ENUM

            None,

            Synchronized,

            MatchSourceSyncEventIndex,
            MatchSourceSyncEventPercentage,
            MatchSourceSyncEventIndexAndPercentage,

            MatchSyncEventID,
            MatchClosestSyncEventID,
            MatchSyncEventIDAndPercentage,
            MatchClosestSyncEventIDAndPercentage,

            MatchTimeInSeconds,
            OffsetTimeInSeconds,
        };

    public:

        TransitionToolsNode();

        virtual bool IsRenameable() const override final { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }

        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual char const* GetCategory() const override { return "Transitions"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionConduit ); }
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual Color GetTitleBarColor() const override;
        virtual Color GetHighlightOutlineColor( NodeGraph::UserContext* pUserContext ) const override;
        virtual int16_t Compile( GraphCompilationContext& context ) const override { EE_UNREACHABLE_CODE(); return InvalidIndex; }

        virtual UUID GetEndStateID() const;

    protected:

        // Should we use a easing mode for the blend weight calculation? Go to http://easings.net as a reference for all the curves.
        EE_REFLECT();
        Math::Easing::Operation                       m_blendWeightEasing = Math::Easing::Operation::Linear;

        // How should we blend the root motion for this transition
        EE_REFLECT();
        RootMotionBlendMode                           m_rootMotionBlend = RootMotionBlendMode::Blend;

        // How long should this transition take?
        EE_REFLECT();
        Seconds                                       m_duration = 0.2f;

        // Should we clamp the transition duration to the estimated remaining time left in the source state (prevent looping of source states)
        EE_REFLECT();
        bool                                          m_clampDurationToSource = false;

        // Can this transition be force started, needed when you want to emergency transition back to your original state
        EE_REFLECT();
        bool                                          m_canBeForced = false;

        // How should we determine the start time of the target node?
        EE_REFLECT();
        TimeMatchMode                                 m_timeMatchMode = TimeMatchMode::None;

        // Time offset to apply to target node start time - with no time match mode it will start the target node at the specified offset
        // This is either a sync event offset (index & percent) or a time in seconds
        EE_REFLECT();
        float                                         m_timeOffset = 0.0f;

        // How long do we take to blend to the masked target before blending out the bone mask (a percentage of the transition duration)
        EE_REFLECT();
        Percentage                                    m_boneMaskBlendInTimePercentage = 0.33f; 
    };

    //-------------------------------------------------------------------------

    class TransitionConduitToolsNode final : public NodeGraph::TransitionConduitNode
    {
        EE_REFLECT_TYPE( TransitionConduitToolsNode );

    public:

        TransitionConduitToolsNode();
        TransitionConduitToolsNode( NodeGraph::StateNode const* pStartState, NodeGraph::StateNode const* pEndState );

        virtual bool HasTransitions() const override;

        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual Color GetConduitColor( NodeGraph::UserContext* pUserContext ) const override;
        virtual void PreDrawUpdate( NodeGraph::UserContext* pUserContext ) override;

    private:

        bool isAnyChildActive = false;
    };
}
#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Math/Easing.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GlobalTransitionsEditorGraph;
    class StateMachineGraph;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    // The result node for a transition
    class TransitionToolsNode : public FlowToolsNode
    {
        friend class StateMachineToolsNode;
        EE_REGISTER_TYPE( TransitionToolsNode );

        enum class TimeMatchMode
        {
            EE_REGISTER_ENUM

            None,
            Synchronized,
            MatchSourceSyncEventIndexOnly,
            MatchSourceSyncEventIndexAndPercentage,
            MatchSourceSyncEventID,
            MatchSourceSyncEventIDAndPercentage,
            MatchSourceSyncEventPercentage
        };

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Unknown; }
        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual char const* GetCategory() const override { return "Transitions"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree ); }
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    protected:

        EE_REGISTER String                                      m_name = "Transition";

        // Should we use a easing mode for the blend weight calculation?
        EE_EXPOSE Math::Easing::Type                            m_blendWeightEasingType = Math::Easing::Type::Linear;

        // How should we blend the root motion for this transition
        EE_EXPOSE RootMotionBlendMode                           m_rootMotionBlend = RootMotionBlendMode::Blend;

        // How long should this transition take?
        EE_EXPOSE Seconds                                       m_duration = 0.3f;

        // Should we clamp the transition duration to the estimated remaining time left in the source state (prevent looping of source states)
        EE_EXPOSE bool                                          m_clampDurationToSource = false;

        // Can this transition be force started, needed when you want to emergency transition back to your original state
        EE_EXPOSE bool                                          m_canBeForced = false;

        // How should we determine the start time of the target node?
        EE_EXPOSE TimeMatchMode                                 m_timeMatchMode = TimeMatchMode::None;

        // Sync event offset to apply to target node start time - with no time match mode it will start the target node at the specified offset
        EE_EXPOSE float                                         m_syncEventOffset = 0.0f;
    };

    //-------------------------------------------------------------------------

    class TransitionConduitToolsNode final : public VisualGraph::SM::TransitionConduit
    {
        EE_REGISTER_TYPE( TransitionConduitToolsNode );

    public:

        bool HasTransitions() const;

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual ImColor GetNodeBorderColor( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::NodeVisualState visualState ) const override;
    };
}
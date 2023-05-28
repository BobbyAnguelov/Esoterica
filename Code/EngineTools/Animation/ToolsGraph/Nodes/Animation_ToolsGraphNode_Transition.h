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
        EE_REFLECT_TYPE( TransitionToolsNode );

        enum class TimeMatchMode
        {
            EE_REFLECT_ENUM

            None,
            Synchronized,
            MatchSourceSyncEventIndexOnly,
            MatchSourceSyncEventIndexAndPercentage,
            MatchSourceSyncEventID,
            MatchSourceSyncEventIDAndPercentage,
            MatchSourceSyncEventPercentage
        };

    public:

        TransitionToolsNode();

        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Unknown; }
        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual char const* GetCategory() const override { return "Transitions"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree ); }
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;
        virtual ImColor GetTitleBarColor() const override;

    protected:

        EE_REFLECT( "IsToolsReadOnly" : true );
        String                                        m_name = "Transition";

        EE_REFLECT();
        Math::Easing::Type                            m_blendWeightEasingType = Math::Easing::Type::Linear; // Should we use a easing mode for the blend weight calculation?

        EE_REFLECT();
        RootMotionBlendMode                           m_rootMotionBlend = RootMotionBlendMode::Blend; // How should we blend the root motion for this transition

        EE_REFLECT();
        Seconds                                       m_duration = 0.2f; // How long should this transition take?

        EE_REFLECT();
        bool                                          m_clampDurationToSource = false; // Should we clamp the transition duration to the estimated remaining time left in the source state (prevent looping of source states)

        EE_REFLECT();
        bool                                          m_canBeForced = false; // Can this transition be force started, needed when you want to emergency transition back to your original state

        EE_REFLECT();
        TimeMatchMode                                 m_timeMatchMode = TimeMatchMode::None; // How should we determine the start time of the target node?

        EE_REFLECT();
        float                                         m_syncEventOffset = 0.0f; // Sync event offset to apply to target node start time - with no time match mode it will start the target node at the specified offset
    };

    //-------------------------------------------------------------------------

    class TransitionConduitToolsNode final : public VisualGraph::SM::TransitionConduit
    {
        EE_REFLECT_TYPE( TransitionConduitToolsNode );

    public:

        bool HasTransitions() const;

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual ImColor GetColor( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::NodeVisualState visualState ) const override;
        virtual void PreDrawUpdate( VisualGraph::UserContext* pUserContext ) override;

    private:

        bool isAnyChildActive = false;
    };
}
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

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "Transition"; }
        virtual char const* GetCategory() const override { return "Transitions"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionTree ); }
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    protected:

        EE_EXPOSE String                                       m_name = "Transition";
        EE_EXPOSE Math::Easing::Type                           m_blendWeightEasingType = Math::Easing::Type::Linear;
        EE_EXPOSE RootMotionBlendMode                          m_rootMotionBlend = RootMotionBlendMode::Blend;
        EE_EXPOSE Seconds                                      m_duration = 0.3f;
        EE_EXPOSE float                                        m_syncEventOffset = 0.0f;
        EE_EXPOSE bool                                         m_isSynchronized = false;
        EE_EXPOSE bool                                         m_clampDurationToSource = true;
        EE_EXPOSE bool                                         m_keepSourceSyncEventIdx = false;
        EE_EXPOSE bool                                         m_keepSourceSyncEventPercentageThrough = false;
        EE_EXPOSE bool                                         m_canBeForced = false;
    };

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
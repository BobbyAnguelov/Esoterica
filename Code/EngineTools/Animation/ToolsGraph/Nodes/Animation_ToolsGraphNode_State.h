#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ResultToolsNode;

    //-------------------------------------------------------------------------

    // The result node for a state's layer settings
    class StateLayerDataToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( StateLayerDataToolsNode );

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "State Layer Data"; }
        virtual char const* GetCategory() const override { return "State Machine"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
    };

    //-------------------------------------------------------------------------

    class StateToolsNode final : public VisualGraph::SM::State
    {
        friend class StateMachineToolsNode;
        EE_REGISTER_TYPE( StateToolsNode );

    public:

        struct TimedStateEvent : public IRegisteredType
        {
            EE_REGISTER_TYPE( TimedStateEvent );

            EE_EXPOSE StringID                 m_ID;
            EE_EXPOSE Seconds                  m_timeValue;
        };

    public:

        enum class StateType
        {
            EE_REGISTER_ENUM

            OffState,
            BlendTreeState,
            StateMachineState
        };

    public:

        StateToolsNode() = default;
        StateToolsNode( StateType type ) : m_type( type ) {}

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override { EE_ASSERT( IsRenameable() ); m_name = newName; }

        inline bool IsOffState() const { return m_type == StateType::OffState; }
        inline bool IsBlendTreeState() const { return m_type == StateType::BlendTreeState; }
        inline bool IsStateMachineState() const { return m_type == StateType::StateMachineState; }

    private:

        virtual char const* GetTypeName() const override { return "State"; }
        virtual ImColor GetTitleBarColor() const override;
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REGISTER String                      m_name = "State";
        EE_EXPOSE TVector<StringID>             m_entryEvents;
        EE_EXPOSE TVector<StringID>             m_executeEvents;
        EE_EXPOSE TVector<StringID>             m_exitEvents;
        EE_EXPOSE TVector<TimedStateEvent>      m_timeRemainingEvents;
        EE_EXPOSE TVector<TimedStateEvent>      m_timeElapsedEvents;
        EE_REGISTER StateType                   m_type = StateType::BlendTreeState;
    };
}
#pragma once
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/NodeGraph/NodeGraph_StateMachineGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ResultToolsNode;

    //-------------------------------------------------------------------------

    // The result node for a state's layer settings
    class StateLayerDataToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( StateLayerDataToolsNode );

        StateLayerDataToolsNode();

        virtual char const* GetTypeName() const override { return "State Layer Data"; }
        virtual char const* GetCategory() const override { return "State Machine"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override { EE_UNREACHABLE_CODE(); return InvalidIndex; }
    };

    //-------------------------------------------------------------------------

    class StateToolsNode final : public NodeGraph::StateNode
    {
        friend class StateMachineToolsNode;
        EE_REFLECT_TYPE( StateToolsNode );

        constexpr static float const s_minimumStateNodeUnscaledWidth = 30;
        constexpr static float const s_minimumStateNodeUnscaledHeight = 30;

    public:

        struct TimedStateEvent : public IReflectedType
        {
            EE_REFLECT_TYPE( TimedStateEvent );

            EE_REFLECT( CustomEditor = "AnimGraph_ID" );
            StringID                 m_ID;

            EE_REFLECT();
            Seconds                  m_timeValue;
        };

    public:

        enum class StateType
        {
            EE_REFLECT_ENUM

            OffState,
            BlendTreeState,
            StateMachineState
        };

    public:

        StateToolsNode();
        StateToolsNode( StateType type );

        virtual bool IsRenameable() const override final { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }

        inline bool IsOffState() const { return m_type == StateType::OffState; }
        inline bool IsBlendTreeState() const { return m_type == StateType::BlendTreeState; }
        inline bool IsStateMachineState() const { return m_type == StateType::StateMachineState; }

        // Return any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const;

        // Rename any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID );

    private:

        virtual char const* GetTypeName() const override { return "State"; }
        virtual Color GetTitleBarColor() const override;
        virtual void DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos ) override;
        virtual NodeGraph::BaseGraph* GetNavigationTarget() override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual bool IsActive( NodeGraph::UserContext* pUserContext ) const override;
        virtual void OnShowNode() override;

        bool CanConvertToBlendTreeState();
        void ConvertToBlendTreeState();

        bool CanConvertToStateMachineState();
        void ConvertToStateMachineState();

        void SharedConstructor();

    private:

        EE_REFLECT( ReadOnly );
        StateType                       m_type = StateType::BlendTreeState;

        //-------------------------------------------------------------------------

        // These events are emitted in all cases (entry/execute/exit)
        EE_REFLECT( CustomEditor = "AnimGraph_ID" );
        TVector<StringID>               m_events;

        //-------------------------------------------------------------------------

        // Only emitted when entering the state
        EE_REFLECT( Category = "Phase Events", CustomEditor = "AnimGraph_ID" );
        TVector<StringID>               m_entryEvents;

        // Only emitted when fully in (no transition occurring) the state
        EE_REFLECT( Category = "Phase Events", CustomEditor = "AnimGraph_ID" );
        TVector<StringID>               m_executeEvents;

        // Only emitted when exiting the state
        EE_REFLECT( Category = "Phase Events", CustomEditor = "AnimGraph_ID" );
        TVector<StringID>               m_exitEvents;

        //-------------------------------------------------------------------------

        // Only emitted when a time remaining condition is met
        EE_REFLECT( Category = "Timed Events" );
        TVector<TimedStateEvent>        m_timeRemainingEvents;

        // Only emitted when a time elapsed condition is met
        EE_REFLECT( Category = "Timed Events" );
        TVector<TimedStateEvent>        m_timeElapsedEvents;
    };
}
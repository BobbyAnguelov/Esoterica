#pragma once
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/NodeGraph/NodeGraph_StateMachineGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_State.h"

//-------------------------------------------------------------------------

namespace EE::Animation
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
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::EntryOverrideTree ); }
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

        struct StateEvent : public IReflectedType
        {
            EE_REFLECT_TYPE( StateEvent );

            inline bool operator==( StringID const &ID ) const { return m_ID == ID; }
            inline bool operator!=( StringID const &ID ) const { return m_ID != ID; }

        public:

            EE_REFLECT( CustomEditor = "AnimGraph_ID" );
            StringID        m_ID;

            EE_REFLECT();
            bool            m_isEntry = true;

            EE_REFLECT();
            bool            m_isFullyInState = true;

            EE_REFLECT();
            bool            m_isExit = true;
        };

        enum class TimedStateEventType
        {
            EE_REFLECT_ENUM

            TimeElapsed,
            TimeRemaining,
        };

        struct TimedStateEvent : public IReflectedType
        {
            EE_REFLECT_TYPE( TimedStateEvent );

        public:

            inline bool IsValid() const { return m_ID.IsValid() && m_timeValue >= 0.0f; }
            inline bool IsTimeElapsedEvent() const { return m_type == TimedStateEventType::TimeElapsed; }
            inline bool IsTimeRemainingEvent() const { return m_type == TimedStateEventType::TimeRemaining; }

            inline bool operator==( StringID const &ID ) const { return m_ID == ID; }
            inline bool operator!=( StringID const &ID ) const { return m_ID != ID; }

            inline bool operator==( TimedStateEvent const& rhs ) const { return m_ID == rhs.m_ID && m_timeValue == rhs.m_timeValue; }
            inline bool operator!=( TimedStateEvent const& rhs ) const { return m_ID != rhs.m_ID || m_timeValue != rhs.m_timeValue; }

        public:

            EE_REFLECT( CustomEditor = "AnimGraph_ID" );
            StringID                                        m_ID;

            EE_REFLECT();
            Seconds                                         m_timeValue;

            EE_REFLECT();
            TimedStateEventType                             m_type = TimedStateEventType::TimeElapsed;

            EE_REFLECT();
            Animation::StateNode::TimedEvent::Comparison    m_comparisonOperator;
        };

    public:

        enum class StateType
        {
            EE_REFLECT_ENUM

            OffState,
            BlendTreeState,
            StateMachineState,
            Clone,
        };

        static void UpdateAllClonedStates( TypeSystem::TypeRegistry const& typeRegistry, NodeGraph::BaseGraph* pGraph, bool isPartOfBulkUpdate = false );

    public:

        StateToolsNode( DefaultInstanceCtor_t ) : NodeGraph::StateNode() {}
        StateToolsNode();
        StateToolsNode( StateType type );
        StateToolsNode( UUID const &sourceStateID );

        virtual bool IsRenameable() const override final { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }

        inline bool IsOffState() const { return m_type == StateType::OffState; }
        inline bool IsBlendTreeState() const { return m_type == StateType::BlendTreeState; }
        inline bool IsStateMachineState() const { return m_type == StateType::StateMachineState; }
        inline bool IsClonedState() const { return m_type == StateType::Clone; }

        // Return any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const;

        // Rename any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID );

        // Call this to update the internal clone state ID
        void UpdateCloneStateID( TypeSystem::TypeRegistry const& typeRegistry, THashMap<UUID, UUID> const& IDMapping );

        // Update the state's clone version - this is used to ensure that we dont regenerate cloned states unecessarily
        void UpdateCloneVersion();

    private:

        virtual char const* GetTypeName() const override { return "State"; }
        virtual Color GetTitleBarColor() const override;
        virtual void DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos ) override;
        virtual NodeGraph::BaseGraph* GetNavigationTarget( NodeGraph::UserContext* pUserContext ) override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual Color GetHighlightOutlineColor( NodeGraph::UserContext* pUserContext ) const override;
        virtual void OnShowNode() override;
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;
        virtual void PostModify() override;

        // State Type
        //-------------------------------------------------------------------------

        bool CanConvertToBlendTreeState();
        void ConvertToBlendTreeState();

        bool CanConvertToStateMachineState();
        void ConvertToStateMachineState();

        void UpdateClonedData( TypeSystem::TypeRegistry const& typeRegistry, bool isPartOfBulkUpdate = false );
        StateToolsNode const *GetCloneStateSourceNode() const;
        char const *GetClonedStateName() const;
        void CreateClone( NodeGraph::UserContext* pUserContext );

        // Events
        //-------------------------------------------------------------------------

        TInlineVector<StringID, 5> GetUniqueEntryEvents() const;
        TInlineVector<StringID, 5> GetUniqueFullyInStateEvents() const;
        TInlineVector<StringID, 5> GetUniqueExitEvents() const;
        TInlineVector<TimedStateEvent, 5> GetUniqueTimeRemainingEvents() const;
        TInlineVector<TimedStateEvent, 5> GetUniqueTimeElapsedEvents() const;

    private:

        EE_REFLECT( ReadOnly );
        StateType                       m_type = StateType::BlendTreeState;

        EE_REFLECT( ReadOnly );
        UUID                            m_cloneSourceStateID;

        EE_REFLECT( ReadOnly );
        UUID                            m_cloneVersion;

        //-------------------------------------------------------------------------

        // Emitted based on the phase of the state
        EE_REFLECT();
        TVector<StateEvent>             m_stateEvents;

        // Only emitted when a time remaining condition is met
        EE_REFLECT();
        TVector<TimedStateEvent>        m_timedEvents;
    };
}
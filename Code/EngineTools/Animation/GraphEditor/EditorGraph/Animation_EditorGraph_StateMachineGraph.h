#pragma once
#include "Animation_EditorGraph_FlowGraph.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_StateMachineGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class StateMachineGraph;

    //-------------------------------------------------------------------------
    // STATE NODES
    //-------------------------------------------------------------------------

    namespace GraphNodes
    {
        // Base class for all states
        class StateBaseEditorNode : public VisualGraph::SM::State
        {
            friend class StateMachineEditorNode;
            EE_REGISTER_TYPE( StateBaseEditorNode );

        public:

            struct TimedStateEvent : public IRegisteredType
            {
                EE_REGISTER_TYPE( TimedStateEvent );

                EE_EXPOSE StringID                 m_ID;
                EE_EXPOSE Seconds                  m_timeValue;
            };

        public:

            virtual char const* GetName() const override { return m_name.c_str(); }
            virtual bool IsRenamable() const override { return true; }
            virtual void SetName( String const& newName ) override { EE_ASSERT( IsRenamable() ); m_name = newName; }

        protected:

            virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx ) override;

        protected:

            EE_EXPOSE String                       m_name = "State";
            EE_EXPOSE TVector<StringID>            m_entryEvents;
            EE_EXPOSE TVector<StringID>            m_executeEvents;
            EE_EXPOSE TVector<StringID>            m_exitEvents;
            EE_EXPOSE TVector<TimedStateEvent>     m_timeRemainingEvents;
            EE_EXPOSE TVector<TimedStateEvent>     m_timeElapsedEvents;
        };

        // The result node for a state's layer settings
        class StateLayerDataEditorNode final : public EditorGraphNode
        {
            EE_REGISTER_TYPE( StateLayerDataEditorNode );

            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

            virtual char const* GetTypeName() const override { return "State Layer Data"; }
            virtual char const* GetCategory() const override { return "State Machine"; }
            virtual bool IsUserCreatable() const override { return false; }
            virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
        };

        // A basic state
        class StateEditorNode final : public StateBaseEditorNode
        {
            friend class StateMachineEditorNode;
            EE_REGISTER_TYPE( StateEditorNode );

        public:

            ResultEditorNode const* GetBlendTreeRootNode() const;
            StateLayerDataEditorNode const* GetLayerDataNode() const;
            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        private:

            virtual char const* GetTypeName() const override { return "State"; }
            virtual void DrawExtraContextMenuOptions( VisualGraph::DrawContext const& ctx, Float2 const& mouseCanvasPos ) override;
        };

        // An off-state
        class OffStateEditorNode final : public StateBaseEditorNode
        {
            friend class StateMachineEditorNode;
            EE_REGISTER_TYPE( OffStateEditorNode );

        private:

            virtual char const* GetTypeName() const override { return "Off State"; }
            virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::PaleVioletRed ); }
            virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx ) override;
        };
    }

    //-------------------------------------------------------------------------
    // ENTRY STATE OVERRIDES
    //-------------------------------------------------------------------------

    namespace GraphNodes
    {
        // The result node for the entry state overrides
        class EntryStateOverrideConditionsEditorNode final : public EditorGraphNode
        {
            friend StateMachineGraph;
            EE_REGISTER_TYPE( EntryStateOverrideConditionsEditorNode );

        public:

            void UpdateInputPins();
            virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }

        private:

            virtual char const* GetTypeName() const override { return "Entry State Conditions"; }
            virtual char const* GetCategory() const override { return "State Machine"; }
            virtual bool IsUserCreatable() const override { return false; }

        private:

            // For each pin, what state does it represent
            EE_REGISTER TVector<UUID>  m_pinToStateMapping;
        };

        // The entry state override node
        class EntryStateOverrideConduitEditorNode final : public VisualGraph::SM::Node
        {
            EE_REGISTER_TYPE( EntryStateOverrideConduitEditorNode );

        public:

            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        private:

            virtual bool IsVisible() const override { return false; }
            virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::SlateBlue ); }
            virtual char const* GetTypeName() const override { return "Entry Overrides"; }
            virtual bool IsUserCreatable() const override { return false; }
        };
    }

    // The entry state overrides flow graph
    class EntryStateOverrideEditorGraph final : public FlowGraph
    {
        EE_REGISTER_TYPE( EntryStateOverrideEditorGraph );

    public:

        EntryStateOverrideEditorGraph() : FlowGraph( GraphType::ValueTree ) {}
        virtual void Initialize( VisualGraph::BaseNode* pParentNode ) override;

    private:

        virtual void OnShowGraph() override;
    };

    //-------------------------------------------------------------------------
    // TRANSITIONS
    //-------------------------------------------------------------------------

    namespace GraphNodes
    {
        // The result node for a transition
        class TransitionEditorNode : public EditorGraphNode
        {
            friend class StateMachineEditorNode;
            EE_REGISTER_TYPE( TransitionEditorNode );

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

        // Transition conduit
        class TransitionConduitEditorNode final : public VisualGraph::SM::TransitionConduit
        {
            EE_REGISTER_TYPE( TransitionConduitEditorNode );

        public:

            bool HasTransitions() const;

            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
            virtual char const* GetTypeName() const override { return "Transition"; }
            virtual ImColor GetNodeBorderColor( VisualGraph::DrawContext const& ctx, VisualGraph::NodeVisualState visualState ) const override;
        };
    }

    //-------------------------------------------------------------------------

     // The Flow graph for the state machine node
    class GlobalTransitionsEditorGraph final : public FlowGraph
    {
        EE_REGISTER_TYPE( GlobalTransitionsEditorGraph );

    public:

        GlobalTransitionsEditorGraph() : FlowGraph( GraphType::ValueTree ) {}
        void UpdateGlobalTransitionNodes();

    private:

        virtual void OnShowGraph() override { UpdateGlobalTransitionNodes(); }
    };

    namespace GraphNodes
    {
        // The result node for a global transition
        class GlobalTransitionEditorNode final : public TransitionEditorNode
        {
            friend GlobalTransitionsEditorGraph;
            friend StateMachineGraph;
            EE_REGISTER_TYPE( GlobalTransitionEditorNode );

        public:

            inline UUID const& GetEndStateID() const { return m_stateID; }

            virtual char const* GetName() const override { return m_name.c_str(); }
            virtual char const* GetTypeName() const override { return "Global Transition"; }
            virtual char const* GetCategory() const override { return "State Machine"; }
            virtual bool IsUserCreatable() const override { return false; }
            virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }

        private:

            EE_REGISTER UUID                                       m_stateID;
        };

        // The global transition node present in state machine graphs
        class GlobalTransitionConduitEditorNode final : public VisualGraph::SM::Node
        {
            EE_REGISTER_TYPE( GlobalTransitionConduitEditorNode );

        public:

            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        private:

            virtual bool IsVisible() const override { return false; }
            virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::OrangeRed ); }
            virtual char const* GetTypeName() const override { return "Global Transitions"; }
            virtual bool IsUserCreatable() const override { return false; }
        };
    }

    //-------------------------------------------------------------------------
    // STATE MACHINE
    //-------------------------------------------------------------------------

    // The animation state machine graph
    class StateMachineGraph final : public VisualGraph::StateMachineGraph
    {
        EE_REGISTER_TYPE( StateMachineGraph );

    public:

        void CreateNewState( Float2 const& mouseCanvasPos );
        void CreateNewOffState( Float2 const& mouseCanvasPos );

        GraphNodes::GlobalTransitionConduitEditorNode const* GetGlobalTransitionConduit() const { return m_pGlobalTransitionConduit; }
        GraphNodes::GlobalTransitionConduitEditorNode* GetGlobalTransitionConduit() { return m_pGlobalTransitionConduit; }

        GraphNodes::EntryStateOverrideConduitEditorNode const* GetEntryStateOverrideConduit() const { return m_pEntryOverridesConduit; }
        GraphNodes::EntryStateOverrideConduitEditorNode* GetEntryStateOverrideConduit() { return m_pEntryOverridesConduit; }

        bool HasGlobalTransitionForState( UUID const& stateID ) const;
        GraphNodes::EditorGraphNode const* GetEntryConditionNodeForState( UUID const& stateID ) const;

    private:

        void UpdateDependentNodes();

        virtual void Initialize( VisualGraph::BaseNode* pParentNode ) override;
        virtual bool CanDeleteNode( VisualGraph::BaseNode const* pNode ) const override;
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;
        virtual VisualGraph::SM::TransitionConduit* CreateTransitionNode() const override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) override;
        virtual void PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes ) override;
        virtual void DrawExtraContextMenuOptions( VisualGraph::DrawContext const& ctx, Float2 const& mouseCanvasPos ) override;

    private:

        GraphNodes::EntryStateOverrideConduitEditorNode*            m_pEntryOverridesConduit = nullptr;
        GraphNodes::GlobalTransitionConduitEditorNode*              m_pGlobalTransitionConduit = nullptr;
    };

    namespace GraphNodes
    {
        // The state machine node shown in blend trees
        class StateMachineEditorNode final : public EditorGraphNode
        {
            EE_REGISTER_TYPE( StateMachineEditorNode );

        public:

            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

            virtual char const* GetName() const override { return m_name.c_str(); }
            virtual char const* GetTypeName() const override { return "State Machine"; }
            virtual char const* GetCategory() const override { return ""; }
            virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::CornflowerBlue ); }

            virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
            virtual int16_t Compile( GraphCompilationContext& context ) const override;

        private:

            int16_t CompileState( GraphCompilationContext& context, StateBaseEditorNode const* pBaseStateNode ) const;
            int16_t CompileTransition( GraphCompilationContext& context, TransitionEditorNode const* pTransitionNode, int16_t targetStateNodeIdx ) const;

        private:

            EE_EXPOSE String m_name = "SM";
        };
    }
}
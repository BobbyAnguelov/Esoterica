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
        EE_REFLECT_TYPE( StateLayerDataToolsNode );

        StateLayerDataToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Unknown; }
        virtual char const* GetTypeName() const override { return "State Layer Data"; }
        virtual char const* GetCategory() const override { return "State Machine"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
    };

    //-------------------------------------------------------------------------

    class StateToolsNode final : public VisualGraph::SM::State
    {
        friend class StateMachineToolsNode;
        EE_REFLECT_TYPE( StateToolsNode );

    public:

        struct TimedStateEvent : public IReflectedType
        {
            EE_REFLECT_TYPE( TimedStateEvent );

            EE_REFLECT() StringID                 m_ID;
            EE_REFLECT() Seconds                  m_timeValue;
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

        StateToolsNode() = default;
        StateToolsNode( StateType type ) : m_type( type ) {}

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override { EE_ASSERT( IsRenameable() ); m_name = newName; }

        inline bool IsOffState() const { return m_type == StateType::OffState; }
        inline bool IsBlendTreeState() const { return m_type == StateType::BlendTreeState; }
        inline bool IsStateMachineState() const { return m_type == StateType::StateMachineState; }

        // Return any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const;

    private:

        virtual char const* GetTypeName() const override { return "State"; }
        virtual ImColor GetTitleBarColor() const override;
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual bool IsActive( VisualGraph::UserContext* pUserContext ) const override;
        virtual void OnShowNode() override;

        bool CanConvertToBlendTreeState();
        void ConvertToBlendTreeState();

        bool CanConvertToStateMachineState();
        void ConvertToStateMachineState();

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        String                          m_name = "State";
        
        EE_REFLECT();
        TVector<StringID>               m_entryEvents;

        EE_REFLECT();
        TVector<StringID>               m_executeEvents;

        EE_REFLECT();
        TVector<StringID>               m_exitEvents;

        EE_REFLECT();
        TVector<TimedStateEvent>        m_timeRemainingEvents;

        EE_REFLECT();
        TVector<TimedStateEvent>        m_timeElapsedEvents;
        
        EE_REFLECT( "IsToolsReadOnly" : true );
        StateType                       m_type = StateType::BlendTreeState;
    };
}
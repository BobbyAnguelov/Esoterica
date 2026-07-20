#pragma once

#include "NodeGraph_BaseGraph.h"
#include "NodeGraph_Comment.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    class StateMachineGraph;
    class GraphView;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API StateMachineNode : public BaseNode
    {
        friend StateMachineGraph;
        friend GraphView;

        EE_REFLECT_TYPE( StateMachineNode );

    protected:

        // Override this if you want to provide additional context menu options
        virtual void DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos ) {}
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API StateNode : public StateMachineNode
    {
        friend StateMachineGraph;
        friend GraphView;

        EE_REFLECT_TYPE( StateNode );
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TransitionConduitNode : public StateMachineNode
    {
        friend StateMachineGraph;
        friend GraphView;

        EE_REFLECT_TYPE( TransitionConduitNode );

    public:

        using StateMachineNode::StateMachineNode;

        TransitionConduitNode( NodeGraph::StateNode const* pStartState, NodeGraph::StateNode const* pEndState );

        // Does this conduit contain any transitions
        virtual bool HasTransitions() const { return false; }

        // Get the ID of the start (from) state
        inline UUID const GetStartStateID() const { return m_startStateID; }

        // Get the ID of the end (to) state
        inline UUID const GetEndStateID() const { return m_endStateID; }

        // Set the ID of the start (from) state
        void SetStartStateID( UUID const &startStateID );

        // Set the ID of the end (to) state
        void SetEndStateID( UUID const &endStateID );

        // Switch the start and end states
        void InvertDirection();

        // Get the conduit arrow color
        // Note, if you return transparent then we will use the default colors from the graph view in terms of selection/hover
        virtual Color GetConduitColor( UserContext* pUserContext ) const { return Colors::Transparent; }

        // Conduit specific draw extra controls function, that provides the start and end points of the conduit
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, ImVec2 const& startPoint, ImVec2 const& endPoint ) {}

    private:

        virtual void DrawExtraControls( DrawContext const& ctx, UserContext* pUserContext ) override final {}

    protected:

        Percentage m_transitionProgress = 0.0f;

    private:

        EE_REFLECT( ReadOnly );
        UUID        m_startStateID;

        EE_REFLECT( ReadOnly );
        UUID        m_endStateID;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API StateMachineGraph : public BaseGraph
    {
        EE_REFLECT_TYPE( StateMachineGraph );

        friend GraphView;

    public:

        inline UUID const& GetDefaultEntryStateID() const { return m_entryStateID; }
        void SetDefaultEntryState( UUID const& newDefaultEntryStateID );

        // Transitions
        //-------------------------------------------------------------------------

        bool DoesTransitionConduitExist( StateNode const* pStartState, StateNode const* pEndState ) const;
        bool DoesTransitionConduitExist( UUID const& startStateID, UUID const &endStateID ) const;
        bool CanCreateTransitionConduit( StateNode const* pStartState, StateNode const* pEndState ) const;
        bool CanCreateTransitionConduit( UUID const &startStateID, UUID const &endStateID ) const;
        virtual TransitionConduitNode* CreateTransitionConduit( StateNode const* pStartState, StateNode const* pEndState ) = 0;


    protected:

        virtual void PostDeserialize( TypeSystem::TypeRegistry const& typeRegistry ) override;

        virtual BaseNode const* GetMostSignificantNode() const override { return FindNode( m_entryStateID ); }

        void UpdateEntryState();

        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;

        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const override;

        virtual void PreDestroyNode( BaseNode* pNodeAboutToBeDestroyed ) override;
        virtual void PostDestroyNode( UUID const& nodeID ) override;

        // Draw the graph context menu option - returns true if the menu should be closed i.e. a custom selection or action has been made
        virtual bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens ) { return false; }

        // Should we show the context menu filter?
        virtual bool HasContextMenuFilter() const { return true; }

    protected:

        EE_REFLECT( ReadOnly );
        UUID        m_entryStateID;
    };
}
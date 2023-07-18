#pragma once

#include "VisualGraph_BaseGraph.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    class StateMachineGraph;
    class GraphView;

    //-------------------------------------------------------------------------

    namespace SM
    {
        class EE_ENGINETOOLS_API Node : public BaseNode
        {
            friend StateMachineGraph;
            friend GraphView;

            EE_REFLECT_TYPE( Node );

        protected:

            // Override this if you want to provide additional context menu options
            virtual void DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos ) {}
        };

        //-------------------------------------------------------------------------

        class EE_ENGINETOOLS_API State : public Node
        {
            friend StateMachineGraph;
            friend GraphView;

            EE_REFLECT_TYPE( State );
        };

        //-------------------------------------------------------------------------

        class EE_ENGINETOOLS_API TransitionConduit : public Node
        {
            friend StateMachineGraph;
            friend GraphView;

            EE_REFLECT_TYPE( TransitionConduit );

            // Conduits are special since they are drawn as connections

            inline UUID const GetStartStateID() const { return m_startStateID; }
            inline UUID const GetEndStateID() const { return m_endStateID; }

            // Get the conduit arrow color
            virtual Color GetColor( VisualGraph::DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const;

            // Conduit specific draw extra controls function, that provides the start and end points of the conduit
            virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, ImVec2 const& startPoint, ImVec2 const& endPoint ) {}

        private:

            virtual Color GetNodeBorderColor( VisualGraph::DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const override final { return Node::GetNodeBorderColor( ctx, pUserContext, visualState ); }
            virtual void DrawExtraControls( DrawContext const& ctx, UserContext* pUserContext ) override final {}

        protected:

            Percentage m_transitionProgress = 0.0f;

        private:

            EE_REFLECT( "IsToolsReadOnly" : true );
            UUID        m_startStateID;

            EE_REFLECT( "IsToolsReadOnly" : true );
            UUID        m_endStateID;
        };
    }

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

        bool DoesTransitionConduitExist( SM::State const* pStartState, SM::State const* pEndState ) const;
        bool CanCreateTransitionConduit( SM::State const* pStartState, SM::State const* pEndState ) const;
        void CreateTransitionConduit( SM::State const* pStartState, SM::State const* pEndState );

    protected:

        virtual BaseNode const* GetMostSignificantNode() const override { return FindNode( m_entryStateID ); }

        void UpdateEntryState();

        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;

        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const override { return pNodeTypeInfo->IsDerivedFrom( SM::Node::GetStaticTypeID() ); }

        virtual void PreDestroyNode( BaseNode* pNodeAboutToBeDestroyed ) override;
        virtual void PostDestroyNode( UUID const& nodeID ) override;

        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) override;

        // Draw the graph context menu option - returns true if the menu should be closed i.e. a custom selection or action has been made
        virtual bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens ) { return false; }

        // Should we show the context menu filter?
        virtual bool HasContextMenuFilter() const { return true; }

    protected:

        // Users need to implement this function to create the appropriate transition node
        virtual SM::TransitionConduit* CreateTransitionNode() const = 0;

    protected:

        EE_REFLECT( "IsToolsReadOnly" : true );
        UUID m_entryStateID;
    };
}
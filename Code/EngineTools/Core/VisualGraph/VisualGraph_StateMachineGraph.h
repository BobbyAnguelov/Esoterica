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

            EE_REGISTER_TYPE( Node );

        protected:

            // Override this if you want to provide additional context menu options
            virtual void DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos ) {}
        };

        //-------------------------------------------------------------------------

        class EE_ENGINETOOLS_API State : public Node
        {
            friend StateMachineGraph;
            friend GraphView;

            EE_REGISTER_TYPE( State );
        };

        //-------------------------------------------------------------------------

        class EE_ENGINETOOLS_API TransitionConduit : public Node
        {
            friend StateMachineGraph;
            friend GraphView;

            EE_REGISTER_TYPE( TransitionConduit );

            // Conduits are special since they are drawn as connections
            virtual ImColor GetNodeBorderColor( VisualGraph::DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const override;

            inline UUID const GetStartStateID() const { return m_startStateID; }
            inline UUID const GetEndStateID() const { return m_endStateID; }

        private:

            EE_REGISTER UUID   m_startStateID;
            EE_REGISTER UUID   m_endStateID;
        };
    }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API StateMachineGraph : public BaseGraph
    {
        EE_REGISTER_TYPE( StateMachineGraph );

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

        // Draw the graph context menu option - returns true if the menu should be closed i.e. a cusotm selection or action has been made
        virtual bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens ) { return false; }

    protected:

        // Users need to implement this function to create the appropriate transition node
        virtual SM::TransitionConduit* CreateTransitionNode() const = 0;

    protected:

        EE_REGISTER UUID       m_entryStateID;
    };
}
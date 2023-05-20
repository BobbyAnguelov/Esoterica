#pragma once
#include "EngineTools/Core/VisualGraph/VisualGraph_StateMachineGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    namespace GraphNodes
    {
        class EntryStateOverrideConduitToolsNode;
        class GlobalTransitionConduitToolsNode;
    }

    //-------------------------------------------------------------------------

    // The animation state machine graph
    class StateMachineGraph final : public VisualGraph::StateMachineGraph
    {
        EE_REFLECT_TYPE( StateMachineGraph );

    public:

        GraphNodes::EntryStateOverrideConduitToolsNode const* GetEntryStateOverrideConduit() const;
        GraphNodes::EntryStateOverrideConduitToolsNode* GetEntryStateOverrideConduit() { return const_cast<GraphNodes::EntryStateOverrideConduitToolsNode*>( const_cast<StateMachineGraph const*>( this )->GetEntryStateOverrideConduit() ); }
        GraphNodes::GlobalTransitionConduitToolsNode const* GetGlobalTransitionConduit() const;
        GraphNodes::GlobalTransitionConduitToolsNode* GetGlobalTransitionConduit() { return const_cast<GraphNodes::GlobalTransitionConduitToolsNode*>( const_cast<StateMachineGraph const*>( this )->GetGlobalTransitionConduit() ); }
        
        void UpdateDependentNodes();

        // Node factory methods
        //-------------------------------------------------------------------------

        template<typename T, typename ... ConstructorParams>
        T* CreateNode( ConstructorParams&&... params )
        {
            VisualGraph::ScopedGraphModification sgm( this );

            static_assert( std::is_base_of<VisualGraph::SM::Node, T>::value );
            auto pNode = EE::New<T>( std::forward<ConstructorParams>( params )... );
            pNode->Initialize( this );

            // Ensure unique name
            if ( pNode->IsRenameable() )
            {
                pNode->SetName( GetUniqueNameForRenameableNode( pNode->GetName() ) );
            }

            AddNode( pNode );
            return pNode;
        }

    private:

        virtual void DrawExtraInformation( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual bool DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens ) override;
        virtual void Initialize( VisualGraph::BaseNode* pParentNode ) override;
        virtual bool CanDeleteNode( VisualGraph::BaseNode const* pNode ) const override;
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;
        virtual VisualGraph::SM::TransitionConduit* CreateTransitionNode() const override;
        virtual void PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes ) override;
        virtual void PostDestroyNode( UUID const& nodeID ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;

        bool CanConvertToBlendTreeState() const;
        bool CanConvertToStateMachineState() const;
    };
}
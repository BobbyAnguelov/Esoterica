#pragma once
#include "EngineTools/Core/VisualGraph/VisualGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;
    class ToolsGraphDefinition;

    //-------------------------------------------------------------------------
    // Base Animation Flow Graph
    //-------------------------------------------------------------------------

    class FlowGraph : public VisualGraph::FlowGraph
    {
        friend class ToolsGraphDefinition;
        EE_REGISTER_TYPE( FlowGraph );

    public:

        FlowGraph( GraphType type = GraphType::BlendTree );

        inline GraphType GetType() const { return m_type; }

        // Node factory methods
        //-------------------------------------------------------------------------

        template<typename T, typename ... ConstructorParams>
        T* CreateNode( ConstructorParams&&... params )
        {
            VisualGraph::ScopedGraphModification sgm( this );

            static_assert( std::is_base_of<GraphNodes::FlowToolsNode, T>::value );
            auto pNode = EE::New<T>( std::forward<ConstructorParams>( params )... );
            EE_ASSERT( pNode->GetAllowedParentGraphTypes().IsFlagSet( m_type ) );
            pNode->Initialize( this );

            // Ensure unique name
            if ( pNode->IsRenameable() )
            {
                pNode->SetName( GetUniqueNameForRenameableNode( pNode->GetName() ) );
            }

            AddNode( pNode );
            return pNode;
        }

        GraphNodes::FlowToolsNode* CreateNode( TypeSystem::TypeInfo const* pTypeInfo )
        {
            VisualGraph::ScopedGraphModification sgm( this );

            EE_ASSERT( pTypeInfo->IsDerivedFrom( GraphNodes::FlowToolsNode::GetStaticTypeID() ) );
            auto pNode = Cast<GraphNodes::FlowToolsNode>( pTypeInfo->CreateType() );
            EE_ASSERT( pNode->GetAllowedParentGraphTypes().IsFlagSet( m_type ) );
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

        virtual bool SupportsAutoConnection() const override { return true; }
        virtual bool DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens, VisualGraph::Flow::Node* pSourceNode, VisualGraph::Flow::Pin* pSourcePin ) override;
        virtual void HandleDragAndDrop( VisualGraph::UserContext* pUserContext, ImVec2 const& mouseCanvasPos ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const override;

    private:

        EE_REGISTER GraphType       m_type;
    };
}
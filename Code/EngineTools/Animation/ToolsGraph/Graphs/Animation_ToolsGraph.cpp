#include "Animation_ToolsGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    FlowGraph::FlowGraph( GraphType type )
        : m_type( type )
    {}

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        VisualGraph::FlowGraph::SerializeCustom( typeRegistry, graphObjectValue );
        m_type = (GraphType) graphObjectValue["GraphType"].GetUint();
    }

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        VisualGraph::FlowGraph::SerializeCustom( typeRegistry, writer );
        writer.Key( "GraphType" );
        writer.Uint( (uint8_t) m_type );
    }

    void FlowGraph::PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes )
    {
        VisualGraph::FlowGraph::PostPasteNodes( pastedNodes );
        GraphNodes::ParameterReferenceToolsNode::GloballyRefreshParameterReferences( GetRootGraph() );
    }
}
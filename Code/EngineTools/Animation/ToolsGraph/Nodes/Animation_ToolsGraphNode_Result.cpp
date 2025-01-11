#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    ResultToolsNode::ResultToolsNode()
        : FlowToolsNode()
    {}

    ResultToolsNode::ResultToolsNode( GraphValueType valueType )
        : FlowToolsNode()
        , m_valueType( valueType )
    {
        CreateInputPin( "Out", m_valueType );
    }

    int16_t ResultToolsNode::Compile( GraphCompilationContext& context ) const
    {
        // Get connected node and compile it
        auto pConnectedNode = GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pConnectedNode != nullptr )
        {
            return pConnectedNode->Compile( context );
        }

        context.LogError( this, "Result node not connected" );
        return InvalidIndex;
    }
}
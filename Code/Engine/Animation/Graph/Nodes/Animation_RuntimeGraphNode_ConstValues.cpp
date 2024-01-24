#include "Animation_RuntimeGraphNode_ConstValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ConstBoolNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstBoolNode>( context, options );
    }

    void ConstBoolNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (bool*) pOutValue ) = GetDefinition<ConstBoolNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstIDNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstIDNode>( context, options );
    }

    void ConstIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (StringID*) pOutValue ) = GetDefinition<ConstIDNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstFloatNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstFloatNode>( context, options );
    }

    void ConstFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (float*) pOutValue ) = GetDefinition<ConstFloatNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstVectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstVectorNode>( context, options );
    }

    void ConstVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (Vector*) pOutValue ) = GetDefinition<ConstVectorNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstTargetNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstTargetNode>( context, options );
    }

    void ConstTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (Target*) pOutValue ) = GetDefinition<ConstTargetNode>()->m_value;
    }
}
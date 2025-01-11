#include "Animation_RuntimeGraphNode_ConstValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ConstBoolNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstBoolNode>( context, options );
    }

    void ConstBoolNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (bool*) pOutValue ) = GetDefinition<ConstBoolNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstIDNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstIDNode>( context, options );
    }

    void ConstIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (StringID*) pOutValue ) = GetDefinition<ConstIDNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstFloatNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstFloatNode>( context, options );
    }

    void ConstFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (float*) pOutValue ) = GetDefinition<ConstFloatNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstVectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstVectorNode>( context, options );
    }

    void ConstVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (Float3*) pOutValue ) = GetDefinition<ConstVectorNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstTargetNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstTargetNode>( context, options );
    }

    void ConstTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (Target*) pOutValue ) = GetDefinition<ConstTargetNode>()->m_value;
    }
}
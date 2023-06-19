#include "Animation_RuntimeGraphNode_ConstValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ConstBoolNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstBoolNode>( context, options );
    }

    void ConstBoolNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (bool*) pOutValue ) = GetSettings<ConstBoolNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstIDNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstIDNode>( context, options );
    }

    void ConstIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (StringID*) pOutValue ) = GetSettings<ConstIDNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstFloatNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstFloatNode>( context, options );
    }

    void ConstFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (float*) pOutValue ) = GetSettings<ConstFloatNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstVectorNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstVectorNode>( context, options );
    }

    void ConstVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (Vector*) pOutValue ) = GetSettings<ConstVectorNode>()->m_value;
    }

    //-------------------------------------------------------------------------

    void ConstTargetNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ConstTargetNode>( context, options );
    }

    void ConstTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (Target*) pOutValue ) = GetSettings<ConstTargetNode>()->m_value;
    }
}
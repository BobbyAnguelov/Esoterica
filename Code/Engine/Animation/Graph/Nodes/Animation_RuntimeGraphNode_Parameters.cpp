#include "Animation_RuntimeGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ControlParameterBoolNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterBoolNode>( context, options );
    }

    void ControlParameterBoolNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (bool*) pOutValue ) = m_value;
    }

    void ControlParameterBoolNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(bool*) pInValue;
    }

    //-------------------------------------------------------------------------

    void ControlParameterIDNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterIDNode>( context, options );
    }

    void ControlParameterIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (StringID*) pOutValue ) = m_value;
    }

    void ControlParameterIDNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(StringID*) pInValue;
    }

    //-------------------------------------------------------------------------

    void ControlParameterIntNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterIntNode>( context, options );
    }

    void ControlParameterIntNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (int32_t*) pOutValue ) = m_value;
    }

    void ControlParameterIntNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(int32_t*) pInValue;
    }

    //-------------------------------------------------------------------------

    void ControlParameterFloatNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterFloatNode>( context, options );
    }

    void ControlParameterFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (float*) pOutValue ) = m_value;
    }

    void ControlParameterFloatNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(float*) pInValue;
    }

    //-------------------------------------------------------------------------

    void ControlParameterVectorNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterVectorNode>( context, options );
    }

    void ControlParameterVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (Vector*) pOutValue ) = m_value;
    }

    void ControlParameterVectorNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(Vector*) pInValue;
    }

    //-------------------------------------------------------------------------

    void ControlParameterTargetNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterTargetNode>( context, options );
    }

    void ControlParameterTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *( (Target*) pOutValue ) = m_value;
    }

    void ControlParameterTargetNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(Target*) pInValue;
    }

    //-------------------------------------------------------------------------

    void VirtualParameterBoolNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterBoolNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterBoolNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterBoolNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void VirtualParameterBoolNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<bool*>( pOutValue ) = m_pChildNode->GetValue<bool>( context );
    }

    //-------------------------------------------------------------------------

    void VirtualParameterIDNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterIDNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterIDNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        IDValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterIDNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        IDValueNode::ShutdownInternal( context );
    }

    void VirtualParameterIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<StringID*>( pOutValue ) = m_pChildNode->GetValue<StringID>( context );
    }

    //-------------------------------------------------------------------------

    void VirtualParameterIntNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterIntNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterIntNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        IntValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterIntNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        IntValueNode::ShutdownInternal( context );
    }

    void VirtualParameterIntNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<int32_t*>( pOutValue ) = m_pChildNode->GetValue<int32_t>( context );
    }

    //-------------------------------------------------------------------------

    void VirtualParameterFloatNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterFloatNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterFloatNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterFloatNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void VirtualParameterFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<float*>( pOutValue ) = m_pChildNode->GetValue<float>( context );
    }

    //-------------------------------------------------------------------------

    void VirtualParameterVectorNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterVectorNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterVectorNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        VectorValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterVectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        VectorValueNode::ShutdownInternal( context );
    }

    void VirtualParameterVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<Vector*>( pOutValue ) = m_pChildNode->GetValue<Vector>( context );
    }

    //-------------------------------------------------------------------------

    void VirtualParameterTargetNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterTargetNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterTargetNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        TargetValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterTargetNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        TargetValueNode::ShutdownInternal( context );
    }

    void VirtualParameterTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<Target*>( pOutValue ) = m_pChildNode->GetValue<Target>( context );
    }

    //-------------------------------------------------------------------------

    void VirtualParameterBoneMaskNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VirtualParameterBoneMaskNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
    }

    void VirtualParameterBoneMaskNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        BoneMaskValueNode::InitializeInternal( context );
        m_pChildNode->Initialize( context );
    }

    void VirtualParameterBoneMaskNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        m_pChildNode->Shutdown( context );
        BoneMaskValueNode::ShutdownInternal( context );
    }

    void VirtualParameterBoneMaskNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pChildNode != nullptr );
        *reinterpret_cast<BoneMask const**>( pOutValue ) = m_pChildNode->GetValue<BoneMask const*>( context );
    }
}
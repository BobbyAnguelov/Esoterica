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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (bool*) pOutValue ) = m_value;
    }

    void ControlParameterBoolNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(bool*) pInValue;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ControlParameterBoolNode::RecordGraphState( RecordedGraphState& outState )
    {
        BoolValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    void ControlParameterBoolNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        BoolValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_value );
    }
    #endif

    //-------------------------------------------------------------------------

    void ControlParameterIDNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterIDNode>( context, options );
    }

    void ControlParameterIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (StringID*) pOutValue ) = m_value;
    }

    void ControlParameterIDNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(StringID*) pInValue;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ControlParameterIDNode::RecordGraphState( RecordedGraphState& outState )
    {
        IDValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    void ControlParameterIDNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        IDValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_value );
    }
    #endif

    //-------------------------------------------------------------------------

    void ControlParameterIntNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterIntNode>( context, options );
    }

    void ControlParameterIntNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (int32_t*) pOutValue ) = m_value;
    }

    void ControlParameterIntNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(int32_t*) pInValue;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ControlParameterIntNode::RecordGraphState( RecordedGraphState& outState )
    {
        IntValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    void ControlParameterIntNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        IntValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_value );
    }
    #endif

    //-------------------------------------------------------------------------

    void ControlParameterFloatNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterFloatNode>( context, options );
    }

    void ControlParameterFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (float*) pOutValue ) = m_value;
    }

    void ControlParameterFloatNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(float*) pInValue;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ControlParameterFloatNode::RecordGraphState( RecordedGraphState& outState )
    {
        FloatValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    void ControlParameterFloatNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        FloatValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_value );
    }
    #endif

    //-------------------------------------------------------------------------

    void ControlParameterVectorNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterVectorNode>( context, options );
    }

    void ControlParameterVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (Vector*) pOutValue ) = m_value;
    }

    void ControlParameterVectorNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(Vector*) pInValue;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ControlParameterVectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        VectorValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    void ControlParameterVectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        VectorValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_value );
    }
    #endif

    //-------------------------------------------------------------------------

    void ControlParameterTargetNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ControlParameterTargetNode>( context, options );
    }

    void ControlParameterTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *( (Target*) pOutValue ) = m_value;
    }

    void ControlParameterTargetNode::SetValueInternal( void const* pInValue )
    {
        m_value = *(Target*) pInValue;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ControlParameterTargetNode::RecordGraphState( RecordedGraphState& outState )
    {
        TargetValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    void ControlParameterTargetNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        TargetValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_value );
    }
    #endif

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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pChildNode->GetValue<bool>( context );
        }

        *reinterpret_cast<bool*>( pOutValue ) = m_value;
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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pChildNode->GetValue<StringID>( context );
        }

        *reinterpret_cast<StringID*>( pOutValue ) = m_value;
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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pChildNode->GetValue<int32_t>( context );
        }

        *reinterpret_cast<int32_t*>( pOutValue ) = m_value;
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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pChildNode->GetValue<float>( context );
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pChildNode->GetValue<Vector>( context );
        }

        *reinterpret_cast<Vector*>( pOutValue ) = m_value;
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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pChildNode->GetValue<Target>( context );
        }

        *reinterpret_cast<Target*>( pOutValue ) = m_value;
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
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = *m_pChildNode->GetValue<BoneMask const*>( context );
        }

        *reinterpret_cast<BoneMask const**>( pOutValue ) = &m_value;
    }
}
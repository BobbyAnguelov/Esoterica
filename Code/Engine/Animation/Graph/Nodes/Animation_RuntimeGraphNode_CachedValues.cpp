#include "Animation_RuntimeGraphNode_CachedValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void CachedBoolNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedBoolNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedBoolNode::InitializeInternal( GraphContext& context )
    {
        BoolValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pDefinition = GetDefinition<CachedBoolNode>();
        if ( pDefinition->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<bool>( context );
            m_hasCachedValue = true;
        }
        else
        {
            m_hasCachedValue = false;
        }
    }

    void CachedBoolNode::ShutdownInternal( GraphContext& context )
    {
        m_pInputValueNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void CachedBoolNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            if ( !m_hasCachedValue )
            {
                EE_ASSERT( GetDefinition<CachedBoolNode>()->m_mode == CachedValueMode::OnExit );

                if ( context.m_branchState == BranchState::Inactive )
                {
                    m_hasCachedValue = true;
                }
                else
                {
                    m_value = m_pInputValueNode->GetValue<bool>( context );
                }
            }
        }

        *reinterpret_cast<bool*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedBoolNode::RecordGraphState( RecordedGraphState& outState )
    {
        BoolValueNode::RecordGraphState( outState );
        outState.WriteValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            outState.WriteValue( m_value );
        }
    }

    void CachedBoolNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        BoolValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            inState.ReadValue( m_value );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedIDNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedIDNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedIDNode::InitializeInternal( GraphContext& context )
    {
        IDValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pDefinition = GetDefinition<CachedIDNode>();
        if ( pDefinition->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<StringID>( context );
            m_hasCachedValue = true;
        }
        else
        {
            m_hasCachedValue = false;
        }
    }

    void CachedIDNode::ShutdownInternal( GraphContext& context )
    {
        m_pInputValueNode->Shutdown( context );
        IDValueNode::ShutdownInternal( context );
    }

    void CachedIDNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            if ( !m_hasCachedValue )
            {
                EE_ASSERT( GetDefinition<CachedIDNode>()->m_mode == CachedValueMode::OnExit );

                if ( context.m_branchState == BranchState::Inactive )
                {
                    m_hasCachedValue = true;
                }
                else
                {
                    m_value = m_pInputValueNode->GetValue<StringID>( context );
                }
            }
        }

        *reinterpret_cast<StringID*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedIDNode::RecordGraphState( RecordedGraphState& outState )
    {
        IDValueNode::RecordGraphState( outState );
        outState.WriteValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            outState.WriteValue( m_value );
        }
    }

    void CachedIDNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        IDValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            inState.ReadValue( m_value );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedFloatNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedFloatNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedFloatNode::InitializeInternal( GraphContext& context )
    {
        FloatValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        // Cache on entry
        auto pDefinition = GetDefinition<CachedFloatNode>();
        if ( pDefinition->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<float>( context );
            m_hasCachedValue = true;
        }
        else
        {
            m_hasCachedValue = false;
        }
    }

    void CachedFloatNode::ShutdownInternal( GraphContext& context )
    {
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void CachedFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            if ( !m_hasCachedValue )
            {
                EE_ASSERT( GetDefinition<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

                // Should we stop updating the current value
                if ( context.m_branchState == BranchState::Inactive )
                {
                    m_hasCachedValue = true;
                }
                else // Update the previous and current values
                {
                    m_value = m_pInputValueNode->GetValue<float>( context );
                }
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedFloatNode::RecordGraphState( RecordedGraphState& outState )
    {
        FloatValueNode::RecordGraphState( outState );
        outState.WriteValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            outState.WriteValue( m_value );
        }
    }

    void CachedFloatNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        FloatValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            inState.ReadValue( m_value );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedVectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedVectorNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedVectorNode::InitializeInternal( GraphContext& context )
    {
        VectorValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pDefinition = GetDefinition<CachedVectorNode>();
        if ( pDefinition->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<Float3>( context );
            m_hasCachedValue = true;
        }
        else
        {
            m_hasCachedValue = false;
        }
    }

    void CachedVectorNode::ShutdownInternal( GraphContext& context )
    {
        m_pInputValueNode->Shutdown( context );
        VectorValueNode::ShutdownInternal( context );
    }

    void CachedVectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            if ( !m_hasCachedValue )
            {
                EE_ASSERT( GetDefinition<CachedVectorNode>()->m_mode == CachedValueMode::OnExit );

                if ( context.m_branchState == BranchState::Inactive )
                {
                    m_hasCachedValue = true;
                }
                else
                {
                    m_value = m_pInputValueNode->GetValue<Float3>( context );
                }
            }
        }

        *reinterpret_cast<Float3*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedVectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        VectorValueNode::RecordGraphState( outState );
        outState.WriteValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            outState.WriteValue( m_value );
        }
    }

    void CachedVectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        VectorValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            inState.ReadValue( m_value );
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedTargetNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedTargetNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedTargetNode::InitializeInternal( GraphContext& context )
    {
        TargetValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pDefinition = GetDefinition<CachedTargetNode>();
        if ( pDefinition->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<Target>( context );
            m_hasCachedValue = true;
        }
        else
        {
            m_hasCachedValue = false;
        }
    }

    void CachedTargetNode::ShutdownInternal( GraphContext& context )
    {
        m_pInputValueNode->Shutdown( context );
        TargetValueNode::ShutdownInternal( context );
    }

    void CachedTargetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            if ( !m_hasCachedValue )
            {
                EE_ASSERT( GetDefinition<CachedTargetNode>()->m_mode == CachedValueMode::OnExit );

                if ( context.m_branchState == BranchState::Inactive )
                {
                    m_hasCachedValue = true;
                }
                else
                {
                    m_value = m_pInputValueNode->GetValue<Target>( context );
                }
            }
        }

        *reinterpret_cast<Target*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedTargetNode::RecordGraphState( RecordedGraphState& outState )
    {
        TargetValueNode::RecordGraphState( outState );
        outState.WriteValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            outState.WriteValue( m_value );
        }
    }

    void CachedTargetNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        TargetValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_hasCachedValue );
        if ( m_hasCachedValue )
        {
            inState.ReadValue( m_value );
        }
    }
    #endif
}
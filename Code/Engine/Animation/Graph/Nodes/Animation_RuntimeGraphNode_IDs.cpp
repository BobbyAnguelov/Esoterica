#include "Animation_RuntimeGraphNode_IDs.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void IDComparisonNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDComparisonNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void IDComparisonNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
        m_result = false;
    }

    void IDComparisonNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void IDComparisonNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<IDComparisonNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            StringID const inputID = m_pInputValueNode->GetValue<StringID>( context );

            switch ( pDefinition->m_comparison )
            {
                case Comparison::Matches:
                {
                    if ( pDefinition->m_comparisionIDs.empty() )
                    {
                        m_result = !inputID.IsValid();
                    }
                    else
                    {
                        m_result = VectorContains( pDefinition->m_comparisionIDs, inputID );
                    }
                }
                break;

                case Comparison::DoesntMatch:
                {
                    m_result = !VectorContains( pDefinition->m_comparisionIDs, inputID );
                }
                break;
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void IDToFloatNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDToFloatNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void IDToFloatNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );

        auto pDefinition = GetDefinition<IDToFloatNode>();
        EE_ASSERT( pDefinition->m_IDs.size() == pDefinition->m_values.size() );
    }

    void IDToFloatNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void IDToFloatNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pDefinition = GetDefinition<IDToFloatNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            StringID const inputID = m_pInputValueNode->GetValue<StringID>( context );
            int32_t const foundIdx = VectorFindIndex( pDefinition->m_IDs, inputID );
            if ( foundIdx != InvalidIndex )
            {
                m_value = pDefinition->m_values[foundIdx];
            }
            else
            {
                m_value = pDefinition->m_defaultValue;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void IDSwitchNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDSwitchNode>( context, options );
        context.SetNodePtrFromIndex( m_switchValueNodeIdx, pNode->m_pSwitchValueNode );
        context.SetOptionalNodePtrFromIndex( m_trueValueNodeIdx, pNode->m_pTrueValueNode );
        context.SetOptionalNodePtrFromIndex( m_falseValueNodeIdx, pNode->m_pFalseValueNode );
    }

    void IDSwitchNode::InitializeInternal( GraphContext& context )
    {
        IDValueNode::InitializeInternal( context );
        m_pSwitchValueNode->Initialize( context );

        if ( m_pTrueValueNode != nullptr )
        {
            m_pTrueValueNode->Initialize( context );
        }

        if ( m_pFalseValueNode != nullptr )
        {
            m_pFalseValueNode->Initialize( context );
        }
    }

    void IDSwitchNode::ShutdownInternal( GraphContext& context )
    {
        m_pSwitchValueNode->Shutdown( context );

        if ( m_pTrueValueNode != nullptr )
        {
            m_pTrueValueNode->Shutdown( context );
        }

        if ( m_pFalseValueNode != nullptr )
        {
            m_pFalseValueNode->Shutdown( context );
        }

        IDValueNode::ShutdownInternal( context );

        m_value.Clear();
    }

    void IDSwitchNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            auto pDefinition = GetDefinition<IDSwitchNode>();

            if ( m_pSwitchValueNode->GetValue<bool>( context ) )
            {
                m_value = m_pTrueValueNode ? m_pTrueValueNode->GetValue<StringID>( context ) : pDefinition->m_trueValue;;
            }
            else
            {
                m_value = m_pFalseValueNode ? m_pFalseValueNode->GetValue<StringID>( context ) : pDefinition->m_falseValue;;
            }
        }

        *reinterpret_cast<StringID*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void IDSelectorNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDSelectorNode>( context, options );
        for ( auto nodeIdx : m_conditionNodeIndices )
        {
            BoolValueNode*& pConditionNode = pNode->m_conditionNodes.emplace_back( nullptr );
            context.SetNodePtrFromIndex( nodeIdx, pConditionNode );
        }
    }

    void IDSelectorNode::InitializeInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        IDValueNode::InitializeInternal( context );
        for ( auto pConditionNode : m_conditionNodes )
        {
            pConditionNode->Initialize( context );
        }
    }

    void IDSelectorNode::ShutdownInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        for ( auto pConditionNode : m_conditionNodes )
        {
            pConditionNode->Shutdown( context );
        }
        IDValueNode::ShutdownInternal( context );
    }

    void IDSelectorNode::GetValueInternal( GraphContext &context, void *pOutValue )
    {
        EE_ASSERT( context.IsValid() );
        auto pDefinition = GetDefinition<IDSelectorNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            m_value = pDefinition->m_defaultValue;

            int32_t const numConditions = (int32_t) m_conditionNodes.size();
            for ( int32_t i = 0; i < numConditions; i++ )
            {
                if ( m_conditionNodes[i]->GetValue<bool>( context ) )
                {
                    m_value = pDefinition->m_values[i];
                    break;
                }
            }
        }

        *( (StringID *) pOutValue ) = m_value;
    }

    void IDSelectorNode::RecordGraphState( RecordedGraphState &outState )
    {
        IDValueNode::RecordGraphState( outState );
        outState.WriteValue( m_value );
    }

    bool IDSelectorNode::RestoreGraphState( RecordedGraphState const &inState )
    {
        if ( !IDValueNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_value );

        return true;
    }
}
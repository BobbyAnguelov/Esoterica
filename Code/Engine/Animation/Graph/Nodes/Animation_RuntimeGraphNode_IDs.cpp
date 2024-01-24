#include "Animation_RuntimeGraphNode_IDs.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
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
}
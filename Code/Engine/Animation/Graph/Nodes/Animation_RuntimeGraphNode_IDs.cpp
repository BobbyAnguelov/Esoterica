#include "Animation_RuntimeGraphNode_IDs.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void IDComparisonNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        auto pSettings = GetSettings<IDComparisonNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            StringID const inputID = m_pInputValueNode->GetValue<StringID>( context );

            switch ( pSettings->m_comparison )
            {
                case Comparison::Matches:
                {
                    m_result = VectorContains( pSettings->m_comparisionIDs, inputID );
                }
                break;

                case Comparison::DoesntMatch:
                {
                    m_result = !VectorContains( pSettings->m_comparisionIDs, inputID );
                }
                break;
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void IDToFloatNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDToFloatNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void IDToFloatNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );

        auto pSettings = GetSettings<IDToFloatNode>();
        EE_ASSERT( pSettings->m_IDs.size() == pSettings->m_values.size() );
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
        auto pSettings = GetSettings<IDToFloatNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            StringID const inputID = m_pInputValueNode->GetValue<StringID>( context );
            int32_t const foundIdx = VectorFindIndex( pSettings->m_IDs, inputID );
            if ( foundIdx != InvalidIndex )
            {
                m_value = pSettings->m_values[foundIdx];
            }
            else
            {
                m_value = pSettings->m_defaultValue;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }
}
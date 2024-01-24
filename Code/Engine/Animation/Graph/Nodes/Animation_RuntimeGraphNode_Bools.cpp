#include "Animation_RuntimeGraphNode_Bools.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void AndNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<AndNode>( context, options );

        pNode->m_conditionNodes.reserve( m_conditionNodeIndices.size() );
        for ( auto conditionNodeIdx : m_conditionNodeIndices )
        {
            context.SetNodePtrFromIndex( conditionNodeIdx, pNode->m_conditionNodes.emplace_back( nullptr ) );
        }
    }

    void AndNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        BoolValueNode::InitializeInternal( context );

        for ( auto pNode : m_conditionNodes )
        {
            pNode->Initialize( context );
        }

        m_result = false;
    }

    void AndNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        for ( auto pNode : m_conditionNodes )
        {
            pNode->Shutdown( context );
        }

        BoolValueNode::ShutdownInternal( context );
    }

    void AndNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            m_result = true;
            for ( auto pCondition : m_conditionNodes )
            {
                if ( !pCondition->GetValue<bool>( context ) )
                {
                    m_result = false;
                    break;
                }
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void OrNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<OrNode>( context, options );

        pNode->m_conditionNodes.reserve( m_conditionNodeIndices.size() );
        for ( auto conditionNodeIdx : m_conditionNodeIndices )
        {
            context.SetNodePtrFromIndex( conditionNodeIdx, pNode->m_conditionNodes.emplace_back( nullptr ) );
        }
    }

    void OrNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        BoolValueNode::InitializeInternal( context );

        for ( auto pNode : m_conditionNodes )
        {
            pNode->Initialize( context );
        }

        m_result = false;
    }

    void OrNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        for ( auto pNode : m_conditionNodes )
        {
            pNode->Shutdown( context );
        }

        BoolValueNode::ShutdownInternal( context );
    }

    void OrNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            m_result = false;
            for ( auto pCondition : m_conditionNodes )
            {
                if ( pCondition->GetValue<bool>( context ) )
                {
                    m_result = true;
                    break;
                }
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void NotNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<NotNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void NotNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
        m_result = false;
    }

    void NotNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void NotNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_result = !( m_pInputValueNode->GetValue<bool>( context ) );
        }

        *( (bool*) pOutValue ) = m_result;
    }
}
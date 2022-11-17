#include "Animation_RuntimeGraphNode_CachedValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void CachedBoolNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedBoolNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedBoolNode::InitializeInternal( GraphContext& context )
    {
        BoolValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pSettings = GetSettings<CachedBoolNode>();
        if ( pSettings->m_mode == CachedValueMode::OnEntry )
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
        MarkNodeActive( context );

        if ( !m_hasCachedValue )
        {
            EE_ASSERT( GetSettings<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

            if ( context.m_branchState == BranchState::Inactive )
            {
                m_hasCachedValue = true;
            }
            else
            {
                m_value = m_pInputValueNode->GetValue<bool>( context );
            }
        }

        *reinterpret_cast<bool*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedBoolNode::RecordGraphState( GraphStateRecorder& recorder )
    {
        BoolValueNode::RecordGraphState( recorder );
        recorder << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recorder << m_value;
        }
    }

    void CachedBoolNode::RestoreGraphState( GraphStateRecording const& recording )
    {
        BoolValueNode::RestoreGraphState( recording );
        recording << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recording << m_value;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedIDNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedIDNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedIDNode::InitializeInternal( GraphContext& context )
    {
        IDValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pSettings = GetSettings<CachedIDNode>();
        if ( pSettings->m_mode == CachedValueMode::OnEntry )
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
        MarkNodeActive( context );

        if ( !m_hasCachedValue )
        {
            EE_ASSERT( GetSettings<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

            if ( context.m_branchState == BranchState::Inactive )
            {
                m_hasCachedValue = true;
            }
            else
            {
                m_value = m_pInputValueNode->GetValue<StringID>( context );
            }
        }

        *reinterpret_cast<StringID*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedIDNode::RecordGraphState( GraphStateRecorder& recorder )
    {
        IDValueNode::RecordGraphState( recorder );
        recorder << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recorder << m_value;
        }
    }

    void CachedIDNode::RestoreGraphState( GraphStateRecording const& recording )
    {
        IDValueNode::RestoreGraphState( recording );
        recording << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recording << m_value;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedIntNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedIntNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedIntNode::InitializeInternal( GraphContext& context )
    {
        IntValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pSettings = GetSettings<CachedIntNode>();
        if ( pSettings->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<int32_t>( context );
            m_hasCachedValue = true;
        }
        else
        {
            m_hasCachedValue = false;
        }
    }

    void CachedIntNode::ShutdownInternal( GraphContext& context )
    {
        m_pInputValueNode->Shutdown( context );
        IntValueNode::ShutdownInternal( context );
    }

    void CachedIntNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        MarkNodeActive( context );

        if ( !m_hasCachedValue )
        {
            EE_ASSERT( GetSettings<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

            if ( context.m_branchState == BranchState::Inactive )
            {
                m_hasCachedValue = true;
            }
            else
            {
                m_value = m_pInputValueNode->GetValue<int32_t>( context );
            }
        }

        *reinterpret_cast<int32_t*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedIntNode::RecordGraphState( GraphStateRecorder& recorder )
    {
        IntValueNode::RecordGraphState( recorder );
        recorder << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recorder << m_value;
        }
    }

    void CachedIntNode::RestoreGraphState( GraphStateRecording const& recording )
    {
        IntValueNode::RestoreGraphState( recording );
        recording << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recording << m_value;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedFloatNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedFloatNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedFloatNode::InitializeInternal( GraphContext& context )
    {
        FloatValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        // Cache on entry
        auto pSettings = GetSettings<CachedFloatNode>();
        if ( pSettings->m_mode == CachedValueMode::OnEntry )
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
                EE_ASSERT( GetSettings<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

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
    void CachedFloatNode::RecordGraphState( GraphStateRecorder& recorder )
    {
        FloatValueNode::RecordGraphState( recorder );
        recorder << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recorder << m_value;
        }
    }

    void CachedFloatNode::RestoreGraphState( GraphStateRecording const& recording )
    {
        FloatValueNode::RestoreGraphState( recording );
        recording << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recording << m_value;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedVectorNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedVectorNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedVectorNode::InitializeInternal( GraphContext& context )
    {
        VectorValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pSettings = GetSettings<CachedVectorNode>();
        if ( pSettings->m_mode == CachedValueMode::OnEntry )
        {
            m_value = m_pInputValueNode->GetValue<Vector>( context );
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
        MarkNodeActive( context );

        if ( !m_hasCachedValue )
        {
            EE_ASSERT( GetSettings<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

            if ( context.m_branchState == BranchState::Inactive )
            {
                m_hasCachedValue = true;
            }
            else
            {
                m_value = m_pInputValueNode->GetValue<Vector>( context );
            }
        }

        *reinterpret_cast<Vector*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedVectorNode::RecordGraphState( GraphStateRecorder& recorder )
    {
        VectorValueNode::RecordGraphState( recorder );
        recorder << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recorder << m_value;
        }
    }

    void CachedVectorNode::RestoreGraphState( GraphStateRecording const& recording )
    {
        VectorValueNode::RestoreGraphState( recording );
        recording << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recording << m_value;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void CachedTargetNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CachedTargetNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void CachedTargetNode::InitializeInternal( GraphContext& context )
    {
        TargetValueNode::InitializeInternal( context );

        m_pInputValueNode->Initialize( context );

        auto pSettings = GetSettings<CachedTargetNode>();
        if ( pSettings->m_mode == CachedValueMode::OnEntry )
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
        MarkNodeActive( context );

        if ( !m_hasCachedValue )
        {
            EE_ASSERT( GetSettings<CachedFloatNode>()->m_mode == CachedValueMode::OnExit );

            if ( context.m_branchState == BranchState::Inactive )
            {
                m_hasCachedValue = true;
            }
            else
            {
                m_value = m_pInputValueNode->GetValue<Target>( context );
            }
        }

        *reinterpret_cast<Target*>( pOutValue ) = m_value;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CachedTargetNode::RecordGraphState( GraphStateRecorder& recorder )
    {
        TargetValueNode::RecordGraphState( recorder );
        recorder << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recorder << m_value;
        }
    }

    void CachedTargetNode::RestoreGraphState( GraphStateRecording const& recording )
    {
        TargetValueNode::RestoreGraphState( recording );
        recording << m_hasCachedValue;
        if ( m_hasCachedValue )
        {
            recording << m_value;
        }
    }
    #endif
}
#include "Animation_RuntimeGraphNode_BoneMasks.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void BoneMaskNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<BoneMaskNode>( context, options );
        int32_t const maskIdx = context.m_pDataSet->GetSkeleton()->GetBoneMaskIndex( m_boneMaskID );
        if ( maskIdx != InvalidIndex )
        {
            EE_ASSERT( maskIdx >= 0 && maskIdx < 255 );
            pNode->m_taskList.EmplaceTask( (uint8_t) maskIdx );
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( "Couldn't find bone mask with ID: %s", m_boneMaskID.c_str() );
            #endif

            pNode->m_taskList.EmplaceTask( 0.0f );
        }
    }

    void BoneMaskNode::InitializeInternal( GraphContext& context )
    {
        BoneMaskValueNode::InitializeInternal( context );
    }

    void BoneMaskNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
        }

        *reinterpret_cast<BoneMaskTaskList const**>( pOutValue ) = &m_taskList;
    }

    //-------------------------------------------------------------------------

    void BoneMaskBlendNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<BoneMaskBlendNode>( context, options );
        context.SetNodePtrFromIndex( m_sourceMaskNodeIdx, pNode->m_pSourceBoneMask );
        context.SetNodePtrFromIndex( m_targetMaskNodeIdx, pNode->m_pTargetBoneMask );
        context.SetNodePtrFromIndex( m_blendWeightValueNodeIdx, pNode->m_pBlendWeightValueNode );
    }

    void BoneMaskBlendNode::InitializeInternal( GraphContext& context )
    {
        BoneMaskValueNode::InitializeInternal( context );

        //-------------------------------------------------------------------------

        m_pSourceBoneMask->Initialize( context );
        m_pTargetBoneMask->Initialize( context );
        m_pBlendWeightValueNode->Initialize( context );
    }

    void BoneMaskBlendNode::ShutdownInternal( GraphContext& context )
    {
        m_pBlendWeightValueNode->Shutdown( context );
        m_pTargetBoneMask->Shutdown( context );
        m_pSourceBoneMask->Shutdown( context );

        BoneMaskValueNode::ShutdownInternal( context );
    }

    void BoneMaskBlendNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pBlendWeightValueNode != nullptr && m_pSourceBoneMask != nullptr && m_pTargetBoneMask != nullptr );

        float const blendWeight = m_pBlendWeightValueNode->GetValue<float>( context );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // If we dont need to perform the blend, set the ptr to the required source
            if ( blendWeight <= 0.0f )
            {
                m_taskList = *m_pSourceBoneMask->GetValue<BoneMaskTaskList const*>( context );
            }
            else if ( blendWeight >= 1.0f )
            {
                m_taskList = *m_pTargetBoneMask->GetValue<BoneMaskTaskList const*>( context );
            }
            else // Actually perform the blend
            {
                m_taskList.CreateBlend( *m_pSourceBoneMask->GetValue<BoneMaskTaskList const*>( context ), *m_pTargetBoneMask->GetValue<BoneMaskTaskList const*>( context ), blendWeight );
            }
        }

        *reinterpret_cast<BoneMaskTaskList const**>( pOutValue ) = &m_taskList;
    }

    //-------------------------------------------------------------------------

    void BoneMaskSelectorNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<BoneMaskSelectorNode>( context, options );

        for ( int16_t const maskNodeIdx : m_maskNodeIndices )
        {
            context.SetNodePtrFromIndex( maskNodeIdx, pNode->m_boneMaskOptionNodes.emplace_back() );
        }

        context.SetNodePtrFromIndex( m_defaultMaskNodeIdx, pNode->m_pDefaultMaskValueNode );
        context.SetNodePtrFromIndex( m_parameterValueNodeIdx, pNode->m_pParameterValueNode );
    }

    void BoneMaskSelectorNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        BoneMaskValueNode::InitializeInternal( context );
        m_pParameterValueNode->Initialize( context );

        m_pDefaultMaskValueNode->Initialize( context );

        size_t const numMasks = m_boneMaskOptionNodes.size();
        for ( auto i = 0u; i < numMasks; i++ )
        {
            m_boneMaskOptionNodes[i]->Initialize( context );
        }

        //-------------------------------------------------------------------------

        m_selectedMaskIndex = TrySelectMask( context );
        m_taskList.CopyFrom( *GetBoneMaskForIndex( context, m_selectedMaskIndex ) );
        m_newMaskIndex = InvalidIndex;
        m_isBlending = false;
    }

    void BoneMaskSelectorNode::ShutdownInternal( GraphContext& context )
    {
        m_selectedMaskIndex = InvalidIndex;
        m_newMaskIndex = InvalidIndex;
        m_isBlending = false;

        //-------------------------------------------------------------------------

        size_t const numOptions =  m_boneMaskOptionNodes.size();
        for ( auto i = 0; i < numOptions; i++ )
        {
            m_boneMaskOptionNodes[i]->Shutdown( context );
        }

        m_pDefaultMaskValueNode->Shutdown( context );

        m_pParameterValueNode->Shutdown( context );
        BoneMaskValueNode::ShutdownInternal( context );
    }

    BoneMaskTaskList const* BoneMaskSelectorNode::GetBoneMaskForIndex( GraphContext& context, int32_t optionIndex ) const
    {
        EE_ASSERT( optionIndex >= -1 && optionIndex < (int32_t) m_boneMaskOptionNodes.size() );

        if ( optionIndex != InvalidIndex )
        {
            return m_boneMaskOptionNodes[optionIndex]->GetValue<BoneMaskTaskList const*>( context );
        }
        else
        {
            return m_pDefaultMaskValueNode->GetValue<BoneMaskTaskList const*>( context );
        }
    }

    void BoneMaskSelectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Perform selection
            //-------------------------------------------------------------------------

            auto pSettings = GetSettings<BoneMaskSelectorNode>();
            if ( pSettings->m_switchDynamically )
            {
                // Only try to select a new mask if we are not blending
                if ( !m_isBlending )
                {
                    m_newMaskIndex = TrySelectMask( context );

                    // If the new mask is the same as the current one, do nothing
                    if ( m_newMaskIndex == m_selectedMaskIndex )
                    {
                        m_newMaskIndex = InvalidIndex;
                    }
                    else // Start a blend to the new mask
                    {
                        m_currentTimeInBlend = 0.0f;
                        m_isBlending = true;
                    }
                }
            }

            // Generate task list
            //-------------------------------------------------------------------------

            if ( m_isBlending )
            {
                m_currentTimeInBlend += context.m_deltaTime;
                float const blendWeight = m_currentTimeInBlend / pSettings->m_blendTime;

                // If the blend is complete, then update the selected mask index
                if ( blendWeight >= 1.0f )
                {
                    m_taskList.CopyFrom( *GetBoneMaskForIndex( context, m_newMaskIndex ) );
                    m_selectedMaskIndex = m_newMaskIndex;
                    m_newMaskIndex = InvalidIndex;
                    m_isBlending = false;
                }
                else // Perform blend and return the result
                {
                    m_taskList.CreateBlend( *GetBoneMaskForIndex( context, m_selectedMaskIndex ), *GetBoneMaskForIndex( context, m_newMaskIndex ), blendWeight );
                }
            }
        }

        *reinterpret_cast<BoneMaskTaskList const**>( pOutValue ) = &m_taskList;
    }

    #if EE_DEVELOPMENT_TOOLS
    void BoneMaskSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        BoneMaskValueNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedMaskIndex );
        outState.WriteValue( m_newMaskIndex );
        outState.WriteValue( m_currentTimeInBlend );
        outState.WriteValue( m_isBlending );
    }

    void BoneMaskSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        BoneMaskValueNode::RestoreGraphState( inState );
        inState.ReadValue( m_selectedMaskIndex );
        inState.ReadValue( m_newMaskIndex );
        inState.ReadValue( m_currentTimeInBlend );
        inState.ReadValue( m_isBlending );
    }
    #endif
}